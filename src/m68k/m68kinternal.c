#define _CRT_SECURE_NO_WARNINGS
#include "m68kinternal.h"

#ifdef M68K_INTERNAL_FUNCTIONS
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define MAXIMUM_NUMBER_CONDITIONS 10

typedef enum _CONDITIONS_FLAGS
{
    _CF_AMODE1B = 1,    // allow byte instructions in mode 1
} _CONDITIONS_FLAGS, *P_CONDITIONS_FLAGS;

typedef enum _CONDITION_TYPE
{
    _CT_AMODE,              // iterate addressing modes (_CONDITION_AMODE_INFO)
    _CT_AMODE_SWAP,         // iterate addressing modes and swap the mode/registers fields (_CONDITION_AMODE_INFO)
    _CT_CREG,               // iterate control registers
    _CT_FC,                 // iterate function codes
    _CT_FLAGS,              // change the flags (_CONDITIONS_FLAGS)
    _CT_FPOP,               // iterate fpu opmodes
    _CT_FPSIZE,             // iterate fpu sizes: l/s/x/p/w/d/b
    _CT_IMM,                // iterate immediate values using the instruction size
    _CT_IMM16,              // iterate 16-bit immediate value with an extra word
    _CT_IMM32,              // iterate 32-bit immediate value with two extra words
    _CT_INTSIZE,            // iterate integer sizes: b, w or l (_CONDITION_INTSIZE_INFO)
    _CT_OFFSETWIDTH,        // iterate values for an offset/width (always S0E11EW1)
    _CT_PREG,               // iterate values for p-register and the implicit sizes
    _CT_SIZE,               // set instruction size
    _CT_VALUE,              // iterate a value (_CONDITION_VALUE_INFO)
    _CT_VALUE_B1_S8E8EW1,   // iterate a value for address register banking with one bit (_CONDITION_VALUE_INFO)
    _CT_VALUE_VAMODE_A,     // iterate a value for the A bit used by the vector addressing mode (_CONDITION_VALUE_INFO)
    _CT_VAMODE,             // iterate vector addressing modes (_CONDITION_AMODE_INFO)
} _CONDITION_TYPE, *P_CONDITION_TYPE;

typedef M68K_WORD _CONDITION_TYPE_VALUE, *P_CONDITION_TYPE_VALUE;

typedef struct _CONDITION_AMODE_INFO
{
    ADDRESSING_MODE_TYPE_VALUE  Allowed;    // mask of allowed addressing modes
    M68K_BYTE                   StartBit;   // start bit (lowest) that is used by the effective address
    M68K_BYTE                   B1W1Bit;    // bit in the extension word 1 that is used for banking the address register
} _CONDITION_AMODE_INFO, *P_CONDITION_AMODE_INFO;

typedef struct _CONDITION_INTSIZE_INFO
{
    M68K_WORD   Allowed;    // mask of allowed sizes: bit 0 = byte, bit 1 = word, bit 2 = long
    M68K_WORD   Mask;       // mask with the bits that are changed by the condition
    M68K_WORD   OrMaskB;    // or mask for byte size
    M68K_WORD   OrMaskW;    // or mask for word size
    M68K_WORD   OrMaskL;    // or mask for long size
} _CONDITION_INTSIZE_INFO, *P_CONDITION_INTSIZE_INFO;

typedef struct _CONDITION_VALUE_INFO
{
    M68K_WORD   NumberValues;   // number of values to iterate
    M68K_WORD   Mask;           // mask with the bits that are changed by the condition
    M68K_WORD   StartValue;     // start value for the condition
    M68K_WORD   Increment;      // increment value for the condition
} _CONDITION_VALUE_INFO, *P_CONDITION_VALUE_INFO;

typedef union _CONDITION_INFO
{
    _CONDITION_AMODE_INFO   AMode;
    _CONDITIONS_FLAGS       Flags;
    _CONDITION_INTSIZE_INFO IntSize;
    M68K_BYTE               MMURegisters;
    M68K_SIZE               Size;
    _CONDITION_VALUE_INFO   Value;
} _CONDITION_INFO, *P_CONDITION_INFO;

typedef struct _CONDITION
{
    M68K_WORD               WordIndex;  // index of the word that must be changed
    _CONDITION_TYPE_VALUE   Type;       // type of condition
    _CONDITION_INFO         Info;       // information for the condition
} _CONDITION, *P_CONDITION;

typedef struct _CONDITIONS
{
    M68K_SIZE   Size;                               // size for the instruction
    M68K_UINT   Count;                              // number of conditions to iterate
    _CONDITION  Table[MAXIMUM_NUMBER_CONDITIONS];   // table of conditions
} _CONDITIONS, *P_CONDITIONS;

typedef struct _INSTR
{
    BINARY_INSTR_HEADER Header;
    M68K_WORD           Words[M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS]; // opcode words
} _INSTR, *P_INSTR;

typedef struct _ITERATE_CTX
{
    PM68K_WORD          Start;
    M68K_UINT           MaximumSize;
    M68K_UINT           TotalCount;
    M68K_DISASM_FLAGS   Flags;
    _INSTR              Instr;
    _CONDITIONS         Conditions;
} _ITERATE_CTX, *P_ITERATE_CTX;

// forward declarations
static M68K_BOOL    AddCondition(P_CONDITIONS Conditions, P_CONDITION Condition);
static M68K_BOOL    AnalyseAction(P_ITERATE_CTX ICtx, DISASM_ACTION_TYPE ActionType);
static void         GetBankExtensionWordValuesForOperand(PM68K_OPERAND Operand, P_CONDITION_VALUE_INFO Values, M68K_BYTE StartBit);
static M68K_BOOL    InitImmediate(P_ITERATE_CTX ICtx, M68K_UINT ConditionIndex, M68K_SIZE ImmSize, _CONDITIONS_FLAGS Flags);
static M68K_BOOL    IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register);
static M68K_BOOL    IterateBankExtensionWord(P_ITERATE_CTX ICtx);
static M68K_BOOL    IterateConditionsAndWriteOutput(P_ITERATE_CTX ICtx, M68K_UINT ConditionIndex, _CONDITIONS_FLAGS Flags);
static void         OrModeRegister(PM68K_WORD Word, M68K_WORD InvMask, M68K_UINT StartBit, M68K_BOOL Swap, M68K_WORD Value);
static M68K_BOOL    SaveInstr(P_ITERATE_CTX ICtx, M68K_BOOL WithBankExtensionWord, M68K_WORD BankExtensionWord);
static M68K_BOOL    SaveWord(P_ITERATE_CTX ICtx, M68K_WORD Value);
static void         WriteAutoGenerated(FILE *File, PM68KC_STR Type);

// add a new condition to the table of conditions
static M68K_BOOL AddCondition(P_CONDITIONS Conditions, P_CONDITION Condition)
{
    // enough space?
    if (Conditions->Count >= MAXIMUM_NUMBER_CONDITIONS)
        return M68K_FALSE;

    Conditions->Table[Conditions->Count++] = *Condition;
    return M68K_TRUE;
}

// analyse an action and update the instruction or create conditions
static M68K_BOOL AnalyseAction(P_ITERATE_CTX ICtx, DISASM_ACTION_TYPE ActionType)
{
    _CONDITION Condition;
    M68K_BOOL Result = M68K_FALSE;
    P_INSTR Instr = &(ICtx->Instr);
    P_CONDITIONS Conditions = &(ICtx->Conditions);

    // check a value and mask?
    if (ActionType >= DAT_C__VMSTART__ && ActionType < DAT_C__VMEND__)
    {
        // get the value and mask
        PC_VALUE_MASK ValueMask = _M68KDisValueMasks + ((M68K_LUINT)ActionType - (M68K_LUINT)DAT_C__VMSTART__);

        Condition.Type = _CT_VALUE;
        Condition.WordIndex = (ActionType >= DAT_C__VMEW2START__ ? 2 : 1);
        Condition.Info.Value.NumberValues = 1;
        Condition.Info.Value.Mask = ValueMask->Mask;
        Condition.Info.Value.StartValue = ValueMask->Value;
        Result = AddCondition(Conditions, &Condition);
    }
    else
    {
        ADDRESSING_MODE_TYPE_VALUE amode;

        switch (ActionType)
        {
        case DAT_NONE:
        case DAT_O_ImmB0_I:
        case DAT_O_RegAC0_I:
        case DAT_O_RegAC1_I:
        case DAT_O_RegACUSR_I:
        case DAT_O_RegCCR_I:
        case DAT_O_RegCRP_I:
        case DAT_O_RegFPCR_I:
        case DAT_O_RegFPIAR_I:
        case DAT_O_RegFPSR_I:
        case DAT_O_RegPCSR_I:
        case DAT_O_RegPSR_I:
        case DAT_O_RegSR_I:
        case DAT_O_RegSRP_I:
        case DAT_O_RegTC_I:
        case DAT_O_RegTT0_I:
        case DAT_O_RegTT1_I:
        case DAT_O_RegUSP_I:
        case DAT_O_RegVAL_I:
        case DAT_S_ST_I:
            Result = M68K_TRUE;
            break;

        case DAT_F_AMode1B:
            Condition.Type = _CT_FLAGS;
            Condition.Info.Flags = _CF_AMODE1B;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AbsL_NW:
        case DAT_O_DispL_NW:
        case DAT_O_ImmL_NW:
            Condition.WordIndex = 0;
            Condition.Type = _CT_IMM32;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80AMode_S0E5W0_S8E8EW1:
            amode = AMT__ALL__;
amode_B1:
            Condition.Type = _CT_VALUE_B1_S8E8EW1;
            Condition.WordIndex = 1;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0100;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            Result = AddCondition(Conditions, &Condition);

            Condition.Info.AMode.Allowed = amode;
amode:
            Condition.WordIndex = 0;
            Condition.Type = _CT_AMODE;
            Condition.Info.AMode.StartBit = 0;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80AModeA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__AC80_ALTERABLE__;
            goto amode;

        case DAT_O_AC80AModeA_S0E5W0_S8E8EW1:
            amode = AMT__AC80_ALTERABLE__;
            goto amode_B1;

        case DAT_O_AC80AReg_S12E14EW1_S7E7EW1:
            // a2/b3
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x7080;
            Condition.Info.Value.StartValue = 0x2000;
            Condition.Info.Value.Increment = 0x1080;

register_ew1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80BankEWSize_S6E7W0:
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x00c0;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0040;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80BReg_S0E2W0:
            // b6/b7
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0006;
            Condition.Info.Value.Increment = 0x0001;

register_w0:
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80BReg_S9E11W0:
            // b1/b2
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0200;
            Condition.Info.Value.Increment = 0x0200;
            goto register_w0;

        case DAT_O_AC80DReg_S12E14EW1_S7E7EW1:
            // d1/d2
            // note: the bank extension is not allowed
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x7080;
            Condition.Info.Value.StartValue = 0x1000;
            Condition.Info.Value.Increment = 0x1000;
            goto register_ew1;

        case DAT_O_AC80VAMode_S0E5W0_S8E8W0:
            amode = AMT__AC80_ALL__;

vamode:
            Condition.Type = _CT_VALUE_VAMODE_A;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0100;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 0;
            Condition.Type = _CT_VAMODE;
            Condition.Info.AMode.Allowed = amode;
            Condition.Info.AMode.StartBit = 0;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80VAModeA_S0E5W0_S8E8W0:
            amode = AMT__AC80_ALTERABLE__;
            goto vamode;

        case DAT_O_AC80VReg_S0E3EW1_S8E8W0:
            // d1/e1/e9/e17
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0100;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 1;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x000f;
            Condition.Info.Value.StartValue = 0x0001;
            Condition.Info.Value.Increment = 0x0008;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80VReg_S12E15EW1_S7E7W0:
            // d5/e5/e13/e21
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0080;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0080;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 1;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0xf000;
            Condition.Info.Value.StartValue = 0x5000;
            Condition.Info.Value.Increment = 0x8000;
            Result = AddCondition(Conditions, &Condition);
            break;
        
        case DAT_O_AC80VReg_S8E11EW1_S6E6W0:
            // d4/e4/e12/e20
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0040;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0040;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 1;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0f00;
            Condition.Info.Value.StartValue = 0x0400;
            Condition.Info.Value.Increment = 0x0800;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80VRegPM2_S8E11EW1_S6E6W0:
            // d6/e6/e14/e22
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0040;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0040;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 1;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0f00;
            Condition.Info.Value.StartValue = 0x0600;
            Condition.Info.Value.Increment = 0x0800;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AC80VRegPM4_S0E3W0_S8E8W0:
            // d4/e4/e12/e20
            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0100;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.Type = _CT_VALUE;
            Condition.WordIndex = 0;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x000f;
            Condition.Info.Value.StartValue = 0x0004;
            Condition.Info.Value.Increment = 0x0008;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AMode_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__ALL__;
            goto amode;

        case DAT_O_AModeA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__ALTERABLE__;
            goto amode;

        case DAT_O_AModeC_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL__;
            goto amode;

        case DAT_O_AModeCA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL_ALTERABLE__;
            goto amode;

        case DAT_O_AModeCAPD_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT;
            goto amode;

        case DAT_O_AModeCAPDPI_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT;
            goto amode;

        case DAT_O_AModeCPD_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT;
            goto amode;

        case DAT_O_AModeCPDPI_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT;
            goto amode;

        case DAT_O_AModeCPI_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT;
            goto amode;

        case DAT_O_AModeD_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__DATA__;
            goto amode;

        case DAT_O_AModeDA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__DATA_ALTERABLE__;
            goto amode;

        case DAT_O_AModeDA_S6E11W0:
            Condition.WordIndex = 0;
            Condition.Type = _CT_AMODE_SWAP;
            Condition.Info.AMode.Allowed = AMT__DATA_ALTERABLE__;
            Condition.Info.AMode.StartBit = 6;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AModeDAM_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__DATA_ALTERABLE_MEMORY__;
            goto amode;

        case DAT_O_AModeDAPC_S0E5W0:
            Condition.Info.AMode.Allowed = ((Instr->Header.Architectures & M68K_ARCH__P20H__) != 0 ? AMT__DATA_ALTERABLE_PC__ : AMT__DATA_ALTERABLE__);
            goto amode;

        case DAT_O_AModeDRDCA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__DATA_REGISTER_DIRECT_CONTROL_ALTERABLE__;
            goto amode;

        case DAT_O_AModeIA_S0E2W0:
            // (a6)/(a7)
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0006;
            Condition.Info.Value.Increment = 0x0001;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AModeIAD_S0E2W0:
            // (a5+nnn)
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0005;
            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 0;
            Condition.Type = _CT_IMM16;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_AModeMA_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__MEMORY_ALTERABLE__;
            goto amode;

        case DAT_O_AModePD_S0E2W0:
            // -(a1)/-(a3)
        case DAT_O_AModePI_S0E2W0:
            // (a1)+/(a3)+
        case DAT_O_DReg_S0E2W0:
            // d1/d3
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 1;
            Condition.Info.Value.Increment = 2;
            goto register_w0;

        case DAT_O_AModePD_S9E11W0:
            // -(a2)/-(a4)
        case DAT_O_AModePI_S9E11W0:
            // (a2)+/(a4)+
        case DAT_O_DReg_S9E11W0:
            // d2/d4
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0400;
            Condition.Info.Value.Increment = 0x0400;
            goto register_w0;

        case DAT_O_AModePD_S0E5W0:
            Condition.Info.AMode.Allowed = AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT;
            goto amode;

        case DAT_O_AModePFR_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__PFR__;
            goto amode;

        case DAT_O_AModePI_S12E14W1:
            // (a6)+/(a7)+
        case DAT_O_AReg_S12E14EW1:
            // a6/a7
        case DAT_O_DReg_S12E14EW1:
            // d6/d7
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x7000;
            Condition.Info.Value.StartValue = 0x6000;
            Condition.Info.Value.Increment = 0x1000;
            goto register_ew1;

        case DAT_O_AModeTOUCH_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__TOUCH__;
            goto amode;

        case DAT_O_AModeTST_S0E5W0:
            Condition.Info.AMode.Allowed = AMT__TST__;
            goto amode;

        case DAT_O_AReg_S5E7EW1:
            // a3/a4
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x00e0;
            Condition.Info.Value.StartValue = 0x0060;
            Condition.Info.Value.Increment = 0x0020;
            goto register_ew1;

        case DAT_O_AReg_S0E2W0:
            // a4/a6
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0004;
            Condition.Info.Value.Increment = 0x0002;
            goto register_w0;

        case DAT_O_AReg_S0E2EW1:
            // a5/a7
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0005;
            Condition.Info.Value.Increment = 0x0002;
            goto register_ew1;

        case DAT_O_AReg_S9E11W0:
            // a1/a3
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0200;
            Condition.Info.Value.Increment = 0x0400;
            goto register_w0;

        case DAT_O_BACReg_S2E4EW1:
            // 3/4/5/6
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x001c;
            Condition.Info.Value.StartValue = 0x000c;
            Condition.Info.Value.Increment = 0x0004;
            goto register_ew1;

        case DAT_O_BADReg_S2E4EW1:
            // 0/2/4/6
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x001c;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0008;
            goto register_ew1;

        case DAT_O_CC_S8E11W0:
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 16;
            Condition.Info.Value.Mask = 0x0f00;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_CC2H_S8E11W0:
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 14;
            Condition.Info.Value.Mask = 0x0f00;
            Condition.Info.Value.StartValue = 0x0200;
            Condition.Info.Value.Increment = 0x0100;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_CPID_S9E11W0:
cpid:
            // 0, 1, 2
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 3;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0200;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_CPIDCC_S9E11W0_S0E5W0:
        case DAT_O_CPIDCC_S9E11W0_S0E5EW1:
            // cc: 00/3f
            Condition.WordIndex = (ActionType == DAT_O_CPIDCC_S9E11W0_S0E5W0 ? 0 : 1);
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x003f;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x003f;
            if (!AddCondition(Conditions, &Condition))
                break;
        
            goto cpid;

        case DAT_O_CReg_S0E11EW1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_CREG;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_CT_S6E7W0:
            // nc/dc/ic/bc
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x00c0;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0040;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DispB_S0E7W0:
            // $22/$44
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x00ff;
            Condition.Info.Value.StartValue = 0x0022;
            Condition.Info.Value.Increment = 0x0022;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DispW_NW:
        case DAT_O_ImmW_NW:
            Condition.WordIndex = 0;
            Condition.Type = _CT_IMM16;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DKFac_S4E6EW1:
            // {d3}/{d4}
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0070;
            Condition.Info.Value.StartValue = 0x0030;
            Condition.Info.Value.Increment = 0x0010;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DReg_S0E2EW1:
            // d0/d1
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0001;
            goto register_ew1;

        case DAT_O_DReg_S4E6EW1:
            // d4/d6
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0070;
            Condition.Info.Value.StartValue = 0x0040;
            Condition.Info.Value.Increment = 0x0020;
            goto register_ew1;

        case DAT_O_DReg_S6E8EW1:
            // d4/d5
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x01c0;
            Condition.Info.Value.StartValue = 0x0100;
            Condition.Info.Value.Increment = 0x0040;
            goto register_ew1;

        case DAT_O_DRegP_S0E2EW1_S12E14EW1:
            // d5:d6
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x7007;
            Condition.Info.Value.StartValue = 0x6005;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DRegP_S0E2W0_S0E2EW1:
            // d5:d6
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0005;

            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 1;
            Condition.Info.Value.StartValue = 0x0006;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DRegP_S0E2EW1_S0E2EW2:
            // d1:d2
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0001;

            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 2;
            Condition.Info.Value.StartValue = 0x0002;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_DRegP_S6E8EW1_S6E8EW2:
            // d3:d4
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x01c0;
            Condition.Info.Value.StartValue = 0x00c0;

            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 2;
            Condition.Info.Value.StartValue = 0x0100;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_FC_S0E4EW1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_FC;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_FPCC_S0E4W0:
        case DAT_O_FPCC_S0E4EW1:
            Condition.WordIndex = (ActionType == DAT_O_FPCC_S0E4W0 ? 0 : 1);
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 32;
            Condition.Info.Value.Mask = 0x001f;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0001;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_FPCRRegList_S10E12EW1:
            // fpcr/fpiar and fpcr/fpsr
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x1c00;
            Condition.Info.Value.StartValue = 0x1400;
            Condition.Info.Value.Increment = 0x0400;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_FPOP_S0E7EW1:
            // note: fpu opmodes are always located in bits 0-7 of extension word 1
            Condition.WordIndex = 1;
            Condition.Type = _CT_FPOP;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_FPReg_S0E2EW1:
            // fp4/fp5
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0004;
            Condition.Info.Value.Increment = 0x0001;
            goto register_ew1;

        case DAT_O_FPReg_S7E9EW1:
            // fp2/fp3
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0380;
            Condition.Info.Value.StartValue = 0x0100;
            Condition.Info.Value.Increment = 0x0080;
            goto register_ew1;

        case DAT_O_FPReg_S10E12EW1:
            // fp0/fp7
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x1c00;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x1c00;
            goto register_ew1;

        case DAT_O_FPRegIList_S0E7EW1:
            // fp0-fp1/fp4-fp7
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x00ff;
            Condition.Info.Value.StartValue = 0x00cf;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_FPRegList_S0E7EW1:
            // fp2-fp6
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0x00ff;
            Condition.Info.Value.StartValue = 0x007c;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E1W0:
            // 2/3
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0003;
            Condition.Info.Value.StartValue = 0x0002;
            Condition.Info.Value.Increment = 0x0001;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E2W0:
            // 3/5
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0007;
            Condition.Info.Value.StartValue = 0x0003;
            Condition.Info.Value.Increment = 0x0002;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E3W0:
            // 4/12
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x000f;
            Condition.Info.Value.StartValue = 0x0004;
            Condition.Info.Value.Increment = 0x0008;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E6EW1:
            // $30/$31/$32/$33
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x007f;
            Condition.Info.Value.StartValue = 0x0030;
            Condition.Info.Value.Increment = 0x0001;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E7W0:
            // 3/5
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x00ff;
            Condition.Info.Value.StartValue = 0x002f;
            Condition.Info.Value.Increment = 0x0080;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S0E7EW1:
            // $55/$aa
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x00ff;
            Condition.Info.Value.StartValue = 0x0055;
            Condition.Info.Value.Increment = 0x0055;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S10E12EW1:
            // 3/6
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x1c00;
            Condition.Info.Value.StartValue = 0x0c00;
            Condition.Info.Value.Increment = 0x0c00;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S2E3W0:
            // 1/2
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x000c;
            Condition.Info.Value.StartValue = 0x0004;
            Condition.Info.Value.Increment = 0x0004;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S8E9EW1:
            // 0/1/2/3
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 4;
            Condition.Info.Value.Mask = 0x0300;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0100;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S9E11W0:
            // 4/5
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0800;
            Condition.Info.Value.Increment = 0x0200;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmB_S9E11W0_S4E5W0:
            // 0/18
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e30;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0420;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmS:
            Condition.WordIndex = 0;
            Condition.Type = _CT_IMM;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_ImmW_S0E11EW1:
            // 0b011_011_011_011 = 0x6db / 0xfff
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0fff;
            Condition.Info.Value.StartValue = 0x06db;
            Condition.Info.Value.Increment = 0x0924;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_MMUCC_S0E3W0:
        case DAT_O_MMUCC_S0E3EW1:
            Condition.WordIndex = (ActionType == DAT_O_MMUCC_S0E3W0 ? 0 : 1);
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 16;
            Condition.Info.Value.Mask = 0x000f;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0001;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_MMUMask_S5E8EW1:
            // 1/3/5/7(/9/11)
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = ((Instr->Header.Architectures & M68K_ARCH_68030) != 0 ? 4 : 6);
            Condition.Info.Value.Mask = 0x01e0;
            Condition.Info.Value.StartValue = 0x0020;
            Condition.Info.Value.Increment = 0x0040;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_MRegP_S12E15EW1_S12E15EW2:
            // (d2):(a4)
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0xf000;
            Condition.Info.Value.StartValue = 0x2000;

            if (!AddCondition(Conditions, &Condition))
                break;

            Condition.WordIndex = 2;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0xf000;
            Condition.Info.Value.StartValue = 0xc000;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_OW_S0E11EW1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_OFFSETWIDTH;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_PReg_S10E12EW1:
            // iteration is done using DAT_S_PReg_S10E12EW1
            Result = M68K_TRUE;
            break;

        case DAT_O_RegList_EW1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 1;
            Condition.Info.Value.Mask = 0xffff;
            Condition.Info.Value.StartValue = 0xfaf5;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_Shift8_S9E11W0:
            // 8(0)/6
            Condition.WordIndex = 0;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 2;
            Condition.Info.Value.Mask = 0x0e00;
            Condition.Info.Value.StartValue = 0x0000;
            Condition.Info.Value.Increment = 0x0c00;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_O_SKFac_S0E6EW1:
            // -16, 1, 18
            Condition.WordIndex = 1;
            Condition.Type = _CT_VALUE;
            Condition.Info.Value.NumberValues = 3;
            Condition.Info.Value.Mask = 0x003f;
            Condition.Info.Value.StartValue = 0x0030;
            Condition.Info.Value.Increment = 0x0011;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_B_I:
            // b
            Condition.Info.IntSize.Allowed = 0b001;

int_size_implicit:
            Condition.Info.IntSize.Mask = 0x0000;
            Condition.Info.IntSize.OrMaskB = 0x0000;
            Condition.Info.IntSize.OrMaskW = 0x0000;
            Condition.Info.IntSize.OrMaskL = 0x0000;

int_size:
            Condition.WordIndex = 0;
            Condition.Type = _CT_INTSIZE;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_BWL_S6E7W0:
            // b/w/l
            Condition.Info.IntSize.Mask = 0x00c0;
            Condition.Info.IntSize.OrMaskB = 0x0000;
            Condition.Info.IntSize.OrMaskW = 0x0040;
            Condition.Info.IntSize.OrMaskL = 0x0080;

int_size_111:
            Condition.Info.IntSize.Allowed = 0b111;
            goto int_size;

        case DAT_S_BWL_S6E7EW1:
            // b/w/l
            Condition.WordIndex = 1;
            Condition.Type = _CT_INTSIZE;
            Condition.Info.IntSize.Allowed = 0b111;
            Condition.Info.IntSize.Mask = 0x00c0;
            Condition.Info.IntSize.OrMaskB = 0x0000;
            Condition.Info.IntSize.OrMaskW = 0x0040;
            Condition.Info.IntSize.OrMaskL = 0x0080;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_BWL_S9E10W0:
            // b/w/l
            Condition.Info.IntSize.Mask = 0x0600;
            Condition.Info.IntSize.OrMaskB = 0x0000;
            Condition.Info.IntSize.OrMaskW = 0x0200;
            Condition.Info.IntSize.OrMaskL = 0x0400;
            goto int_size_111;

        case DAT_S_FP_S10E12EW1:
            // l/s/x/p/w/d/b
            Condition.WordIndex = 1;
            Condition.Type = _CT_FPSIZE;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_L_I:
            // l
            Condition.Info.IntSize.Allowed = 0b100;
            goto int_size_implicit;

        case DAT_S_P_I:
            // p
            Condition.Type = _CT_SIZE;
            Condition.Info.Size = M68K_SIZE_P;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_PReg_S10E12EW1:
            Condition.WordIndex = 1;
            Condition.Type = _CT_PREG;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_Q_I:
            // q
            Condition.Type = _CT_SIZE;
            Condition.Info.Size = M68K_SIZE_Q;
            Result = AddCondition(Conditions, &Condition);
            break;

        case DAT_S_W_I:
            // w
            Condition.Info.IntSize.Allowed = 0b010;
            goto int_size_implicit;

        case DAT_S_WL_S6E6W0:
            // w/l
            Condition.Info.IntSize.Mask = 0x0040;
            Condition.Info.IntSize.OrMaskW = 0x0000;
            Condition.Info.IntSize.OrMaskL = 0x0040;

int_size_110:
            Condition.Info.IntSize.Allowed = 0b110;
            goto int_size;

        case DAT_S_WL_S8E8W0:
            // w/l
            Condition.Info.IntSize.Mask = 0x0100;
            Condition.Info.IntSize.OrMaskW = 0x0000;
            Condition.Info.IntSize.OrMaskL = 0x0100;
            goto int_size_110;

        case DAT_S_X_I:
            Condition.Type = _CT_SIZE;
            Condition.Info.Size = M68K_SIZE_X;
            Result = AddCondition(Conditions, &Condition);
            break;

        default:
            break;
        }
    }

    return Result;
}

// get the values that we need iterate for a bank extension word
static void GetBankExtensionWordValuesForOperand(PM68K_OPERAND Operand, P_CONDITION_VALUE_INFO Values, M68K_BYTE StartBit)
{
    M68K_BOOL CanIterate = M68K_FALSE;

    if (Operand != M68K_NULL)
    {
        // only a few operands can be extended
        switch (Operand->Type)
        {
        case M68K_OT_MEM_INDIRECT:
            // (An) => (Bn)
        case M68K_OT_MEM_INDIRECT_DISP_W:
            // (d16, An) => (d16, Bn)
        case M68K_OT_MEM_INDIRECT_INDEX_DISP_B:
            // (d8, An, Xn.SIZE * SCALE) => (d8, Bn, Xn.SIZE * SCALE)
        case M68K_OT_MEM_INDIRECT_INDEX_DISP_L:
            // (d32, An, Xn.SIZE * SCALE) => (d32, Bn, Xn.SIZE * SCALE)
        case M68K_OT_MEM_INDIRECT_POST_INCREMENT:
            // (An)+ => (Bn)+
        case M68K_OT_MEM_INDIRECT_POST_INDEXED:
            // ([d32i, An], Xn.SIZE * SCALE, d32o) => ([d32i, Bn], Xn.SIZE * SCALE, d32o)
        case M68K_OT_MEM_INDIRECT_PRE_DECREMENT:
            // -(An) => -(Bn)
        case M68K_OT_MEM_INDIRECT_PRE_INDEXED:
            // ([d32i, An, Xn.SIZE * SCALE], d32o) => ([d32i, Bn, Xn.SIZE * SCALE], d32o)
            if (IsValidRegister07(M68K_RT_A0, Operand->Info.Memory.Base))
            {
                Values->NumberValues = 2;
                CanIterate = M68K_TRUE;
            }
            break;

        case M68K_OT_REGISTER:
            // Dn => Dn/En
            // An => Bn
            // FPn => FPn/En
            if (IsValidRegister07(M68K_RT_D0, Operand->Info.Register) ||
                IsValidRegister07(M68K_RT_FP0, Operand->Info.Register))
            {
                Values->NumberValues = 4;
                CanIterate = M68K_TRUE;
            }
            else if (IsValidRegister07(M68K_RT_A0, Operand->Info.Register))
            {
                Values->NumberValues = 2;
                CanIterate = M68K_TRUE;
            }
            break;

        default:
            break;
        }
    }

    if (CanIterate)
    {
        Values->Mask = 3 << StartBit;
        Values->StartValue = 0x0000;
        Values->Increment = 1 << StartBit;
    }
    else
        Values->NumberValues = 0;
}

// init the instruction for an immediate value
static M68K_BOOL InitImmediate(P_ITERATE_CTX ICtx, M68K_UINT ConditionIndex, M68K_SIZE ImmSize, _CONDITIONS_FLAGS Flags)
{
    P_INSTR Instr = &(ICtx->Instr);

    switch (ImmSize)
    {
    case M68K_SIZE_B:
        // #$1a
        Instr->Words[Instr->Header.NumberWords++] = 0x001a;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // restore the number of words
        Instr->Header.NumberWords--;
        break;

    case M68K_SIZE_D:
        {
            // a double value uses 4 words
            PM68K_WORD Words = Instr->Words + Instr->Header.NumberWords;
            Instr->Header.NumberWords += 4;

            // +0
            Words[0] = 0x0000;
            Words[1] = 0x0000;
            Words[2] = 0x0000;
            Words[3] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -denorm
            Words[0] = 0x8000;
            Words[3] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -inf
            Words[0] = 0xfff0;
            Words[3] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +nan
            Words[0] = 0x7ff0;
            Words[3] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +$1.5682611d04c49p+23
            Words[0] = 0x4165;
            Words[1] = 0x6826;
            Words[2] = 0x11d0;
            Words[3] = 0x4c49;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // restore the number of words
            Instr->Header.NumberWords -= 4;
        }
        break;

    case M68K_SIZE_L:
        // #$3a3b3c3d
        Instr->Words[Instr->Header.NumberWords++] = 0x3a3b;
        Instr->Words[Instr->Header.NumberWords++] = 0x3c3d;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // restore the number of words
        Instr->Header.NumberWords -= 2;
        break;

    case M68K_SIZE_P:
        {
            // a packed decimal value uses 6 words
            PM68K_WORD Words = Instr->Words + Instr->Header.NumberWords;
            Instr->Header.NumberWords += 6;

            Words[0] = 0xc321;
            Words[1] = 0x0001;
            Words[2] = 0x5432;
            Words[3] = 0x1098;
            Words[4] = 0x7654;
            Words[5] = 0x3210;
                        
            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // restore the number of words
            Instr->Header.NumberWords -= 6;
        }
        break;

    case M68K_SIZE_Q:
        {
            // quad word value uses 4 words
            PM68K_WORD Words = Instr->Words + Instr->Header.NumberWords;
            Instr->Header.NumberWords += 4;

            Words[0] = 0x8899;
            Words[1] = 0xaabb;
            Words[2] = 0xccdd;
            Words[3] = 0xeeff;
                        
            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // restore the number of words
            Instr->Header.NumberWords -= 4;
        }
        break;

    case M68K_SIZE_S:
        {
            // a single value uses 2 words
            PM68K_WORD Words = Instr->Words + Instr->Header.NumberWords;
            Instr->Header.NumberWords += 2;

            // +0
            Words[0] = 0x0000;
            Words[1] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -denorm
            Words[0] = 0x8000;
            Words[1] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -inf
            Words[0] = 0xff80;
            Words[1] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +nan
            Words[0] = 0x7f80;
            Words[1] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -0x1.edd2f2p+6
            Words[0] = 0xc2f6;
            Words[1] = 0xe979;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // restore the number of words
            Instr->Header.NumberWords -= 2;
        }
        break;

    case M68K_SIZE_W:
        // #$2a2b
        Instr->Words[Instr->Header.NumberWords++] = 0x2a2b;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // restore the number of words
        Instr->Header.NumberWords--;
        break;

    case M68K_SIZE_X:
        {
            // an extended value uses 6 words
            PM68K_WORD Words = Instr->Words + Instr->Header.NumberWords;
            Instr->Header.NumberWords += 6;

            // -0
            Words[0] = 0x8000;
            Words[1] = 0x0000;
            Words[2] = 0x0000;
            Words[3] = 0x0000;
            Words[4] = 0x0000;
            Words[5] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -denorm
            Words[5] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +pinf
            Words[0] = 0x7fff;
            Words[5] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +pnan
            Words[5] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -inf
            Words[0] = 0xffff;
            Words[2] = 0x8000;
            Words[5] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // -nan
            Words[5] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +ind
            Words[0] = 0x7fff;
            Words[2] = 0xc000;
            Words[5] = 0x0000;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +qnan
            Words[5] = 0x0001;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // +$1.2222222222222222p-12287
            Words[0] = 0x1000;
            Words[2] = 0x9111;
            Words[3] = 0x1111;
            Words[4] = 0x1111;
            Words[5] = 0x1111;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            // restore the number of words
            Instr->Header.NumberWords -= 6;
        }
        break;

    case M68K_SIZE_NONE:
    default:
        return M68K_FALSE;
    }

    return M68K_TRUE;
}

// check if we can use the bank extension word and generate several values for it
static M68K_BOOL IterateBankExtensionWord(P_ITERATE_CTX ICtx)
{
    P_INSTR Instr = &(ICtx->Instr);

    // get the location where the opcodes will be generated;
    // see SaveInstr to understand why we add 4
    PM68K_WORD Start = ICtx->Start + 4;

    if (!SaveInstr(ICtx, M68K_FALSE, 0))
        return M68K_FALSE;

    // can we use the bank extension word?
    if ((ICtx->Flags & M68K_DFLG_ALLOW_BANK_EXTENSION_WORD) != 0 &&
        (Instr->Header.Architectures & M68K_ARCH_AC68080) != 0)
    {
        M68K_INSTRUCTION LocalInstruction;

        // we need the internal type to get the instruction flags;
        // the only way to get this is by disassembling what we just wrote to the output
        if (M68KDisassemble(M68K_NULL, Start, Start + Instr->Header.NumberWords, ICtx->Flags, Instr->Header.Architectures, &LocalInstruction) != M68K_NULL)
        {
            INSTRUCTION_FLAGS_VALUE IFlags = _M68KInstrTypeInfos[LocalInstruction.InternalType].IFlags;

            if ((IFlags & IF_FPU_OPMODE) != 0)
                IFlags = _M68KInstrFPUOpmodes[_M68KInstrMasterTypeFPUOpmodes[LocalInstruction.Type]].IFlags;

            if ((IFlags & IF_BANK_EW_A) != 0)
            {
                // size of all opcodes is too large?
                // it can't use more than 4 words because the maximum size (including the bank) is 10 ((1 + 4) * 2)
                if (Instr->Header.NumberWords <= 4)
                {
                    _CONDITION_VALUE_INFO Values[3];
                    M68K_WORD BankExtensionWord = 0x7100 | (M68K_WORD)((Instr->Header.NumberWords - 1) << 6);

                    GetBankExtensionWordValuesForOperand((IFlags & IF_BANK_EW_A) != 0 ? LocalInstruction.Operands + 0 : M68K_NULL, Values + 0, 2);
                    GetBankExtensionWordValuesForOperand((IFlags & IF_BANK_EW_B) != 0 ? LocalInstruction.Operands + 1 : M68K_NULL, Values + 1, 0);

                    if ((IFlags & IF_BANK_EW_C) != 0)
                    {
                        Values[2].NumberValues = 4;
                        Values[2].Mask = 0x0e30;
                        Values[2].StartValue = 0x0000;
                        Values[2].Increment = 0x0010;
                    }
                    else
                        Values[2].NumberValues = 0;

                    // iterate all values
                    M68K_WORD CurrentValue0 = Values[0].StartValue;
                    for (M68K_WORD Index0 = 0; Index0 < Values[0].NumberValues; Index0++)
                    {
                        BankExtensionWord &= ~Values[0].Mask;
                        BankExtensionWord |= CurrentValue0;
                        CurrentValue0 += Values[0].Increment;

                        M68K_WORD Index1 = 0;
                        if (Index1 < Values[1].NumberValues)
                        {
                            M68K_WORD CurrentValue1 = Values[1].StartValue;
                            for (; Index1 < Values[1].NumberValues; Index1++)
                            {
                                BankExtensionWord &= ~Values[1].Mask;
                                BankExtensionWord |= CurrentValue1;
                                CurrentValue1 += Values[1].Increment;

                                M68K_WORD Index2 = 0;
                                if (Index2 < Values[2].NumberValues)
                                {
                                    M68K_WORD CurrentValue2 = Values[2].StartValue;
                                    for (; Index2 < Values[2].NumberValues; Index2++)
                                    {
                                        BankExtensionWord &= ~Values[2].Mask;
                                        BankExtensionWord |= CurrentValue2;
                                        CurrentValue2 += Values[2].Increment;

                                        if (!SaveInstr(ICtx, M68K_TRUE, BankExtensionWord))
                                            return M68K_FALSE;
                                    }
                                }
                                else if (!SaveInstr(ICtx, M68K_TRUE, BankExtensionWord))
                                    return M68K_FALSE;
                            }
                        }
                        else if (!SaveInstr(ICtx, M68K_TRUE, BankExtensionWord))
                            return M68K_FALSE;
                    }
                }
            }
        }
    }

    return M68K_TRUE;
}

// iterate the conditions and write the instruction to the output
static M68K_BOOL IterateConditionsAndWriteOutput(P_ITERATE_CTX ICtx, M68K_UINT ConditionIndex, _CONDITIONS_FLAGS Flags)
{
    // no more conditions?
    if (ConditionIndex >= ICtx->Conditions.Count)
        return IterateBankExtensionWord(ICtx);
    
    M68K_BOOL VEA;
    M68K_BOOL Swap;
    M68K_WORD InvMask;
    P_INSTR Instr = &(ICtx->Instr);
    P_CONDITIONS Conditions = &(ICtx->Conditions);
    P_CONDITION Condition = Conditions->Table + ConditionIndex;
    PM68K_WORD Word = Instr->Words + Condition->WordIndex;

    switch ((_CONDITION_TYPE)Condition->Type)
    {
    case _CT_AMODE:
        Swap = M68K_FALSE;
        VEA = M68K_FALSE;
amode:
        {
            // get more information about the addressing mode
            M68K_UINT StartBit = (M68K_UINT)Condition->Info.AMode.StartBit;
            InvMask = (M68K_WORD)~((M68K_WORD)0x3f << StartBit);
            ADDRESSING_MODE_TYPE_VALUE AMode = Condition->Info.AMode.Allowed;

            // one or more addressing modes?
            if (AMode == 0)
                return M68K_FALSE;

            M68K_SIZE Size = (M68K_SIZE)Conditions->Size;

            // selecting a different address bank? if so do not generate the addressing modes that cant't use it
            M68K_BOOL forAddressBank1 = M68K_FALSE;
            if (ConditionIndex > 0)
            {
                P_CONDITION PrevCondition = Conditions->Table + (ConditionIndex - 1);
                if ((!VEA && PrevCondition->Type == _CT_VALUE_B1_S8E8EW1) || (VEA && PrevCondition->Type == _CT_VALUE_VAMODE_A))
                {
                    // the bit is set?
                    forAddressBank1 = ((Instr->Words[PrevCondition->WordIndex] & PrevCondition->Info.Value.Mask) != 0);
                }
            }

            // iterate the addressing modes according to the mask
            // Dn/En (when size is NONE, B, W, L or S because all other sizes are invalid);
            // in VEA mode we also allow size Q
            if (!forAddressBank1 && (AMode & AMT_DATA_REGISTER_DIRECT) != 0)
            {
                if (Size == M68K_SIZE_NONE || Size == M68K_SIZE_B || Size == M68K_SIZE_W || Size == M68K_SIZE_L || Size == M68K_SIZE_S || (VEA && Size == M68K_SIZE_Q))
                {
                    // d5/e13: mode = 000, register = 101
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b000101);

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }
            }

            // An/Bn/En
            // note: 
            // address registers can't be used with byte operations so the size must be W or L;
            // the exception is apollo core 68080 which 
            // - includes some instructions where An/Bn can be used as a byte;
            // - in VEA mode the size is always Q
            if ((AMode & (AMT_ADDRESS_REGISTER_DIRECT | AMT_ADDRESS_REGISTER_INDIRECT_P20H)) != 0)
            {
                if ((Size == M68K_SIZE_B && (Flags & _CF_AMODE1B) != 0) || Size == M68K_SIZE_W || Size == M68K_SIZE_L || (VEA && Size == M68K_SIZE_Q))
                {
                    // a3/b3/e3/e19: mode = 001, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b001011);

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }
            }

            // (An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT) != 0)
            {
                // (a4/b4): mode = 010, register = 100
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b010100);

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;
            }

            // (An/Bn)+
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT) != 0)
            {
                // (a6/b6)++: mode = 011, register = 110
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b011110);

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;
            }

            // -(An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT) != 0)
            {
                // --(a2/b2): mode = 100, register = 010
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b100010);

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;
            }

            // (d16, An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT) != 0)
            {
                // (a0/b0 - $2710): mode = 101, register = 000
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b101000);
            
                Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                // restore the number of words
                Instr->Header.NumberWords--;
            }
        
            // (d8, An/Bn, Xn.SIZE * SCALE)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_B) != 0)
            {
                PM68K_WORD BriefEWord = Instr->Words + (Instr->Header.NumberWords++);

                // scale must be 1 in older architectures i.e. 68000, 68008 and 68010
                if ((Instr->Header.Architectures & M68K_ARCH__P20H__) != 0)
                {
                    // (a3/b3 + d6.w * 2 - $9c): mode = 110, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110011);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b1110001010011100;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (a0/b0 + a7.l * 8 + $64): mode = 110, register = 000
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b0111111001100100;
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }
                else
                {
                    // (a3/b3 + d6.w * 1 - $9c): mode = 110, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110011);
                
                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b1110000010011100;
                
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (a0/b0 + a7.l * 1 + $64): mode = 110, register = 000
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b0111100001100100;
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }

                // restore the number of words
                Instr->Header.NumberWords--;
            }

            if ((Instr->Header.Architectures & M68K_ARCH__P20H__) != 0)
            {
                // (d32, An/Bn, Xn.SIZE * SCALE) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0)
                {
                    PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                    if (!forAddressBank1)
                    {
                        // (d7.l * 2): mode = 110, register = 000
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b0111101110010000;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;
                    }

                    // (a1/b1 + d3.w * 4 - $2710): mode = 110, register = 001
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110001);

                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0011010100100000;
                    Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;
            
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    if (!forAddressBank1)
                    {
                        // ($1234): mode = 110, register = ---
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b0000000111100000;
                        Instr->Words[Instr->Header.NumberWords - 1] = 0x1234;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;
                    }

                    // (a4/b4 + $f4240): mode = 110, register = 100
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b110100);

                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0000000101110000;
                    Instr->Words[Instr->Header.NumberWords - 1] = (M68K_WORD)0x000f;
                    Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // restore the number of words
                    Instr->Header.NumberWords -= 3;
                }

                if ((Instr->Header.Architectures & M68K_ARCH__P20304060__) != 0)
                {
                    // ([d32i, An/Bn], Xn.SIZE * SCALE, d32o) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                    if ((AMode & AMT_MEMORY_INDIRECT_POST_INDEXED) != 0)
                    {
                        PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                        // ([a5/b5 - $2710] + a2.w * 2 + $f4240): mode = 110, register = 101
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b110101);

                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b1010001100100111;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        if (!forAddressBank1)
                        {
                            // ([$f4240] + $f2441): mode = 110, register = ---
                            //             abbbcdd1efgg0hhh
                            OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                            //             abbbcdd1efgg0hhh
                            *FullEWord = 0b0000000111110011;

                            Instr->Words[Instr->Header.NumberWords - 3] = (M68K_WORD)0x000f;
                            Instr->Words[Instr->Header.NumberWords - 2] = (M68K_WORD)0x4240;
                            Instr->Words[Instr->Header.NumberWords - 1] = (M68K_WORD)0x000f;
                            Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4241;

                            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                                return M68K_FALSE;

                            // ([$f4240] + a3.w*4): mode = 110, register = ---
                            OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                            //             abbbcdd1efgg0hhh
                            *FullEWord = 0b1011010110110101;

                            Instr->Header.NumberWords -= 2;

                            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                                return M68K_FALSE;

                            // restore the number of words
                            Instr->Header.NumberWords -= 3;
                        }
                        else
                            // restore the number of words
                            Instr->Header.NumberWords -= 4;
                    }

                    // ([d32i, An/Bn, Xn.SIZE * SCALE], d32o) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                    if ((AMode & AMT_MEMORY_INDIRECT_PRE_INDEXED) != 0)
                    {
                        PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                        if (!forAddressBank1)
                        {
                            // [[0]]: mode = 110, register = ---
                            OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                            //             abbbcdd1efgg0hhh
                            *FullEWord = 0b0000000111010001;

                            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                                return M68K_FALSE;
                        }

                        // ([a0/b0 + a5*1 + $f4240] + $2710): mode = 110, register = 000
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);
        
                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b1101100100110010;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        if (!forAddressBank1)
                        {
                            // ([a1.w*2 + $f4240]): mode = 110, register = 000
                            OrModeRegister(Word, InvMask, StartBit, Swap, 0b110000);

                            //             abbbcdd1efgg0hhh
                            *FullEWord = 0b1001010110010001;

                            Instr->Header.NumberWords -= 3;

                            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                                return M68K_FALSE;

                            // restore the number of words
                            Instr->Header.NumberWords -= 1;
                        }
                        else
                            // restore the number of words
                            Instr->Header.NumberWords -= 4;
                    }
                }
            }

            // (d16, PC)
            if (!forAddressBank1 && (
                (AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT) != 0 ||
                ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT_P20H) != 0 && (Instr->Header.Architectures & M68K_ARCH__P20H__) != 0)
            ))
            {
                // (pc + $2710): mode = 111, register = 010
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b111010);
            
                Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)10000;

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                // restore the number of words
                Instr->Header.NumberWords--;
            }

            // (d8, PC, Xn.SIZE * SCALE) 
            if (!forAddressBank1 && (AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_B) != 0)
            {
                PM68K_WORD BriefEWord = Instr->Words + (Instr->Header.NumberWords++);

                // scale must be 1 in older architectures i.e. 68000, 68008 and 68010
                if ((Instr->Header.Architectures & M68K_ARCH__P20H__) != 0)
                {
                    // (pc + d0.w * 2 - $9c): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b1000001010011100;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (pc + a7.l * 8 + $64): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b0111111001100100;
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }
                else
                {
                    // (pc + d0.w * 1 - $9c): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b1000000010011100;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (pc + a7.l * 1 + $64): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //              abbbcdd0eeeeeeee
                    *BriefEWord = 0b0111100001100100;
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }

                // restore the number of words
                Instr->Header.NumberWords--;
            }

            if ((Instr->Header.Architectures & M68K_ARCH__P20H__) != 0)
            {
                // (d32, PC, Xn.SIZE * SCALE) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                if (!forAddressBank1 && (AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0)
                {
                    PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                    // (pc + d3.w * 4 - $2710): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0011010100100000;
                    Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;
            
                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (zpc + $1234): mode = 110, register = ---
                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0000000111100000;
                    Instr->Words[Instr->Header.NumberWords - 1] = 0x1234;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (zpc + f4240): mode = 110, register = ---
                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0000000111110000;
                    Instr->Words[Instr->Header.NumberWords - 1] = (M68K_WORD)0x000f;
                    Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // (pc + $f4240): mode = 111, register = 011
                    OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                    //             abbbcdd1efgg0hhh
                    *FullEWord = 0b0000000101110000;
                    Instr->Words[Instr->Header.NumberWords - 2] = (M68K_WORD)0x000f;
                    Instr->Words[Instr->Header.NumberWords - 1] = (M68K_WORD)0x4240;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // restore the number of words
                    Instr->Header.NumberWords -= 3;
                }

                if (!forAddressBank1 && (Instr->Header.Architectures & M68K_ARCH__P20304060__) != 0)
                {
                    // ([d32i, PC], Xn.SIZE * SCALE, d32o) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                    if ((AMode & AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_POST_INDEXED) != 0)
                    {
                        PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                        // ([pc - $2710] + a2.w * 2 + $f4240): mode = 111, register = 011
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b1010001100100111;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        // ([zpc + $f4240] + $f2441): mode = 110, register = ---
                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b0000000111110011;
                        Instr->Words[Instr->Header.NumberWords - 3] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords - 2] = (M68K_WORD)0x4240;
                        Instr->Words[Instr->Header.NumberWords - 1] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4241;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        // restore the number of words
                        Instr->Header.NumberWords -= 5;
                    }

                    // ([d32i, PC, Xn.SIZE * SCALE], d32o) where all elements are optional; also note that d32 can be 16-bit or 32-bit
                    if ((AMode & AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_PRE_INDEXED) != 0)
                    {
                        PM68K_WORD FullEWord = Instr->Words + (Instr->Header.NumberWords++);

                        // [[zpc + 0]]: mode = 111, register = 001
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);

                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b0000000111010001;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        // ([pc + a5 * 1 + $f4240] + $2710): mode = 111, register = 011
                        OrModeRegister(Word, InvMask, StartBit, Swap, 0b111011);
        
                        //             abbbcdd1efgg0hhh
                        *FullEWord = 0b1101100100110010;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x000f;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)0x4240;
                        Instr->Words[Instr->Header.NumberWords++] = (M68K_WORD)-10000;

                        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                            return M68K_FALSE;

                        // restore the number of words
                        Instr->Header.NumberWords -= 4;
                    }
                }
            }

            // (xxx).w
            if (!forAddressBank1 && (AMode & AMT_ABSOLUTE_SHORT) != 0)
            {
                // (1234).w: mode = 111, register = 000
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b111000);

                Instr->Words[Instr->Header.NumberWords++] = 0x1234;

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                // restore the number of words
                Instr->Header.NumberWords--;
            }

            // (xxx).l
            if (!forAddressBank1 && (AMode & AMT_ABSOLUTE_LONG) != 0)
            {
                // (12345678).w: mode = 111, register = 001
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b111001);

                Instr->Words[Instr->Header.NumberWords++] = 0x1234;
                Instr->Words[Instr->Header.NumberWords++] = 0x5678;

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                // restore the number of words
                Instr->Header.NumberWords -= 2;
            }

            // #xxx
            if ((!forAddressBank1 || VEA) && (AMode & (AMT_IMMEDIATE_DATA | AMT_IMMEDIATE_DATA_P20H)) != 0)
            {
                // #xxx: mode = 111, register = 100
                OrModeRegister(Word, InvMask, StartBit, Swap, 0b111100);

                if (VEA)
                {
                    if (!InitImmediate(ICtx, ConditionIndex, (ICtx->Instr.Words[0] & 0x0100) != 0 ? M68K_SIZE_W : M68K_SIZE_Q, Flags))
                        return M68K_FALSE;
                }
                else
                {
                    if (!InitImmediate(ICtx, ConditionIndex, Conditions->Size, Flags))
                        return M68K_FALSE;
                }
            }

            // clear the bits used by the effective address
            *Word &= InvMask;
        }
        break;

    case _CT_AMODE_SWAP:
        Swap = M68K_TRUE;
        VEA = M68K_FALSE;
        goto amode;

    case _CT_CREG:
        // iterate the value for all control registers
        for (PC_CONTROL_REGISTER NextCReg = _M68KControlRegisters; NextCReg->Value < 0xf000; NextCReg++)
        {
            // can we include the next value?
            if ((Instr->Header.Architectures & NextCReg->Architectures) != 0)
            {
                M68K_DWORD prevArch = Instr->Header.Architectures;

                (*Word) &= 0xf000;
                (*Word) |= NextCReg->Value;
                
                Instr->Header.Architectures &= NextCReg->Architectures;

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                Instr->Header.Architectures = prevArch;
            }
        }
        break;

    case _CT_FC:
        if ((Instr->Header.Architectures & (M68K_ARCH_68851 | M68K_ARCH_68030 | M68K_ARCH_68EC030)) == 0)
            return M68K_FALSE;

        // sfc
        *Word &= 0xffe0;
        *Word |= 0b00000;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // dfc
        *Word &= 0xffe0;
        *Word |= 0b00001;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // immediate: 7 (68030) or 14 (68851)
        *Word &= 0xffe0;
        
        if ((Instr->Header.Architectures & M68K_ARCH_68851) != 0)
        {
            *Word &= 0xffe0;
            *Word |= 0b11110;
        
            M68K_DWORD prevArch = Instr->Header.Architectures;
            Instr->Header.Architectures &= ~(M68K_ARCH_68030 | M68K_ARCH_68EC030);

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            Instr->Header.Architectures = prevArch;
        }
        
        if ((Instr->Header.Architectures & (M68K_ARCH_68030 | M68K_ARCH_68EC030)) != 0)
        {
            *Word &= 0xffe0;
            *Word |= 0b10111;

            M68K_DWORD prevArch = Instr->Header.Architectures;
            Instr->Header.Architectures &= ~M68K_ARCH_68851;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

            Instr->Header.Architectures = prevArch;
        }

        // d6
        *Word &= 0xffe0;
        *Word |= 0b01110;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;
        break;

    case _CT_FLAGS:
        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags | Condition->Info.Flags))
            return M68K_FALSE;
        break;

    case _CT_FPOP:
        {
            // iterate using all opmodes that are valid
            PC_INSTRUCTION_FPU_OPMODE_INFO NextFPO = _M68KInstrFPUOpmodes;
            PC_INSTRUCTION_FPU_OPMODE_INFO LastFPO = _M68KInstrFPUOpmodes + 128;

            M68K_DWORD prevArch = Instr->Header.Architectures;

            for (;NextFPO < LastFPO; NextFPO++)
            {
                // valid instruction?
                if (NextFPO->MasterType != M68K_IT_INVALID)
                {
                    // force the required architectures
                    Instr->Header.Architectures = prevArch & NextFPO->Architectures;

                    // set the opmode in the extensions word 1
                    *Word &= 0xff80;
                    *Word |= (M68K_WORD)(NextFPO - _M68KInstrFPUOpmodes);

                    // set the instruction type
                    Instr->Header.Type = NextFPO->MasterType;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;
                }
            }

            Instr->Header.Architectures = prevArch;
        }
        break;

    case _CT_FPSIZE:
        // note: fpu size is always specified in bits 10-12 of extension word 1
        {
            M68K_WORD Value = 0;
            for (M68K_UINT Index = 0; Index < 7; Index++)
            {
                // save the size
                *Word &= 0xe3ff;
                *Word |= Value;
                Conditions->Size = (M68K_SIZE)_M68KFpuSizes[Index];

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;

                // get the next value
                Value += 0x0400;
            }
        }

        // clear the size
        Conditions->Size = M68K_SIZE_NONE;
        break;

    case _CT_IMM:
        if (!InitImmediate(ICtx, ConditionIndex, Conditions->Size, Flags))
            return M68K_FALSE;
        break;

    case _CT_IMM16:
        if (!InitImmediate(ICtx, ConditionIndex, M68K_SIZE_W, Flags))
            return M68K_FALSE;
        break;

    case _CT_IMM32:
        if (!InitImmediate(ICtx, ConditionIndex, M68K_SIZE_L, Flags))
            return M68K_FALSE;
        break;

    case _CT_INTSIZE:
        // we need at least one size and the size can't be defined
        if (Condition->Info.IntSize.Mask == 0 && Conditions->Size != M68K_SIZE_NONE)
            return M68K_FALSE;

        // byte?
        if ((Condition->Info.IntSize.Allowed & 1) != 0)
        {
            *Word &= ~Condition->Info.IntSize.Mask;
            *Word |= Condition->Info.IntSize.OrMaskB;

            // set the size
            Conditions->Size = M68K_SIZE_B;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;
        }

        // word?
        if ((Condition->Info.IntSize.Allowed & 2) != 0)
        {
            *Word &= ~Condition->Info.IntSize.Mask;
            *Word |= Condition->Info.IntSize.OrMaskW;

            // set the size
            Conditions->Size = M68K_SIZE_W;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;
        }

        // long?
        if ((Condition->Info.IntSize.Allowed & 4) != 0)
        {
            *Word &= ~Condition->Info.IntSize.Mask;
            *Word |= Condition->Info.IntSize.OrMaskL;

            // set the size
            Conditions->Size = M68K_SIZE_L;

            if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;
        }

        // clear the size
        Conditions->Size = M68K_SIZE_NONE;
        break;

    case _CT_OFFSETWIDTH:
        // {15:32}
        *Word &= 0xf000;

        //         OoooooWwwwww
        *Word |= 0b001111000000;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;
        
        // {0:d5}
        *Word &= 0xf000;

        //         OoooooWwwwww
        *Word |= 0b000000100101;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

        // {d6:7}
        *Word &= 0xf000;

        //         OoooooWwwwww
        *Word |= 0b100110000111;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                return M68K_FALSE;

        // {d1:d2}
        *Word &= 0xf000;

        //         OoooooWwwwww
        *Word |= 0b100001100010;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        *Word &= 0xf000;
        break;

    case _CT_SIZE:
        Conditions->Size = Condition->Info.Size;

        if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
            return M68K_FALSE;

        // clear the size
        Conditions->Size = M68K_SIZE_NONE;
        break;

    case _CT_PREG:
        // note: p-register is always using bits 10-12 in extension word 1
        {
            for (M68K_UINT Index = 0; Index < 8; Index++)
            {
                Conditions->Size = _M68KMMUPRegisterSizes[Index];

                *Word &= ~0x1c00;
                *Word |= (M68K_WORD)(Index << 10);

                if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                    return M68K_FALSE;
            }

            // clear the size
            Conditions->Size = M68K_SIZE_NONE;
        }
        break;

    case _CT_VALUE:
    case _CT_VALUE_B1_S8E8EW1:
    case _CT_VALUE_VAMODE_A:
        {
            M68K_WORD Count = Condition->Info.Value.NumberValues;

            if (Count != 0)
            {
                M68K_WORD Value = Condition->Info.Value.StartValue;
                M68K_WORD Mask = Condition->Info.Value.Mask;
                InvMask = ~Mask;

                if ((Value & InvMask) != 0)
                    return M68K_FALSE;

                while (Count != 0)
                {
                    // add the value to the word
                    (*Word) &= InvMask;
                    (*Word) |= Value;

                    if (!IterateConditionsAndWriteOutput(ICtx, ConditionIndex + 1, Flags))
                        return M68K_FALSE;

                    // apply the increment?
                    if (--Count != 0)
                    {
                        // yes! update and make sure it's still valid
                        Value += Condition->Info.Value.Increment;
                        if ((Value & InvMask) != 0)
                            Value &= Mask;
                    }
                }

                // when going back we need to cancel the changes made by the current condition
                *Word &= InvMask;
            }
        }
        break;

    case _CT_VAMODE:
        Swap = M68K_FALSE;
        VEA = M68K_TRUE;
        goto amode;

    default:
        return M68K_FALSE;
    }

    return M68K_TRUE;
}

// check if a register is valid
static M68K_BOOL IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register)
{
    return (Register >= BaseRegister && Register < (BaseRegister + 8));
}

// save the mode and register in the opcode word
static void OrModeRegister(PM68K_WORD Word, M68K_WORD InvMask, M68K_UINT StartBit, M68K_BOOL Swap, M68K_WORD Value)
{
    // swap mode and register?
    if (Swap)
        Value = ((Value >> 3) & 7) | ((Value << 3) & 0x38);

    *Word &= InvMask;
    *Word |= (Value << StartBit);
}

// save the instruction in the output buffer;
// each instruction in the output has the following format
//
// [1]  M68K_WORD   SizeInWords (SizeInWords + Type + Architectures + Words)
// [1]  M68K_WORD   Type
// [2]  M68K_DWORD  Architectures
//      M68K_WORD   Words[N]
//
static M68K_BOOL SaveInstr(P_ITERATE_CTX ICtx, M68K_BOOL WithBankExtensionWord, M68K_WORD BankExtensionWord)
{
    P_INSTR Instr = &(ICtx->Instr);

    // write the bank extension word?
    if (WithBankExtensionWord)
    {
        // if bits A, B and C are all zero we won't generate the instruction
        if ((BankExtensionWord & 0x0e3f) == 0)
            WithBankExtensionWord = M68K_FALSE;
    }

    assert(!(
        Instr->Header.NumberWords > M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS ||
        (WithBankExtensionWord && Instr->Header.NumberWords == M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS)
    ));

    // SizeInWords
    if (!SaveWord(ICtx, (M68K_WORD)(4 + (WithBankExtensionWord ? 1 : 0) + Instr->Header.NumberWords)))
        return M68K_FALSE;

    // Type
    if (!SaveWord(ICtx, Instr->Header.Type))
        return M68K_FALSE;

    // Architectures
    if (!SaveWord(ICtx, (M68K_WORD)(Instr->Header.Architectures >> 16)))
        return M68K_FALSE;

    if (!SaveWord(ICtx, (M68K_WORD)Instr->Header.Architectures))
        return M68K_FALSE;

    // Words[N]
    if (WithBankExtensionWord)
        if (!SaveWord(ICtx, BankExtensionWord))
            return M68K_FALSE;

    for (M68K_WORD Index = 0; Index < Instr->Header.NumberWords; Index++)
        if (!SaveWord(ICtx, Instr->Words[Index]))
            return M68K_FALSE;

    ICtx->TotalCount++;
    return M68K_TRUE;
}

// save a word in the output buffer
static M68K_BOOL SaveWord(P_ITERATE_CTX ICtx, M68K_WORD Value)
{
    if (ICtx->MaximumSize == 0)
        return M68K_FALSE;

    // copy the value and update the available size
    M68KWriteWord((ICtx->Start)++, Value);
    ICtx->MaximumSize--;
    return M68K_TRUE;
}

// write the "auto-generated" message in the file
static void WriteAutoGenerated(FILE *File, PM68KC_STR Type)
{
    time_t CurrentTime;
    struct tm *TimeInfo;

    time(&CurrentTime);
    TimeInfo = localtime(&CurrentTime);

    fprintf(File, 
        "/*\n"
        "#### DO NOT CHANGE THIS FILE ####\n"
        "file: %s\n"
        "auto generated by M68KInternalGenerateAssemblerTables using the internal disassembler tables\n"
        "%04d-%02d-%02d %02d:%02d:%02d\n"
        "*/\n",
        Type,
        1900 + TimeInfo->tm_year,
        1 + TimeInfo->tm_mon,
        TimeInfo->tm_mday,
        TimeInfo->tm_hour,
        TimeInfo->tm_min,
        TimeInfo->tm_sec
    );
}

// generate the tables used to assemble instructions
void M68KInternalGenerateAssemblerTables(PM68KC_STR FileNameIndexFirstWord, PM68KC_STR FileNameOpcodeMaps, PM68KC_STR FileNameWords)
{
    if (FileNameIndexFirstWord != M68K_NULL && FileNameOpcodeMaps != M68K_NULL && FileNameWords != M68K_NULL)
    {
        FILE *FileIndexFirstWord = fopen(FileNameIndexFirstWord, "w");
        if (FileIndexFirstWord != M68K_NULL)
        {
            FILE *FileOpcodeMaps = fopen(FileNameOpcodeMaps, "w");
            if (FileOpcodeMaps != M68K_NULL)
            {
                FILE *FileWords = fopen(FileNameWords, "w");
                if (FileWords != M68K_NULL)
                {
                    WriteAutoGenerated(FileIndexFirstWord, "index_first_words.h");
                    WriteAutoGenerated(FileOpcodeMaps, "opcode_maps.h");
                    WriteAutoGenerated(FileWords, "words.h");

                    // analyse all instructions and save the information for all entries in the disassembler tables
                    M68K_WORD CurrentIndex = 0;

                    for (M68K_INSTRUCTION_TYPE_VALUE MType = M68K_IT_INVALID + 1; MType < M68K_IT__SIZEOF__; MType++)
                    {
                        // fpu opmode?
                        M68K_BOOL FPOP = ((_M68KInstrMasterTypeFlags[MType] & IMF_FPU_OPMODE) != 0);

                        // write the mnemonic as a comment
                        PM68KC_STR Mnemonic = _M68KTextMnemonics[MType];

                        fprintf(FileIndexFirstWord, "/* %s */\n", Mnemonic);
                        fprintf(FileOpcodeMaps, "/* %s */\n", Mnemonic);
                        fprintf(FileWords, "/* %s */\n", Mnemonic);

                        // write the index of the first word
                        fprintf(FileIndexFirstWord, "%u,\n", CurrentIndex);

                        // prepate the mask of opcode maps; 
                        // each bit in the mask represents a different opcode map i.e. bit N = uses opcode map N
                        M68K_WORD OpcodeMapMask = 0;
                        
                        for (M68K_UINT Index = 0; Index < 16; Index++)
                        {
                            M68K_WORD Mask;
                            M68K_WORD NumberOccurences = 0;
                            PM68KC_WORD NextWord = _M68KDisOpcodeMaps[Index];

                            while ((Mask = *(NextWord++)) != 0)
                            {
                                M68K_WORD Count = *(NextWord++);

                                // we can ignore the skip mask and count
                                NextWord += 2;

                                for (; Count != 0; Count--)
                                {
                                    // same master type?
                                    INSTRUCTION_TYPE_VALUE IType = (INSTRUCTION_TYPE_VALUE)NextWord[1];
                                    PC_INSTRUCTION_TYPE_INFO IInfo = _M68KInstrTypeInfos + IType;

                                    if (IInfo->MasterType == MType || 
                                        (FPOP && (IType == IT_FPOP || IType == IT_FPOP_REG)) ||
                                        ((IInfo->IFlags & IF_FMOVE_KFACTOR) != 0 && MType == M68K_IT_FMOVE))
                                    {
                                        // update the mask of opcode maps
                                        OpcodeMapMask |= ((M68K_WORD)1 << Index);

                                        // write a word with the index in the opcode map
                                        fprintf(FileWords, "/*%05u*/ 0x%04x,\n", CurrentIndex, (M68K_UINT)(M68K_INT)(NextWord - _M68KDisOpcodeMaps[Index]));
                                        NumberOccurences++;
                                        CurrentIndex++;
                                    }

                                    // skip all words used by the instruction in the disassembler table
                                    NextWord += NUM_WORDS_INSTR;
                                }
                            }

                            if (NumberOccurences != 0)
                            {
                                // if we have at least one instruction we must end the sequence with value 0; 
                                // we can use value 0 as a sentinel value because the index in the opcode map can never be 0 
                                // (at index 0 we have the mask that isn't required by the assembler)
                                fprintf(FileWords, "/*%05u*/ 0x%04x,\n", CurrentIndex, 0);
                                CurrentIndex++;
                            }
                        }

                        // write the mask of opcode maps
                        fprintf(FileOpcodeMaps, "0x%04x,\n", OpcodeMapMask);
                    }

                    fclose(FileWords);
                }

                fclose(FileOpcodeMaps);
            }

            fclose(FileIndexFirstWord);
        }
    }
}

// generate all opcodes; returns the number of instructions that were generated
M68K_UINT M68KInternalGenerateOpcodes(PM68K_WORD Start, M68K_UINT MaximumSize /*in words*/, M68K_DISASM_FLAGS Flags, PM68K_UINT UsedSize /*in words*/)
{
    _ITERATE_CTX ICtx;
    ICtx.Start = Start;
    ICtx.MaximumSize = MaximumSize;
    ICtx.TotalCount = 0;
    ICtx.Flags = Flags;

    if (ICtx.Start != M68K_NULL && MaximumSize != 0)
    {
        M68K_BOOL Error = M68K_FALSE;
        M68K_BOOL AllowCPX = ((Flags & M68K_DFLG_ALLOW_CPX) != 0);
        M68K_BOOL AllowFMoveKFactor = ((Flags & M68K_DFLG_ALLOW_FMOVE_KFACTOR) != 0);

        // generate the instructions for all opcode maps
        for (M68K_UINT Index = 0; !Error && Index < 16; Index++)
        {
            M68K_WORD Mask;
            PM68KC_WORD NextWord = _M68KDisOpcodeMaps[Index];

            while ((Mask = *(NextWord++)) != 0)
            {
                M68K_WORD Count = *(NextWord++);

                // we can ignore the skip mask and count
                NextWord += 2;

                for (; Count != 0; Count--)
                {
                    ICtx.Instr.Header.NumberWords = 1;
                    ICtx.Instr.Words[0] = *(NextWord++);

                    PC_INSTRUCTION_TYPE_INFO IInfo = _M68KInstrTypeInfos + *(NextWord++);

                    if ((AllowCPX || (IInfo->IFlags & IF_CPX) == 0) &&
                        (AllowFMoveKFactor || (IInfo->IFlags & IF_FMOVE_KFACTOR) == 0))
                    {
                        ICtx.Instr.Header.Type = (M68K_WORD)IInfo->MasterType;
                        ICtx.Instr.Header.Architectures = IInfo->Architectures;
                    
                        if ((IInfo->IFlags & IF_EXTENSION_WORD_1) != 0)
                        {
                            ICtx.Instr.Words[1] = 0;
                            ICtx.Instr.Header.NumberWords++;
                        }

                        if ((IInfo->IFlags & IF_EXTENSION_WORD_2) != 0)
                        {
                            ICtx.Instr.Words[2] = 0;
                            ICtx.Instr.Header.NumberWords++;
                        }

                        // use the actions to determine the conditions that will be iterated
                        ICtx.Conditions.Size = M68K_SIZE_NONE;
                        ICtx.Conditions.Count = 0;

                        M68K_UINT ActionIndex;
                        for (ActionIndex = 0; ActionIndex < (NUM_WORDS_INSTR - 2); ActionIndex++)
                            if (!AnalyseAction(&ICtx, (DISASM_ACTION_TYPE)*(NextWord++)))
                                break;

                        // success?
                        if (ActionIndex == (NUM_WORDS_INSTR - 2))
                        {
                            // copy the instruction to the output by iterating all conditions;
                            // after the last condition the instruction is written to the output buffer
                            if (!IterateConditionsAndWriteOutput(&ICtx, 0, 0))
                            {
                                Error = M68K_TRUE;
                                break;
                            }
                        }
                        else
                        {
                            Error = M68K_TRUE;
                            break;
                        }
                    }
                    else
                        NextWord += (NUM_WORDS_INSTR - 2);
                }
            }
        }
    }
    
    if (UsedSize != M68K_NULL)
        *UsedSize = (M68K_UINT)(ICtx.Start - Start);

    return ICtx.TotalCount;
}
#endif
