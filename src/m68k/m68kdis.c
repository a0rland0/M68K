#include "m68kinternal.h"

#define InitAModeOperandNoFlags(DCtx, AMode, ModeRegister) InitAModeOperand(DCtx, AMode, AMF_NONE, ModeRegister, 0)

typedef enum AMODE_FLAGS
{
    AMF_NONE                = 0,
    AMF_SWAP_MODE_REGISTER  = 1,
    AMF_BASE_BANK           = 2,    // DO NOT use with AMF_VEA
    AMF_VEA                 = 4,    // DO NOT use with AMF_BASE_BANK
} AMODE_FLAGS, *PAMODE_FLAGS;

// forward declarations
static PM68K_WORD                   Disassemble(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction, M68K_INSTRUCTION_TYPE ForcedInstructionType /*M68K_IT_INVALID = ignored*/, PM68K_VALUE_LOCATION Values /*M68K_NULL = values are ignored*/, M68K_VALUE_OPERANDS_MASK OperandsMask /*0 = values are ignored*/, M68K_UINT MaximumNumberValues /*0 = values are ignored*/, PM68K_UINT TotalNumberValues /*can be M68K_NULL*/, PM68K_WORD FirstWord /*can be M68K_NULL*/);
static M68K_BOOL                    ExecuteAction(PDISASM_CTX DCtx, DISASM_ACTION_TYPE ActionType, PM68K_BOOL Stop);
static M68K_BOOL                    ExtendOperandWithBankBits(PM68K_OPERAND Operand, M68K_WORD Bits, PM68K_WORD FinalBits);
static PM68K_OPERAND                GetNextOperand(PDISASM_CTX DCtx);
static M68K_REGISTER_TYPE_VALUE     GetVRegister(M68K_BYTE Value, M68K_BYTE XBit);
static M68K_BOOL                    InitAModeOperand(PDISASM_CTX DCtx, ADDRESSING_MODE_TYPE_VALUE AMode, AMODE_FLAGS Flags, M68K_BYTE ModeRegister, M68K_BYTE BankExtension);
static void                         InitAModePDOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE BaseReg, M68K_BYTE Value);
static void                         InitAModePIOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE BaseReg, M68K_BYTE Value);
static void                         InitRegOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE_VALUE BaseReg, M68K_BYTE Value);
static void                         InitVRegOperand(PDISASM_CTX DCtx, M68K_BYTE Value, M68K_BYTE XBit);
static M68K_BOOL                    IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD Bits);
static M68K_BOOL                    SavePCOffset(PDISASM_CTX DCtx);
static void                         SaveValue(PDISASM_CTX DCtx, PM68K_OPERAND Operand, M68K_VALUE_TYPE ValueType, M68K_UINT InputIndex, M68K_BOOL InputIsByte);

static PM68K_WORD Disassemble(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction, M68K_INSTRUCTION_TYPE ForcedInstructionType /*M68K_IT_INVALID = ignored*/, PM68K_VALUE_LOCATION Values /*M68K_NULL = values are ignored*/, M68K_VALUE_OPERANDS_MASK OperandsMask /*0 = values are ignored*/, M68K_UINT MaximumNumberValues /*0 = values are ignored*/, PM68K_UINT TotalNumberValues /*can be M68K_NULL*/, PM68K_WORD FirstWord /*can be M68K_NULL*/)
{
    PM68K_WORD Next = M68K_NULL;

    if (Start != M68K_NULL && End != M68K_NULL)
    {
        // init
        DISASM_CTX DCtx;

        DCtx.Address = Address;
        DCtx.Architectures = Architectures;
        DCtx.Flags = Flags;
        DCtx.Input.Start = Start;
        DCtx.Input.End = End;
        DCtx.Instruction = Instruction;
        DCtx.Values.NextValue = Values;
        DCtx.Values.OperandsMask = OperandsMask;
        DCtx.Values.MaximumNumberValues = MaximumNumberValues;
        DCtx.Values.TotalNumberValues = 0;

        if (Values != M68K_NULL && ((OperandsMask & (((M68K_WORD)1 << M68K_MAXIMUM_NUMBER_OPERANDS) - 1)) == 0 || MaximumNumberValues == 0 || TotalNumberValues == 0))
            DCtx.Values.NextValue = M68K_NULL;

        // get the first word
        DCtx.Opcodes.Word = _M68KDisReadWord(&(DCtx.Input), 0);
        if (FirstWord != M68K_NULL)
            *FirstWord = DCtx.Opcodes.Word;

        M68K_BOOL AllowCPX = ((Flags & M68K_DFLG_ALLOW_CPX) != 0);
        M68K_BOOL AllowFMoveKFactor = ((Flags & M68K_DFLG_ALLOW_FMOVE_KFACTOR) != 0);

        // identify the opcode map
        M68K_WORD Mask;
        PM68KC_WORD NextWord = _M68KDisOpcodeMaps[DCtx.Opcodes.Word >> 12];

        // analyse all instruction until we find one that macthes
        while ((Mask = *(NextWord++)) != 0)
        {
            // get the number of instructions
            M68K_WORD NumberInstructions = *(NextWord++);

            // can we skip the whole block?
            M68K_WORD SkipMask = *(NextWord++);
            M68K_WORD SkipValue = *(NextWord++);

            if ((DCtx.Opcodes.Word & SkipMask) == SkipValue)
            {
                // try all instruction with the same mask
                for (; NumberInstructions != 0; NumberInstructions--)
                {
                    // value matches?
                    if ((DCtx.Opcodes.Word & Mask) == *NextWord)
                    {
                        // prepare the context for a new instruction
                        DCtx.IType = (INSTRUCTION_TYPE)NextWord[1];
                        DCtx.IInfo = _M68KInstrTypeInfos + DCtx.IType;
                        DCtx.MasterType = DCtx.IInfo->MasterType;
                        DCtx.DFlags = 0;
                        DCtx.Opcodes.NumberWords = 1;
                        DCtx.Opcodes.PCOffset = 0;
                        DCtx.NextOperand = DCtx.Instruction->Operands;
                        DCtx.UsesPredecrement = M68K_FALSE;
                        DCtx.IndexOperandRegisterList = M68K_MAXIMUM_NUMBER_OPERANDS;

                        // architecture is valid?
                        if ((DCtx.Architectures & DCtx.IInfo->Architectures) != 0)
                        {
                            // flags allow it?
                            if ((AllowCPX || (DCtx.IInfo->IFlags & IF_CPX) == 0) &&
                                (AllowFMoveKFactor || (DCtx.IInfo->IFlags & IF_FMOVE_KFACTOR) == 0))
                            {
                                // read the extension words?
                                if ((DCtx.IInfo->IFlags & IF_EXTENSION_WORD_1) != 0)
                                {
                                    // word was already read?
                                    if ((DCtx.DFlags & DCF_EXTENSION_WORD_1) == 0)
                                    {
                                        DCtx.DFlags |= DCF_EXTENSION_WORD_1;
                                        DCtx.Opcodes.EWord1 = _M68KDisReadWord(&(DCtx.Input), 1);
                                        DCtx.Opcodes.NumberWords++;
                                    }
                                }
                    
                                if ((DCtx.IInfo->IFlags & IF_EXTENSION_WORD_2) != 0)
                                {
                                    // word was already read?
                                    if ((DCtx.DFlags & DCF_EXTENSION_WORD_2) == 0)
                                    {
                                        DCtx.DFlags |= DCF_EXTENSION_WORD_2;
                                        DCtx.Opcodes.EWord2 = _M68KDisReadWord(&(DCtx.Input), 2);
                                        DCtx.Opcodes.NumberWords++;
                                    }
                                }
                    
                                // init
                                DCtx.Instruction->Size = M68K_SIZE_NONE;

                                // execute all actions; we'll stop when we have an error OR we reached the maximum number of actions (success)
                                M68K_UINT ActionIndex;
                                M68K_BOOL Stop = M68K_FALSE;

                                for (ActionIndex = 2; ActionIndex < NUM_WORDS_INSTR; ActionIndex++)
                                    if (!ExecuteAction(&DCtx, (DISASM_ACTION_TYPE)NextWord[ActionIndex], &Stop))
                                        break;

                                // success?
                                if (ActionIndex == NUM_WORDS_INSTR || Stop)
                                {
                                    // use the forced instruction type?
                                    if (ForcedInstructionType == M68K_IT_INVALID || ForcedInstructionType == DCtx.MasterType)
                                    {
                                        // the remaining operands must be set to "none"
                                        for (PM68K_OPERAND LastOperand = DCtx.Instruction->Operands + M68K_MAXIMUM_NUMBER_OPERANDS; DCtx.NextOperand < LastOperand; DCtx.NextOperand++)
                                            DCtx.NextOperand->Type = M68K_OT_NONE;

                                        // one of the operands is a list of registers that needs to be inverted?
                                        if (DCtx.UsesPredecrement && DCtx.IndexOperandRegisterList < M68K_MAXIMUM_NUMBER_OPERANDS)
                                            Instruction->Operands[DCtx.IndexOperandRegisterList].Info.RegisterList = _M68KDisInvertWordBits(Instruction->Operands[DCtx.IndexOperandRegisterList].Info.RegisterList);

                                        // fill the remaining fields in the instruction
                                        DCtx.Instruction->Start = Start;
                                        DCtx.Instruction->End = Start + DCtx.Opcodes.NumberWords;
                                        DCtx.Instruction->Stop = (DCtx.Instruction->End < End ? DCtx.Instruction->End : End);
                                        DCtx.Instruction->PCOffset = DCtx.Opcodes.PCOffset;
                                        DCtx.Instruction->Type = DCtx.MasterType;
                                        DCtx.Instruction->InternalType = (M68K_INTERNAL_INSTRUCTION_TYPE)DCtx.IType;
                                        Next = DCtx.Instruction->End;

                                        if (TotalNumberValues != M68K_NULL)
                                            *TotalNumberValues = DCtx.Values.TotalNumberValues;

                                        break;
                                    }
                                }
                            }
                        }
                    }

                    // skip the instruction
                    NextWord += NUM_WORDS_INSTR;
                }

                // stop?
                if (NumberInstructions != 0)
                    break;
            }
            else
                NextWord += ((M68K_LUINT)NumberInstructions * NUM_WORDS_INSTR);
        }
    }
    else if (FirstWord != M68K_NULL)
        *FirstWord = 0;

    return Next;
}

// execute an action
static M68K_BOOL ExecuteAction(PDISASM_CTX DCtx, DISASM_ACTION_TYPE ActionType, PM68K_BOOL Stop)
{
    M68K_DWORD Value;
    M68K_INT InputIndex;
    PM68K_OPERAND Operand;
    M68K_REGISTER_TYPE Register;
    M68K_OPERAND_TYPE_VALUE AddressOperandType;

    M68K_BOOL Result = M68K_FALSE;

    // check a value and mask?
    if (ActionType >= DAT_C__VMSTART__ && ActionType < DAT_C__VMEND__)
    {
        // get the value and mask
        PC_VALUE_MASK ValueMask = _M68KDisValueMasks + ((M68K_LUINT)ActionType - (M68K_LUINT)DAT_C__VMSTART__);
        M68K_WORD CheckValue = (ActionType >= DAT_C__VMEW2START__ ? DCtx->Opcodes.EWord2 : DCtx->Opcodes.EWord1);
        Result = (M68K_BOOL)((CheckValue & ValueMask->Mask) == ValueMask->Value);
    }
    else
    {
        switch (ActionType)
        {
        case DAT_NONE:
            // note: Result must be M68K_FALSE to stop executing the actions
            *Stop = M68K_TRUE;
            break;

        case DAT_F_AMode1B:
            DCtx->DFlags |= DCF_AMODE1B;
            Result = M68K_TRUE;
            break;

        case DAT_O_AbsL_NW:
            Result = InitAModeOperandNoFlags(DCtx, AMT_ABSOLUTE_LONG, 0b111001);
            break;

        case DAT_O_AC80AMode_S0E5W0_S8E8EW1:
            Result = InitAModeOperand(DCtx, AMT__ALL__, AMF_BASE_BANK, (M68K_BYTE)DCtx->Opcodes.Word, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 8));
            break;

        case DAT_O_AC80AModeA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__AC80_ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AC80AModeA_S0E5W0_S8E8EW1:
            Result = InitAModeOperand(DCtx, AMT__AC80_ALTERABLE__, AMF_BASE_BANK, (M68K_BYTE)DCtx->Opcodes.Word, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 8));
            break;

        case DAT_O_AC80AReg_S12E14EW1_S7E7EW1:
            InitRegOperand(DCtx, ((DCtx->Opcodes.EWord1 & 0x80) != 0 ? M68K_RT_B0 : M68K_RT_A0), (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12));
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80BankEWSize_S6E7W0:
            Value = (M68K_BYTE)(4 + ((DCtx->Opcodes.Word >> 5) & 6));
            InputIndex = -1;

immediateB:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            if (InputIndex >= 0)
                SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_B, (M68K_UINT)InputIndex, M68K_TRUE);

            Operand->Type = M68K_OT_IMMEDIATE_B;
            Operand->Info.Byte = (M68K_BYTE)Value;

            Result = M68K_TRUE;
            break;

        case DAT_O_AC80BReg_S0E2W0:
            InitRegOperand(DCtx, M68K_RT_B0, (M68K_BYTE)DCtx->Opcodes.Word);
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80BReg_S9E11W0:
            InitRegOperand(DCtx, M68K_RT_B0, (M68K_BYTE)(DCtx->Opcodes.Word >> 9));
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80DReg_S12E14EW1_S7E7EW1:
            // the data register cannot be banked with one bit
            if ((DCtx->Opcodes.EWord1 & 0x80) == 0)
            {
                InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12));
                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AC80VAMode_S0E5W0_S8E8W0:
            Result = InitAModeOperand(DCtx, AMT__AC80_ALL__, AMF_VEA, (M68K_BYTE)DCtx->Opcodes.Word, (M68K_BYTE)(DCtx->Opcodes.Word >> 8));
            break;

        case DAT_O_AC80VAModeA_S0E5W0_S8E8W0:
            Result = InitAModeOperand(DCtx, AMT__AC80_ALTERABLE__, AMF_VEA, (M68K_BYTE)DCtx->Opcodes.Word, (M68K_BYTE)(DCtx->Opcodes.Word >> 8));
            break;

        case DAT_O_AC80VReg_S0E3EW1_S8E8W0:
            InitVRegOperand(DCtx, (M68K_BYTE)DCtx->Opcodes.EWord1, (M68K_BYTE)(DCtx->Opcodes.Word >> 8));
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80VReg_S12E15EW1_S7E7W0:
            InitVRegOperand(DCtx, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12), (M68K_BYTE)(DCtx->Opcodes.Word >> 7));
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80VReg_S8E11EW1_S6E6W0:
            InitVRegOperand(DCtx, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 8), (M68K_BYTE)(DCtx->Opcodes.Word >> 6));
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80VRegPM2_S8E11EW1_S6E6W0:
            if ((DCtx->Opcodes.EWord1 & 0x0100) == 0)
            {
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;

                Operand->Type = M68K_OT_VREGISTER_PAIR_M2;
                Operand->Info.RegisterPair.Register1 = GetVRegister((M68K_BYTE)(DCtx->Opcodes.EWord1 >> 8), (M68K_BYTE)(DCtx->Opcodes.Word >> 6));
                Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 1);

                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AC80VRegPM4_S0E3W0_S8E8W0:
            if ((DCtx->Opcodes.Word & 0x0003) == 0)
            {
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;

                Operand->Type = M68K_OT_VREGISTER_PAIR_M4;
                Operand->Info.RegisterPair.Register1 = GetVRegister((M68K_BYTE)DCtx->Opcodes.Word, (M68K_BYTE)(DCtx->Opcodes.Word >> 8));
                Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 3);

                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AMode_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__ALL__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeC_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL_ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCAPD_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCAPDPI_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCPD_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCPDPI_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeCPI_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeD_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__DATA__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeDA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__DATA_ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeDA_S6E11W0:
            Result = InitAModeOperand(DCtx, AMT__DATA_ALTERABLE__, AMF_SWAP_MODE_REGISTER, (M68K_BYTE)(DCtx->Opcodes.Word >> 6), 0);
            break;

        case DAT_O_AModeDAM_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__DATA_ALTERABLE_MEMORY__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeDAPC_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, ((DCtx->Architectures & M68K_ARCH__P20H__) != 0 ? AMT__DATA_ALTERABLE_PC__ : AMT__DATA_ALTERABLE__), (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeDRDCA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__DATA_REGISTER_DIRECT_CONTROL_ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeIA_S0E2W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT_ADDRESS_REGISTER_INDIRECT, (M68K_BYTE)(0b010000 | (DCtx->Opcodes.Word & 7)));
            break;

        case DAT_O_AModeIAD_S0E2W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT, (M68K_BYTE)(0b101000 | (DCtx->Opcodes.Word & 7)));
            break;

        case DAT_O_AModeMA_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__MEMORY_ALTERABLE__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModePD_S0E2W0:
            InitAModePDOperand(DCtx, M68K_RT_A0, (M68K_BYTE)DCtx->Opcodes.Word);
            Result = M68K_TRUE;
            break;

        case DAT_O_AModePD_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModePD_S9E11W0:
            InitAModePDOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.Word >> 9));
            Result = M68K_TRUE;
            break;

        case DAT_O_AModePFR_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__PFR__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModePI_S0E2W0:
            InitAModePIOperand(DCtx, M68K_RT_A0, (M68K_BYTE)DCtx->Opcodes.Word);
            Result = M68K_TRUE;
            break;

        case DAT_O_AModePI_S12E14W1:
            InitAModePIOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12));
            Result = M68K_TRUE;
            break;

        case DAT_O_AModePI_S9E11W0:
            InitAModePIOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.Word >> 9));
            Result = M68K_TRUE;
            break;

        case DAT_O_AModeTOUCH_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__TOUCH__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AModeTST_S0E5W0:
            Result = InitAModeOperandNoFlags(DCtx, AMT__TST__, (M68K_BYTE)DCtx->Opcodes.Word);
            break;

        case DAT_O_AReg_S12E14EW1:
            InitRegOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12));
            Result = M68K_TRUE;
            break;

        case DAT_O_AReg_S5E7EW1:
            InitRegOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 5));
            Result = M68K_TRUE;
            break;

        case DAT_O_AReg_S0E2W0:
            InitRegOperand(DCtx, M68K_RT_A0, (M68K_BYTE)DCtx->Opcodes.Word);
            Result = M68K_TRUE;
            break;

        case DAT_O_AReg_S0E2EW1:
            InitRegOperand(DCtx, M68K_RT_A0, (M68K_BYTE)DCtx->Opcodes.EWord1);
            Result = M68K_TRUE;
            break;

        case DAT_O_AReg_S9E11W0:
            InitRegOperand(DCtx, M68K_RT_A0, (M68K_BYTE)(DCtx->Opcodes.Word >> 9));
            Result = M68K_TRUE;
            break;

        case DAT_O_BACReg_S2E4EW1:
            InitRegOperand(DCtx, M68K_RT_BAC0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 2));
            Result = M68K_TRUE;
            break;

        case DAT_O_BADReg_S2E4EW1:
            InitRegOperand(DCtx, M68K_RT_BAD0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 2));
            Result = M68K_TRUE;
            break;

        case DAT_O_CC_S8E11W0:
        case DAT_O_CC2H_S8E11W0:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
        
            Operand->Type = M68K_OT_CONDITION_CODE;
            Operand->Info.ConditionCode = (M68K_CONDITION_CODE_VALUE)((DCtx->Opcodes.Word >> 8) & (M68K_CC__SIZEOF__ - 1));

            // must be >= 2?
            Result = (ActionType == DAT_O_CC_S8E11W0 || Operand->Info.ConditionCode >= 2);
            break;

        case DAT_O_CPID_S9E11W0:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
        
            Operand->Type = M68K_OT_COPROCESSOR_ID;
            Operand->Info.CoprocessorId = (M68K_COPROCESSOR_ID_VALUE)((DCtx->Opcodes.Word >> 9) & 7);
        
            Result = M68K_TRUE;
            break;

        case DAT_O_CPIDCC_S9E11W0_S0E5W0:
        case DAT_O_CPIDCC_S9E11W0_S0E5EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_COPROCESSOR_ID_CONDITION_CODE;
            Operand->Info.CoprocessorIdCC.Id = (M68K_COPROCESSOR_ID_VALUE)((DCtx->Opcodes.Word >> 9) & 7);
        
            if (ActionType == DAT_O_CPIDCC_S9E11W0_S0E5W0)
                Operand->Info.CoprocessorIdCC.CC = (M68K_COPROCESSOR_CONDITION_CODE_VALUE)(DCtx->Opcodes.Word & 0x3f);
            else
                Operand->Info.CoprocessorIdCC.CC = (M68K_COPROCESSOR_CONDITION_CODE_VALUE)(DCtx->Opcodes.EWord1 & 0x3f);
        
            Result = M68K_TRUE;
            break;

        case DAT_O_CReg_S0E11EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            // register is allowed?
            M68K_WORD CRegValue = (DCtx->Opcodes.EWord1 & 0x0fff);
        
            for (PC_CONTROL_REGISTER NextCReg = _M68KControlRegisters; NextCReg->Value < 0xf000; NextCReg++)
            {
                // found the same value?
                if (NextCReg->Value == CRegValue)
                {
                    // the architecture allows it?
                    if ((DCtx->Architectures & NextCReg->Architectures) != 0)
                    {
                        Operand->Type = M68K_OT_REGISTER;
                        Operand->Info.Register = NextCReg->Register;
                        Result = M68K_TRUE;
                        break;
                    }
                }
                else if (NextCReg->Value > CRegValue)
                    break;
            }
            break;

        case DAT_O_CT_S6E7W0:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            Operand->Type = M68K_OT_CACHE_TYPE;
            Operand->Info.CacheType = (M68K_CACHE_TYPE_VALUE)((DCtx->Opcodes.Word >> 6) & 3);
            Result = M68K_TRUE;
            break;

        case DAT_O_DispB_S0E7W0:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            SaveValue(DCtx, Operand, M68K_VT_DISPLACEMENT_B, 0, M68K_TRUE);

            AddressOperandType = M68K_OT_ADDRESS_B;
            Operand->Type = M68K_OT_DISPLACEMENT_B;
            Operand->Info.Displacement = (M68K_SDWORD)(M68K_SBYTE)(M68K_BYTE)DCtx->Opcodes.Word;

save_disp_base:
            // offset was already defined?
            if (DCtx->Opcodes.PCOffset != 0)
                break;
            
            DCtx->Opcodes.PCOffset = _M68KDisGetImplicitPCOffset(DCtx->MasterType);
            
            // displacement can use a special operand?
            if ((DCtx->Flags & M68K_DFLG_ALLOW_ADDRESS_OPERAND) != 0)
            {
                Operand->Type = AddressOperandType;
                Operand->Info.Displacement += (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DCtx->Address + (M68K_SLWORD)(M68K_SWORD)DCtx->Opcodes.PCOffset);
            }
            
            Result = M68K_TRUE;
            break;

        case DAT_O_DispL_NW:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            SaveValue(DCtx, Operand, M68K_VT_DISPLACEMENT_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

            AddressOperandType = M68K_OT_ADDRESS_L;
            Operand->Type = M68K_OT_DISPLACEMENT_L;
            Operand->Info.Displacement = (M68K_SDWORD)_M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
            DCtx->Opcodes.NumberWords += 2;
            goto save_disp_base;

        case DAT_O_DispW_NW:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            SaveValue(DCtx, Operand, M68K_VT_DISPLACEMENT_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

            AddressOperandType = M68K_OT_ADDRESS_W;
            Operand->Type = M68K_OT_DISPLACEMENT_W;
            Operand->Info.Displacement = (M68K_SDWORD)(M68K_SWORD)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
            goto save_disp_base;

        case DAT_O_DKFac_S4E6EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            Operand->Type = M68K_OT_DYNAMIC_KFACTOR;
            Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + ((DCtx->Opcodes.EWord1 >> 4) & 7));
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S0E2W0:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)DCtx->Opcodes.Word);
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S0E2EW1:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)DCtx->Opcodes.EWord1);
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S12E14EW1:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 12));
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S4E6EW1:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 4));
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S6E8EW1:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 6));
            Result = M68K_TRUE;
            break;

        case DAT_O_DReg_S9E11W0:
            InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)(DCtx->Opcodes.Word >> 9));
            Result = M68K_TRUE;
            break;

        case DAT_O_DRegP_S0E2EW1_S12E14EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_PAIR;
            Operand->Info.RegisterPair.Register1 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + (DCtx->Opcodes.EWord1 & 7));
            Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + ((DCtx->Opcodes.EWord1 >> 12) & 7));

            Result = M68K_TRUE;
            break;

        case DAT_O_DRegP_S0E2W0_S0E2EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_PAIR;
            Operand->Info.RegisterPair.Register1 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + (DCtx->Opcodes.Word & 7));
            Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + (DCtx->Opcodes.EWord1 & 7));

            Result = M68K_TRUE;
            break;

        case DAT_O_DRegP_S0E2EW1_S0E2EW2:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_PAIR;
            Operand->Info.RegisterPair.Register1 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + (DCtx->Opcodes.EWord1 & 7));
            Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + (DCtx->Opcodes.EWord2 & 7));

            Result = M68K_TRUE;
            break;

        case DAT_O_DRegP_S6E8EW1_S6E8EW2:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_PAIR;
            Operand->Info.RegisterPair.Register1 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + ((DCtx->Opcodes.EWord1 >> 6) & 7));
            Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_D0 + ((DCtx->Opcodes.EWord2 >> 6) & 7));

            Result = M68K_TRUE;
            break;

        case DAT_O_FC_S0E4EW1:
            Value = (M68K_DWORD)(DCtx->Opcodes.EWord1 & 0x1f);
            
            // 00000 = sfc?
            if (Value == 0)
            {
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;

                Operand->Type = M68K_OT_REGISTER;
                Operand->Info.Register = M68K_RT_SFC;
            }
            // 00001 = dfc?
            else if (Value == 1)
            {
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;

                Operand->Type = M68K_OT_REGISTER;
                Operand->Info.Register = M68K_RT_DFC;
            }
            // 10ddd (68030/68EC030) or 1dddd (68851) = immediate
            else if ((Value & 0x10) != 0)
            {
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;
                
                Operand->Type = M68K_OT_IMMEDIATE_B;
                Operand->Info.Byte = (M68K_BYTE)(Value & ((DCtx->Architectures & M68K_ARCH_68851) != 0 ? 15 : 7));
            }
            // 01rrr = data register
            else if ((Value & 0x18) == 8)
                InitRegOperand(DCtx, M68K_RT_D0, (M68K_BYTE)Value);
            else
                break;
            
            Result = M68K_TRUE;
            break;

        case DAT_O_FPCC_S0E4W0:
        case DAT_O_FPCC_S0E4EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
        
            Operand->Type = M68K_OT_FPU_CONDITION_CODE;
            Operand->Info.FpuConditionCode = (M68K_FPU_CONDITION_CODE_VALUE)((ActionType == DAT_O_FPCC_S0E4W0 ? DCtx->Opcodes.Word : DCtx->Opcodes.EWord1) & (M68K_FPCC__SIZEOF__ - 1));
            Result = M68K_TRUE;
            break;

        case DAT_O_FPCRRegList_S10E12EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
        
            Operand->Type = M68K_OT_REGISTER_FPCR_LIST;
            Operand->Info.RegisterList = 0;

            // fpcr?
            if ((DCtx->Opcodes.EWord1 & 0x1000) != 0)
                Operand->Info.RegisterList |= 0b001;

            // fpsr?
            if ((DCtx->Opcodes.EWord1 & 0x0800) != 0)
                Operand->Info.RegisterList |= 0b010;

            // fpiar?
            if ((DCtx->Opcodes.EWord1 & 0x0400) != 0)
                Operand->Info.RegisterList |= 0b100;

            // at least one bit must be set
            Result = (Operand->Info.RegisterList != 0);
            break;

        case DAT_FPOP_S0E7EW1:
            {
                PC_INSTRUCTION_FPU_OPMODE_INFO FPOInfo = _M68KInstrFPUOpmodes + (DCtx->Opcodes.EWord1 & 0x7f);
                if (FPOInfo->MasterType != M68K_IT_INVALID)
                {
                    // architecture is supported?
                    if ((DCtx->Architectures & FPOInfo->Architectures) != 0)
                    {
                        // save the the master instruction type
                        DCtx->MasterType = FPOInfo->MasterType;
                        Result = M68K_TRUE;
                    }
                }
            }
            break;

        case DAT_O_FPReg_S0E2EW1:
            InitRegOperand(DCtx, M68K_RT_FP0, (M68K_BYTE)DCtx->Opcodes.EWord1);
            Result = M68K_TRUE;
            break;

        case DAT_O_FPReg_S7E9EW1:
            InitRegOperand(DCtx, M68K_RT_FP0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 7));
            Result = M68K_TRUE;
            break;

        case DAT_O_FPReg_S10E12EW1:
            InitRegOperand(DCtx, M68K_RT_FP0, (M68K_BYTE)(DCtx->Opcodes.EWord1 >> 10));
            Result = M68K_TRUE;
            break;

        case DAT_O_FPRegIList_S0E7EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            // note: we don't need to use IndexOperandRegisterList because the list is inverted before it's saved in the operand
            Operand->Type = M68K_OT_REGISTER_FP_LIST;
            Operand->Info.RegisterList = (M68K_REGISTER_LIST)(_M68KDisInvertWordBits(DCtx->Opcodes.EWord1) >> 8);
            Result = M68K_TRUE;
            break;

        case DAT_O_FPRegList_S0E7EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_FP_LIST;
            Operand->Info.RegisterList = (M68K_REGISTER_LIST)(M68K_BYTE)DCtx->Opcodes.EWord1;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB0_I:
            Value = 0;
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S0E1W0:
            Value = (M68K_BYTE)(DCtx->Opcodes.Word & 3);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S0E2W0:
            Value = (M68K_BYTE)(DCtx->Opcodes.Word & 7);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S0E3W0:
            Value = (M68K_BYTE)(DCtx->Opcodes.Word & 15);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S0E6EW1:
            Value = (M68K_BYTE)(DCtx->Opcodes.EWord1 & 0x7f);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S0E7W0:
            Value = (M68K_BYTE)DCtx->Opcodes.Word;
            InputIndex = 0;
            goto immediateB;

        case DAT_O_ImmB_S0E7EW1:
            Value = (M68K_BYTE)DCtx->Opcodes.EWord1;
            InputIndex = 1;
            goto immediateB;

        case DAT_O_ImmB_S10E12EW1:
            Value = (M68K_BYTE)((DCtx->Opcodes.EWord1 >> 10) & 7);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S2E3W0:
            Value = (M68K_BYTE)((DCtx->Opcodes.Word >> 2) & 3);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S8E9EW1:
            Value = ((DCtx->Opcodes.EWord1 >> 8) & 3);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S9E11W0:
            Value = ((DCtx->Opcodes.Word >> 9) & 7);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmB_S9E11W0_S4E5W0:
            Value = ((DCtx->Opcodes.Word & 0x30) >> 1) | ((DCtx->Opcodes.Word >> 9) & 7);
            InputIndex = -1;
            goto immediateB;

        case DAT_O_ImmL_NW:
            goto immediateL;

        case DAT_O_ImmS:
            switch ((M68K_SIZE)DCtx->Instruction->Size)
            {
            case M68K_SIZE_B:
                InputIndex = (M68K_INT)DCtx->Opcodes.NumberWords;
                Value = _M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
                goto immediateB;

            case M68K_SIZE_L:
immediateL:
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;
            
                SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

                Operand->Type = M68K_OT_IMMEDIATE_L;
                Operand->Info.Long = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                DCtx->Opcodes.NumberWords += 2;

                Result = M68K_TRUE;
                break;

            case M68K_SIZE_W:
next_immediateW:
                InputIndex = (M68K_INT)DCtx->Opcodes.NumberWords;
                Value = _M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);

immediateW:
                if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                    break;
            
                if (InputIndex >= 0)
                    SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_W, InputIndex, M68K_FALSE);

                Operand->Type = M68K_OT_IMMEDIATE_W;
                Operand->Info.Word = (M68K_WORD)Value;

                Result = M68K_TRUE;
                break;

            default:
                break;
            }
            break;

        case DAT_O_ImmW_S0E11EW1:
            Value = (DCtx->Opcodes.EWord1 & 0xfff);
            InputIndex = -1;
            goto immediateW;

        case DAT_O_ImmW_NW:
            goto next_immediateW;

        case DAT_O_MMUCC_S0E3W0:
        case DAT_O_MMUCC_S0E3EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
        
            Operand->Type = M68K_OT_MMU_CONDITION_CODE;
            Operand->Info.MMUConditionCode = (M68K_MMU_CONDITION_CODE_VALUE)((ActionType == DAT_O_MMUCC_S0E3W0 ? DCtx->Opcodes.Word : DCtx->Opcodes.EWord1) & (M68K_MMUCC__SIZEOF__ - 1));
            Result = M68K_TRUE;
            break;

        case DAT_O_MMUMask_S5E8EW1:
            if ((DCtx->Architectures & (M68K_ARCH_68030 | M68K_ARCH_68851)) == 0)
                break;

            Value = (M68K_BYTE)((DCtx->Opcodes.EWord1 >> 5) & ((DCtx->Architectures & M68K_ARCH_68851) != 0 ? 15 : 7));
            InputIndex = -1;
            goto immediateB;

        case DAT_O_MRegP_S12E15EW1_S12E15EW2:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_MEM_REGISTER_PAIR;
            Operand->Info.RegisterPair.Register1 = (M68K_REGISTER_TYPE_VALUE)(((DCtx->Opcodes.EWord1 & 0x8000) != 0 ? M68K_RT_A0 : M68K_RT_D0) + ((DCtx->Opcodes.EWord1 >> 12) & 7));
            Operand->Info.RegisterPair.Register2 = (M68K_REGISTER_TYPE_VALUE)(((DCtx->Opcodes.EWord2 & 0x8000) != 0 ? M68K_RT_A0 : M68K_RT_D0) + ((DCtx->Opcodes.EWord2 >> 12) & 7));

            Result = M68K_TRUE;
            break;

        case DAT_O_OW_S0E11EW1:
            // always applied to the last operand, so we'll check if it exists
            if (DCtx->NextOperand <= DCtx->Instruction->Operands)
                break;

            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            // offset is a value or a register?
            if ((DCtx->Opcodes.EWord1 & 0x0800) != 0)
            {
                // MOTOROLA M68000 FAMILY Programmer’s Reference Manual, page 4-34
                //  Bits 8 – 6 of the extension word specify a data register that contains the offset; bits 10 – 9 are zero.
                if ((DCtx->Opcodes.EWord1 & 0x0600) != 0)
                    break;

                // data register
                Operand->Info.OffsetWidth.Offset = (M68K_INT)(M68K_RT_D0 + ((DCtx->Opcodes.EWord1 >> 6) & 7));
            }
            else
                // immediate value
                Operand->Info.OffsetWidth.Offset = -(M68K_INT)((DCtx->Opcodes.EWord1 >> 6) & 31) - 1;

            // width is a value or a register?
            if ((DCtx->Opcodes.EWord1 & 0x0020) != 0)
            {
                // MOTOROLA M68000 FAMILY Programmer’s Reference Manual, page 4-34
                // Bits 2 – 0 of the extension word specify a data register that contains the width; bits 3 – 4 are zero
                if ((DCtx->Opcodes.EWord1 & 0x0018) != 0)
                    break;

                // data register
                Operand->Info.OffsetWidth.Width = (M68K_INT)(M68K_RT_D0 + (DCtx->Opcodes.EWord1 & 7));
            }
            else
            {
                // immediate value
                Operand->Info.OffsetWidth.Width = (M68K_INT)(DCtx->Opcodes.EWord1 & 31);
                if (Operand->Info.OffsetWidth.Width == 0)
                    Operand->Info.OffsetWidth.Width = 32;

                Operand->Info.OffsetWidth.Width = -Operand->Info.OffsetWidth.Width - 1;
            }

            Operand->Type = M68K_OT_OFFSET_WIDTH;
            Result = M68K_TRUE;
            break;

        case DAT_O_PReg_S10E12EW1:
            Register = _M68KMMUPRegisters[(DCtx->Opcodes.EWord1 >> 10) & 7];

implicit_register:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER;
            Operand->Info.Register = Register;
            Result = M68K_TRUE;
            break;

        case DAT_O_RegAC0_I:
            Register = M68K_RT_AC0;
            goto implicit_register;

        case DAT_O_RegAC1_I:
            Register = M68K_RT_AC1;
            goto implicit_register;

        case DAT_O_RegACUSR_I:
            Register = M68K_RT_ACUSR;
            goto implicit_register;

        case DAT_O_RegCCR_I:
            Register = M68K_RT_CCR;
            goto implicit_register;

        case DAT_O_RegCRP_I:
            Register = M68K_RT_CRP;
            goto implicit_register;

        case DAT_O_RegFPCR_I:
            Register = M68K_RT_FPCR;
            goto implicit_register;

        case DAT_O_RegFPIAR_I:
            Register = M68K_RT_FPIAR;
            goto implicit_register;

        case DAT_O_RegFPSR_I:
            Register = M68K_RT_FPSR;
            goto implicit_register;

        case DAT_O_RegList_EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;

            Operand->Type = M68K_OT_REGISTER_LIST;
            Operand->Info.RegisterList = (M68K_REGISTER_LIST)DCtx->Opcodes.EWord1;
            DCtx->IndexOperandRegisterList = (M68K_BYTE)(Operand - DCtx->Instruction->Operands);
            Result = M68K_TRUE;
            break;

        case DAT_O_RegPCSR_I:
            Register = M68K_RT_PCSR;
            goto implicit_register;

        case DAT_O_RegPSR_I:
            Register = M68K_RT_PSR;
            goto implicit_register;

        case DAT_O_RegSR_I:
            Register = M68K_RT_SR;
            goto implicit_register;

        case DAT_O_RegSRP_I:
            Register = M68K_RT_SRP;
            goto implicit_register;

        case DAT_O_RegTC_I:
            Register = M68K_RT_TC;
            goto implicit_register;

        case DAT_O_RegTT0_I:
            Register = M68K_RT_TT0;
            goto implicit_register;

        case DAT_O_RegTT1_I:
            Register = M68K_RT_TT1;
            goto implicit_register;

        case DAT_O_RegUSP_I:
            Register = M68K_RT_USP;
            goto implicit_register;

        case DAT_O_RegVAL_I:
            Register = M68K_RT_VAL;
            goto implicit_register;

        case DAT_O_Shift8_S9E11W0:
            Value = (DCtx->Opcodes.Word >> 9 & 7);
        
            // value 0 => count = 8
            if (Value == 0)
                Value = 8;

            InputIndex = -1;
            goto immediateB;

        case DAT_O_SKFac_S0E6EW1:
            if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                break;
            
            Operand->Type = M68K_OT_STATIC_KFACTOR;
            Operand->Info.SByte = (M68K_SBYTE)(DCtx->Opcodes.EWord1 & 0x3f);
            if ((Operand->Info.SByte & 0x20) != 0)
                Operand->Info.SByte |= 0xc0;

            Result = M68K_TRUE;
            break;

        case DAT_S_B_I:
            DCtx->Instruction->Size = M68K_SIZE_B;
            Result = M68K_TRUE;
            break;

        case DAT_S_BWL_S6E7W0:
            Value = ((DCtx->Opcodes.Word >> 6) & 3);

size_bwl:
            DCtx->Instruction->Size = M68K_SIZE_NONE;
        
            switch ((M68K_BYTE)Value)
            {
            case 0:
                DCtx->Instruction->Size = M68K_SIZE_B;
                break;

            case 1:
                DCtx->Instruction->Size = M68K_SIZE_W;
                break;
        
            case 2:
                DCtx->Instruction->Size = M68K_SIZE_L;
                break;

            default:
                break;
            }

            Result = (DCtx->Instruction->Size != M68K_SIZE_NONE);
            break;

        case DAT_S_BWL_S6E7EW1:
            Value = ((DCtx->Opcodes.EWord1 >> 6) & 3);
            goto size_bwl;

        case DAT_S_BWL_S9E10W0:
            Value = ((DCtx->Opcodes.Word >> 9) & 3);
            goto size_bwl;

        case DAT_S_FP_S10E12EW1:
            DCtx->Instruction->Size = _M68KFpuSizes[(DCtx->Opcodes.EWord1 >> 10) & 7];
            Result = (DCtx->Instruction->Size != M68K_SIZE_NONE);
            break;

        case DAT_S_L_I:
            DCtx->Instruction->Size = M68K_SIZE_L;
            Result = M68K_TRUE;
            break;

        case DAT_S_P_I:
            DCtx->Instruction->Size = M68K_SIZE_P;
            Result = M68K_TRUE;
            break;

        case DAT_S_PReg_S10E12EW1:
            DCtx->Instruction->Size = _M68KMMUPRegisterSizes[(DCtx->Opcodes.EWord1 >> 10) & 7];
            Result = M68K_TRUE;
            break;

        case DAT_S_Q_I:
            DCtx->Instruction->Size = M68K_SIZE_Q;
            Result = M68K_TRUE;
            break;

        case DAT_S_W_I:
            DCtx->Instruction->Size = M68K_SIZE_W;
            Result = M68K_TRUE;
            break;

        case DAT_S_WL_S6E6W0:
            DCtx->Instruction->Size = ((DCtx->Opcodes.Word & 0x0040) != 0 ? M68K_SIZE_L : M68K_SIZE_W);
            Result = M68K_TRUE;
            break;

        case DAT_S_WL_S8E8W0:
            DCtx->Instruction->Size = ((DCtx->Opcodes.Word & 0x0100) != 0 ? M68K_SIZE_L : M68K_SIZE_W);
            Result = M68K_TRUE;
            break;

        case DAT_S_X_I:
            DCtx->Instruction->Size = M68K_SIZE_X;
            Result = M68K_TRUE;
            break;

        default:
            break;
        }
    }

    return Result;
}

// extend an operand using the bits from the bank extension word
static M68K_BOOL ExtendOperandWithBankBits(PM68K_OPERAND Operand, M68K_WORD Bits, PM68K_WORD FinalBits)
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

        // the upper bit must be ignored because we can only extended from An to Bn i.e. we only have one bank
        if (Bits > 1)
            break;

        if (!IsValidRegister07(M68K_RT_A0, Operand->Info.Memory.Base, FinalBits))
            return (Bits == 0);

        if (Bits == 1)
        {
            Operand->Info.Memory.Base = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_B0 + *FinalBits);
            (*FinalBits) |= 8;
        }

        return M68K_TRUE;
        
    case M68K_OT_REGISTER:
        // Dn => Dn/En
        if (IsValidRegister07(M68K_RT_D0, Operand->Info.Register, FinalBits))
        {
extend_e0_e23:
            // we can extend using two bits
            // 01 => e0-e7
            if (Bits == 1)
            {
                Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E0 + *FinalBits);
                (*FinalBits) |= 8;
            }
            // 10 => e8-e15
            else if (Bits == 2)
            {
                Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E8 + *FinalBits);
                (*FinalBits) |= 0x10;
            }
            // 11 => e16-e23
            else if (Bits == 3)
            {
                Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E16 + *FinalBits);
                (*FinalBits) |= 0x18;
            }

            return M68K_TRUE;
        }

        // An => Bn
        if (IsValidRegister07(M68K_RT_A0, Operand->Info.Register, FinalBits))
        {
            // see the note above for memory operands
            if (Bits > 1)
                break;

            if (Bits == 1)
            {
                Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_B0 + *FinalBits);
                (*FinalBits) |= 8;
            }

            return M68K_TRUE;
        }

        // FPn => FPn/En
        if (IsValidRegister07(M68K_RT_FP0, Operand->Info.Register, FinalBits))
            goto extend_e0_e23;

        break;

    default:
        return (Bits == 0);
    }

    return M68K_FALSE;
}

// get the next operand in the instruction
static PM68K_OPERAND GetNextOperand(PDISASM_CTX DCtx)
{
    // too many operands?
    PM68K_OPERAND Operand = (DCtx->NextOperand)++;
    if (Operand >= (DCtx->Instruction->Operands + M68K_MAXIMUM_NUMBER_OPERANDS))
        return M68K_NULL;

    return Operand;
}

// get the vregister using the 4-bit value and extension bit
static M68K_REGISTER_TYPE_VALUE GetVRegister(M68K_BYTE Value, M68K_BYTE XBit)
{
//       Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(BaseReg + (Value & 7));
    if ((Value & 8) == 0)
        // d0-d7 (x = 0) or e8-e15 (x = 1)
        return (M68K_REGISTER_TYPE_VALUE)(((XBit & 1) == 0 ? M68K_RT_D0 : M68K_RT_E8) + (M68K_INT)(Value & 7));
    else
        // e0-e7 (x = 0) or e16-e23 (x = 1)
        return (M68K_REGISTER_TYPE_VALUE)(((XBit & 1) == 0 ? M68K_RT_E0 : M68K_RT_E16) + (M68K_INT)(Value & 7));
}

// init an operand for an addressing mode
static M68K_BOOL InitAModeOperand(PDISASM_CTX DCtx, ADDRESSING_MODE_TYPE_VALUE AMode, AMODE_FLAGS Flags, M68K_BYTE ModeRegister, M68K_BYTE BankExtension)
{
    PM68K_OPERAND Operand;
    M68K_OPERAND_TYPE_VALUE OperandType;
    M68K_REGISTER_TYPE_VALUE BaseRegister;

    M68K_BOOL Result = M68K_FALSE;

    // get the mode and the register
    M68K_BYTE Mode = (M68K_UINT)((ModeRegister >> 3) & 7);
    M68K_BYTE Register = (M68K_UINT)(ModeRegister & 7);

    // swap the fields?
    if ((Flags & AMF_SWAP_MODE_REGISTER) != 0)
    {
        M68K_BYTE Temp = Mode;
        Mode = Register;
        Register = Temp;
    }

    // the bank extension can be used?
    M68K_REGISTER_TYPE BaseAddrRegister;
    M68K_BYTE BaseAddrRegisterUsedMask;
    M68K_BYTE BankExtensionUnusedMask;

    if ((Flags & AMF_BASE_BANK) != 0)
    {
        BaseAddrRegister = ((BankExtension & 1) != 0 ? M68K_RT_B0 : M68K_RT_A0);
        BaseAddrRegisterUsedMask = 1;
        BankExtensionUnusedMask = 1;
    }
    else
    {
        BaseAddrRegister = M68K_RT_A0;
        BaseAddrRegisterUsedMask = 0;
        BankExtensionUnusedMask = 0;
    }

    // working in vector mode?
    M68K_BOOL VEA;
    M68K_BOOL VEABitAIsSet;

    if ((Flags & AMF_VEA) != 0)
    {
        VEA = M68K_TRUE;
        VEABitAIsSet = ((BankExtension & 1) != 0);

        if (VEABitAIsSet)
            BaseAddrRegister = M68K_RT_B0;
    }
    else
    {
        VEA = M68K_FALSE;
        VEABitAIsSet = M68K_FALSE;
    }

    M68K_SIZE Size = (M68K_SIZE)DCtx->Instruction->Size;

    switch (Mode)
    {
    // Dn/En
    case 0:
        // allowed?
        if ((AMode & AMT_DATA_REGISTER_DIRECT) != 0)
        {
            // Dn (when size is NONE, B, W, L or S because all other sizes are invalid);
            // in VEA mode we also allow size Q
            if (Size == M68K_SIZE_NONE || Size == M68K_SIZE_B || Size == M68K_SIZE_W || Size == M68K_SIZE_L || Size == M68K_SIZE_S || (VEA && Size == M68K_SIZE_Q))
            {
                if (VEA)
                    InitRegOperand(DCtx, VEABitAIsSet ? M68K_RT_E8 : M68K_RT_D0, Register);
                else
                    InitRegOperand(DCtx, M68K_RT_D0, Register);
                
                Result = M68K_TRUE;
            }
        }
        break;

    // An/En
    case 1:
        // note: 
        // address registers can't be used with byte operations so the size must be W or L;
        // the exception is apollo core 68080 which 
        // - includes some instructions where An/Bn can be used as a byte;
        // - in VEA mode the size is always Q
        if (Size == M68K_SIZE_W || Size == M68K_SIZE_L || (DCtx->DFlags & DCF_AMODE1B) != 0 || (VEA && Size == M68K_SIZE_Q))
        {
            // allowed?
            if ((AMode & AMT_ADDRESS_REGISTER_DIRECT) != 0 || 
                ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_P20H) != 0 && (DCtx->Architectures & M68K_ARCH__P20H__) != 0))
            {
                if (VEA)
                    InitRegOperand(DCtx, (VEABitAIsSet ? M68K_RT_E16 : M68K_RT_E0), Register);
                else
                {
                    InitRegOperand(DCtx, BaseAddrRegister, Register);

                    BankExtensionUnusedMask ^= BaseAddrRegisterUsedMask;
                }
                Result = M68K_TRUE;
            }
        }
        break;

    // (An/Bn)
    case 2:
        // allowed?
        if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT) != 0)
        {
            if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
            {
                Operand->Type = M68K_OT_MEM_INDIRECT;
                Operand->Info.Memory.Increment = M68K_INC_NONE;
                Operand->Info.Memory.Base = (M68K_REGISTER_TYPE_VALUE)(BaseAddrRegister + Register);
                Operand->Info.Memory.Index.Register = M68K_RT_NONE;
                Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
                Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;

                BankExtensionUnusedMask ^= BaseAddrRegisterUsedMask;
                Result = M68K_TRUE;
            }
        }
        break;

    // (An/Bn)+
    case 3:
        // allowed?
        if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT) != 0)
        {
            InitAModePIOperand(DCtx, BaseAddrRegister, Register);
            BankExtensionUnusedMask ^= BaseAddrRegisterUsedMask;
            Result = M68K_TRUE;
        }
        break;

    // -(An/Bn)
    case 4:
        // allowed?
        if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT) != 0)
        {
            InitAModePDOperand(DCtx, BaseAddrRegister, Register);
            BankExtensionUnusedMask ^= BaseAddrRegisterUsedMask;
            Result = M68K_TRUE;
        }
        break;
        
    // (d16, An/Bn)
    case 5:
        OperandType = M68K_OT_MEM_INDIRECT_DISP_W;
        BaseRegister = (M68K_REGISTER_TYPE_VALUE)(BaseAddrRegister + Register);

        // allowed?
        if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT) != 0)
        {
indirect_dispW:
            if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
            {
                SaveValue(DCtx, Operand, M68K_VT_BASE_DISPLACEMENT_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

                Operand->Type = OperandType;
                Operand->Info.Memory.Increment = M68K_INC_NONE;
                Operand->Info.Memory.Base = BaseRegister;
                Operand->Info.Memory.Index.Register = M68K_RT_NONE;
                Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                Operand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)(M68K_SWORD)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
                Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;

check_rpc_register:
                if ((Operand->Info.Memory.Base >= M68K_RT_A0 && Operand->Info.Memory.Base <= M68K_RT_A7) ||
                    (Operand->Info.Memory.Base >= M68K_RT_B0 && Operand->Info.Memory.Base <= M68K_RT_B7))
                    BankExtensionUnusedMask ^= BaseAddrRegisterUsedMask;

                if (Operand != M68K_NULL)
                {
                    // base register is pc? user allows the use of the special rpc register?
                    if (Operand->Info.Memory.Base == M68K_RT_PC && (DCtx->Flags & M68K_DFLG_ALLOW_RPC_REGISTER) != 0)
                    {
                        // the size of the base displacement is byte, word or long?
                        if (Operand->Info.Memory.Displacement.BaseSize != M68K_SIZE_NONE)
                        {
                            // change the base register and convert the displacement to an address
                            Operand->Info.Memory.Base = M68K_RT__RPC;
                            Operand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DCtx->Address + (M68K_SLWORD)(M68K_SWORD)DCtx->Opcodes.PCOffset + (M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue);
                        }
                    }

                    Result = M68K_TRUE;
                }
            }
        }
        break;

    // (d8, An/Bn, Xn.SIZE * SCALE)
    // (d32, An/Bn, Xn.SIZE * SCALE)
    // ([d32i, An/Bn], Xn.SIZE * SCALE, d32o)
    // ([d32i, An/Bn, Xn.SIZE * SCALE], d32o)
    case 6:
        BaseRegister = (M68K_REGISTER_TYPE_VALUE)(BaseAddrRegister + Register);

        // allowed?
        if ((AMode & (
            AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_B | 
            AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L | 
            AMT_MEMORY_INDIRECT_POST_INDEXED |
            AMT_MEMORY_INDIRECT_PRE_INDEXED
        )) != 0)
        {
indirect_index:
            {
                // read the brief extension word
                M68K_WORD EWord = _M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);

                // scale must be 1 in older architectures i.e. 68000, 68008 and 68010
                if ((DCtx->Architectures & M68K_ARCH__P10L__) != 0 && (DCtx->Architectures & M68K_ARCH__P20H__) == 0 && (EWord & 0x0600) != 0)
                    break;

                // revert to the full extension word?
                if ((EWord & 0x0100) == 0)
                {
                    // allowed?
                    if ((AMode & 
                        AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_B
                    ) == 0)
                        break;

                    if ((Operand = GetNextOperand(DCtx)) == M68K_NULL)
                        break;

                    SaveValue(DCtx, Operand, M68K_VT_BASE_DISPLACEMENT_B, DCtx->Opcodes.NumberWords - 1, M68K_TRUE);

                    Operand->Type = (Mode == 6 ? M68K_OT_MEM_INDIRECT_INDEX_DISP_B : M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B);
                    Operand->Info.Memory.Increment = M68K_INC_NONE;
                    Operand->Info.Memory.Base = BaseRegister;
                    Operand->Info.Memory.Index.Register = (M68K_REGISTER_TYPE_VALUE)(((EWord & 0x8000) != 0 ? M68K_RT_A0 : M68K_RT_D0) + ((EWord >> 12) & 7));
                    Operand->Info.Memory.Index.Size = ((EWord & 0x0800) != 0 ? M68K_SIZE_L : M68K_SIZE_W);
                    Operand->Info.Memory.Scale = (M68K_SCALE_VALUE)((EWord >> 9) & 3);
                    Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_B;
                    Operand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)(M68K_SBYTE)(M68K_BYTE)EWord;
                    Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    goto check_rpc_register;
                }
                // the full extension word is only available with 68020 or higher
                else if (
                    (DCtx->Architectures & M68K_ARCH__P20H__) != 0 &&
                    (AMode & (
                        AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L |    // f hhh = (0|1) 000
                        AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L |
                        AMT_MEMORY_INDIRECT_POST_INDEXED |                      // f hhh = 0 0(01|10|11) | 1 0(01|10|11) = (0|1) 0(01|10|11)
                        AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_POST_INDEXED |
                        AMT_MEMORY_INDIRECT_PRE_INDEXED |                       // f hhh = 0 1(01|10|11)
                        AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_PRE_INDEXED
                    )))
                {
                    // according to "Table 2-2. IS-I/IS Memory Indirect Action Encodings"
                    // I_IS = Index/Indirect Selection
                    // BD_SIZE = Base Displacement Size
                    M68K_WORD I_IS = (M68K_WORD)(EWord & 0x0007);
                    M68K_WORD BD_SIZE = (M68K_WORD)(EWord & 0x0030);

                    // the value of BD_SIZE can't be 00
                    if (BD_SIZE == 0)
                        break;

                    // get the operand
                    Operand = DCtx->NextOperand;

                    if (I_IS == 0)
                    {
                        // "no memory indirect action" allowed?
                        if ((AMode & (
                            AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L |
                            AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L
                        )) == 0)
                            break;

                        Operand->Type = (Mode == 6 ? M68K_OT_MEM_INDIRECT_INDEX_DISP_L : M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L);
                    }
                    else if (I_IS < 4)
                    {
                        // "memory indirect pre-indexed" allowed?
                        // note: pre-indexed are not available in CPU32
                        if ((AMode & (
                            AMT_MEMORY_INDIRECT_PRE_INDEXED |
                            AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_PRE_INDEXED
                        )) == 0 || (DCtx->Architectures & ~M68K_ARCH_CPU32) == 0)
                            break;

                        Operand->Type = (Mode == 6 ? M68K_OT_MEM_INDIRECT_PRE_INDEXED : M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED);
                    }
                    // the values for condition
                    //
                    //      (IS = 0 and Index/Indirect == 100) or 
                    //      (IS = 1 and Index/Indirect >= 100)
                    //
                    // are reserved
                    else if (((EWord & 0x0040) == 0 && I_IS == 4) || ((EWord & 0x0040) != 0 && I_IS >= 4))
                        break;
                    else
                    {
                        // "memory indirect post-indexed" allowed?
                        // note: post-indexed are not available in CPU32
                        if ((AMode & (
                            AMT_MEMORY_INDIRECT_POST_INDEXED |
                            AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_POST_INDEXED
                        )) == 0 || (DCtx->Architectures & ~M68K_ARCH_CPU32) == 0)
                            break;

                        Operand->Type = (Mode == 6 ? M68K_OT_MEM_INDIRECT_POST_INDEXED : M68K_OT_MEM_PC_INDIRECT_POST_INDEXED);
                    }

                    Operand->Info.Memory.Increment = M68K_INC_NONE;
                
                    if ((EWord & 0x0080) == 0)
                        Operand->Info.Memory.Base = BaseRegister;
                    else if (BaseRegister == M68K_RT_PC)
                        Operand->Info.Memory.Base = M68K_RT_ZPC;
                    else
                        Operand->Info.Memory.Base = M68K_RT_NONE;
                
                    if ((EWord & 0x0040) == 0)
                    {
                        Operand->Info.Memory.Index.Register = (M68K_REGISTER_TYPE_VALUE)(((EWord & 0x8000) != 0 ? M68K_RT_A0 : M68K_RT_D0) + ((EWord >> 12) & 7));
                        Operand->Info.Memory.Index.Size = ((EWord & 0x0800) != 0 ? M68K_SIZE_L : M68K_SIZE_W);
                        Operand->Info.Memory.Scale = (M68K_SCALE_VALUE)((EWord >> 9) & 3);
                    }
                    else
                        // suppressed index register
                        Operand->Info.Memory.Index.Register = M68K_RT_NONE;

                    if (BD_SIZE == 0x0010)
                        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
                    else if (BD_SIZE == 0x0020)
                    {
                        SaveValue(DCtx, Operand, M68K_VT_BASE_DISPLACEMENT_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

                        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                        Operand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)(M68K_SWORD)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
                    }
                    else if (BD_SIZE == 0x0030)
                    {
                        SaveValue(DCtx, Operand, M68K_VT_BASE_DISPLACEMENT_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

                        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                        Operand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)_M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                        DCtx->Opcodes.NumberWords += 2;
                    }
                    else
                        break;

                    if (I_IS == 0)
                        Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    else
                    {
                        switch (I_IS & 3)
                        {
                        case 1:
                            Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                            break;

                        case 2:
                            SaveValue(DCtx, Operand, M68K_VT_OUTER_DISPLACEMENT_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_W;
                            Operand->Info.Memory.Displacement.OuterValue = (M68K_SDWORD)(M68K_SWORD)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
                            break;

                        case 3:
                            SaveValue(DCtx, Operand, M68K_VT_OUTER_DISPLACEMENT_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_L;
                            Operand->Info.Memory.Displacement.OuterValue = (M68K_SDWORD)_M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            DCtx->Opcodes.NumberWords += 2;
                            break;

                        default:
                            break;
                        }
                    }

                    // if all elements are optional we might end up with [0] or [[0]] (these will be forced to prevent empty memory operands)
                    if ((Operand->Info.Memory.Base == M68K_RT_NONE || Operand->Info.Memory.Base == M68K_RT_ZPC) &&
                        Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                        Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_NONE &&
                        Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                    {
                        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                        Operand->Info.Memory.Displacement.BaseValue = 0;
                    }

                    // operand is valid
                    DCtx->NextOperand++;
                    goto check_rpc_register;
                }
            }
        }
        break;

    // (d16, PC)
    // (d8, PC, Xn.SIZE * SCALE)
    // (d32, PC, Xn.SIZE * SCALE)
    // ([d32i, PC], Xn.SIZE * SCALE, d32o)
    // ([d32i, PC, Xn.SIZE * SCALE], d32o)
    // (xxx).w
    // (xxx).l
    // #xxx
    case 7:
        switch (Register)
        {
        // (xxx).w
        case 0:
            // allowed?
            if ((AMode & AMT_ABSOLUTE_SHORT) != 0)
            {
                if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                {
                    SaveValue(DCtx, Operand, M68K_VT_ABSOLUTE_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

                    Operand->Type = M68K_OT_MEM_ABSOLUTE_W;
                    Operand->Info.Long = (M68K_DWORD)(M68K_SDWORD)(M68K_SWORD)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);

                    Result = M68K_TRUE;
                }
            }
            break;
        
        // (xxx).l
        case 1:
            // allowed?
            if ((AMode & AMT_ABSOLUTE_LONG) != 0)
            {
                if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                {
                    SaveValue(DCtx, Operand, M68K_VT_ABSOLUTE_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

                    Operand->Type = M68K_OT_MEM_ABSOLUTE_L;
                    Operand->Info.Long = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                    DCtx->Opcodes.NumberWords += 2;

                    Result = M68K_TRUE;
                }
            }
            break;

        // (d16, PC)
        case 2:
            // allowed?
            if ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT) != 0 ||
                ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT_P20H) != 0 && (DCtx->Architectures & M68K_ARCH__P20H__) != 0))
            {
                if (!SavePCOffset(DCtx))
                    break;
                
                OperandType = M68K_OT_MEM_PC_INDIRECT_DISP_W;
                BaseRegister = (M68K_REGISTER_TYPE_VALUE)M68K_RT_PC;
                goto indirect_dispW;
            }
            break;

        // (d8, PC, Xn.SIZE * SCALE)
        // (d32, PC, Xn.SIZE * SCALE)
        // ([d32i, PC], Xn.SIZE * SCALE, d32o)
        // ([d32i, PC, Xn.SIZE * SCALE], d32o)
        case 3:
            // allowed?
            if ((AMode & (
                AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_B |
                AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L |
                AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_POST_INDEXED |
                AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_PRE_INDEXED
            )) != 0)
            {
                if (!SavePCOffset(DCtx))
                    break;

                BaseRegister = (M68K_REGISTER_TYPE_VALUE)M68K_RT_PC;
                goto indirect_index;
            }
            break;

        case 4:
            // allowed?
            if ((AMode & AMT_IMMEDIATE_DATA) != 0 ||
                ((AMode & AMT_IMMEDIATE_DATA_P20H) != 0 && (DCtx->Architectures & M68K_ARCH__P20H__) != 0))
            {
                // vea mode?
                if (VEA)
                {
                    // the size of the operand must be W or Q according to the A bit;
                    if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                    {
                        SaveValue(DCtx, Operand, VEABitAIsSet ? M68K_VT_IMMEDIATE_W : M68K_VT_IMMEDIATE_Q, DCtx->Opcodes.NumberWords, M68K_FALSE);

                        if (VEABitAIsSet)
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_W;
                            Operand->Info.Word = _M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);
                        }
                        else
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_Q;
                            Operand->Info.Quad[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Operand->Info.Quad[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
                            DCtx->Opcodes.NumberWords += 4;
                        }

                        Result = M68K_TRUE;
                    }
                }
                else
                {
                    // the instruction size will tell us how may words should be read
                    switch (Size)
                    {
                    case M68K_SIZE_B:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_B, DCtx->Opcodes.NumberWords, M68K_TRUE);

                            Operand->Type = M68K_OT_IMMEDIATE_B;
                            Operand->Info.Byte = (M68K_WORD)(M68K_BYTE)_M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_D:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_D, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_D;

                            M68K_DWORD Values[2];

#ifdef M68K_TARGET_IS_BIG_ENDIAN
                            Values[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Values[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
#else
                            Values[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Values[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
#endif

                            DCtx->Opcodes.NumberWords += 4;

                            Operand->Info.Double = *(PM68K_DOUBLE)Values;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_L:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_L, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_L;
                            Operand->Info.Long = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            DCtx->Opcodes.NumberWords += 2;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_P:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_P, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_P;
                            Operand->Info.PackedDecimal[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Operand->Info.PackedDecimal[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
                            Operand->Info.PackedDecimal[2] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 4);
                            DCtx->Opcodes.NumberWords += 6;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_Q:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_Q, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_Q;
                            Operand->Info.Quad[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Operand->Info.Quad[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
                            DCtx->Opcodes.NumberWords += 4;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_S:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_S, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_S;

                            M68K_DWORD Value = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            DCtx->Opcodes.NumberWords += 2;

                            Operand->Info.Single = *(PM68K_SINGLE)&Value;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_W:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_W, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_W;
                            Operand->Info.Word = _M68KDisReadWord(&(DCtx->Input), DCtx->Opcodes.NumberWords++);

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_X:
                        if ((Operand = GetNextOperand(DCtx)) != M68K_NULL)
                        {
                            SaveValue(DCtx, Operand, M68K_VT_IMMEDIATE_X, DCtx->Opcodes.NumberWords, M68K_FALSE);

                            Operand->Type = M68K_OT_IMMEDIATE_X;
                            Operand->Info.Extended[0] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords);
                            Operand->Info.Extended[1] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 2);
                            Operand->Info.Extended[2] = _M68KDisReadDWord(&(DCtx->Input), DCtx->Opcodes.NumberWords + 4);
                            DCtx->Opcodes.NumberWords += 6;

                            Result = M68K_TRUE;
                        }
                        break;

                    case M68K_SIZE_NONE:
                    default:
                        break;
                    }
                }
            }
            break;

        case 5:
        case 6:
        case 7:
        default:
            break;
        }
        break;

    default:
        break;
    }
    
    // the bits that weren't used in the bank extension byte must be 0
    return (Result && (VEA || (BankExtension & BankExtensionUnusedMask) == 0));
}

// init an operand for the addressig mode AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT using the supplied address register index
static void InitAModePDOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE BaseReg, M68K_BYTE Value)
{
    PM68K_OPERAND Operand = GetNextOperand(DCtx);
    if (Operand != M68K_NULL)
    {
        Operand->Type = M68K_OT_MEM_INDIRECT_PRE_DECREMENT;
        Operand->Info.Memory.Increment = M68K_INC_PRE_DECREMENT;
        Operand->Info.Memory.Base = (M68K_REGISTER_TYPE_VALUE)(BaseReg + (Value & 7));
        Operand->Info.Memory.Index.Register = M68K_RT_NONE;
        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
        Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
    }

    DCtx->UsesPredecrement = M68K_TRUE;
}

// init an operand for the addressig mode AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT using the supplied address register index
static void InitAModePIOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE BaseReg, M68K_BYTE Value)
{
    PM68K_OPERAND Operand = GetNextOperand(DCtx);
    if (Operand != M68K_NULL)
    {
        Operand->Type = M68K_OT_MEM_INDIRECT_POST_INCREMENT;
        Operand->Info.Memory.Increment = M68K_INC_POST_INCREMENT;
        Operand->Info.Memory.Base = (M68K_REGISTER_TYPE_VALUE)(BaseReg + (Value & 7));
        Operand->Info.Memory.Index.Register = M68K_RT_NONE;
        Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
        Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
    }
}

// init an operand for a register
static void InitRegOperand(PDISASM_CTX DCtx, M68K_REGISTER_TYPE_VALUE BaseReg, M68K_BYTE Value)
{
    PM68K_OPERAND Operand = GetNextOperand(DCtx);
    if (Operand != M68K_NULL)
    {
        Operand->Type = M68K_OT_REGISTER;
        Operand->Info.Register = (M68K_REGISTER_TYPE_VALUE)(BaseReg + (Value & 7));
    }
}

// init an operand for a vector register i.e. d0-d7,e0-e23
static void InitVRegOperand(PDISASM_CTX DCtx, M68K_BYTE Value, M68K_BYTE XBit)
{
    PM68K_OPERAND Operand = GetNextOperand(DCtx);
    if (Operand != M68K_NULL)
    {
        Operand->Type = M68K_OT_REGISTER;
        Operand->Info.Register = GetVRegister(Value, XBit);
    }
}

// check if a register is valid
static M68K_BOOL IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD Bits)
{
    if (Register >= BaseRegister && Register < (BaseRegister + 8))
    {
        *Bits = (M68K_WORD)(Register - BaseRegister);
        return M68K_TRUE;
    }
    else
        return M68K_FALSE;
}

// save the PC offset for the displacements
static M68K_BOOL SavePCOffset(PDISASM_CTX DCtx)
{
    // already saved?
    if (DCtx->Opcodes.PCOffset != 0)
        return M68K_FALSE;

    DCtx->Opcodes.PCOffset = (M68K_BYTE)(DCtx->Opcodes.NumberWords << 1);
    return M68K_TRUE;
}

// try to save a value in the operand
static void SaveValue(PDISASM_CTX DCtx, PM68K_OPERAND Operand, M68K_VALUE_TYPE ValueType, M68K_UINT InputIndex, M68K_BOOL InputIsByte)
{
    PM68K_VALUE_LOCATION Value;

    // can we really save it?
    if ((Value = DCtx->Values.NextValue) != M68K_NULL)
    {
        // get the operand index
        M68K_UINT OperandIndex = (M68K_UINT)(Operand - DCtx->Instruction->Operands);
            
        // the corresponding bit is set in the mask?
        if ((((M68K_WORD)1 << OperandIndex) & DCtx->Values.OperandsMask) != 0)
        {
            // too many values?
            if (DCtx->Values.TotalNumberValues < DCtx->Values.MaximumNumberValues)
            {
                Value->Type = ValueType;
                Value->Location = (PM68K_BYTE)(DCtx->Input.Start + InputIndex);
                if (InputIsByte)
                    Value->Location++;

                Value->OperandIndex = (M68K_OPERAND_INDEX_VALUE)OperandIndex;

                // get the location to save the next value
                DCtx->Values.NextValue++;
            }

            // update the total number of avlues that are erquired
            DCtx->Values.TotalNumberValues++;
        }
    }
}

// get/extract the information from the bank extension word
void _M68KDisGetBankExtensionInfo(M68K_WORD BankExtensionWord, PBANK_EXTENSION_INFO BankExtensionInfo)
{
    BankExtensionInfo->A = (M68K_WORD)((BankExtensionWord >> 2) & 3);
    BankExtensionInfo->B = (M68K_WORD)(BankExtensionWord & 3);
    BankExtensionInfo->C = (M68K_WORD)(((BankExtensionWord & 0x30) >> 1) | ((BankExtensionWord >> 9) & 7));
    BankExtensionInfo->Size = (M68K_WORD)(4 + ((BankExtensionWord >> 5) & 6));
}

// get the implict pc offset for an instruction; returns the offset in bytes or 0 if the instruction doesn't support pc displacements
M68K_BYTE _M68KDisGetImplicitPCOffset(M68K_INSTRUCTION_TYPE InstrType)
{
    switch (InstrType)
    {
    case M68K_IT_BCC:
    case M68K_IT_BRA:
    case M68K_IT_BSR:
    case M68K_IT_CPBCC:
    case M68K_IT_DBCC:
    case M68K_IT_FBCC:
    case M68K_IT_PBCC:
        // dest = pc + 2 (bytes) + disp
        return 2;

    case M68K_IT_CPDBCC:
    case M68K_IT_FDBCC:
    case M68K_IT_PDBCC:
        // dest = pc + 4 (bytes) + disp
        return 4;

    default:
        return 0;
    }
}

// invert all bits in a word
M68K_WORD _M68KDisInvertWordBits(M68K_WORD Word)
{
    // at least one bit?
    if (Word == 0)
        return 0;

    M68K_WORD InvertedWord = 0;

    for (M68K_WORD Index = 0; Index < 16; Index++)
    {
        InvertedWord <<= 1;
        InvertedWord |= (Word & 1);
        Word >>= 1;
    }

    return InvertedWord;
}

// read a dword value from the input
M68K_DWORD _M68KDisReadDWord(PDISASM_CTX_INPUT DInput, M68K_UINT Index)
{
    M68K_DWORD DWord = ((M68K_DWORD)_M68KDisReadWord(DInput, Index) << 16);
    return DWord + (M68K_DWORD)_M68KDisReadWord(DInput, Index + 1);
}

// read a word value from the input
M68K_WORD _M68KDisReadWord(PDISASM_CTX_INPUT DInput, M68K_UINT Index)
{
    M68K_WORD Word;

    // can we read from the input ?
    if ((DInput->Start + Index) >= DInput->End)
        // no
        return 0;

    // read the word
    Word = DInput->Start[Index];

#ifndef M68K_TARGET_IS_BIG_ENDIAN
    // swap bytes
    Word = (M68K_WORD)((Word >> 8) | (Word << 8));
#endif

    return Word;
}

// disassemble the next instruction
PM68K_WORD M68KDisassemble(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction)
{
    return M68KDisassembleEx(Address, Start, End, Flags, Architectures, Instruction, M68K_IT_INVALID, M68K_NULL, 0, 0, M68K_NULL);
}

PM68K_WORD M68KDisassembleEx(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction, M68K_INSTRUCTION_TYPE ForcedInstructionType /*M68K_IT_INVALID = ignored*/, PM68K_VALUE_LOCATION Values /*M68K_NULL = values are ignored*/, M68K_VALUE_OPERANDS_MASK OperandsMask /*0 = values are ignored*/, M68K_UINT MaximumNumberValues /*0 = values are ignored*/, PM68K_UINT TotalNumberValues /*can be M68K_NULL*/)
{
    M68K_WORD BankExtensionWord;
    M68K_INSTRUCTION LocalInstruction;

    PM68K_WORD Result = Disassemble(Address, Start, End, Flags, Architectures, &LocalInstruction, M68K_IT_INVALID, Values, OperandsMask, MaximumNumberValues, TotalNumberValues, &BankExtensionWord);
    if (Result != M68K_NULL)
    {
        // is it an bank extension word? is it allowed? is that really what the user wants?
        if ((Flags & M68K_DFLG_ALLOW_BANK_EXTENSION_WORD) != 0 &&
            LocalInstruction.Type == M68K_IT_BANK && 
            ForcedInstructionType != M68K_IT_BANK)
        {
            // try to disassemble the next instruction;
            // please note that the bank instruction will never return any values, 
            // which means that we can use the same information
            M68K_INSTRUCTION BankedInstruction;
            PM68K_WORD BankedResult = Disassemble(Address + 1, Result, End, Flags, Architectures, &BankedInstruction, ForcedInstructionType, Values, OperandsMask, MaximumNumberValues, TotalNumberValues, M68K_NULL);
            if (BankedResult != M68K_NULL)
            {
                // can we apply the bank extension word?
                // this is determined by the flags in the internal type
                INSTRUCTION_TYPE IType = (INSTRUCTION_TYPE)BankedInstruction.InternalType;
                INSTRUCTION_FLAGS_VALUE IFlags = _M68KInstrTypeInfos[IType].IFlags;

                if ((IFlags & IF_FPU_OPMODE) != 0)
                    IFlags = _M68KInstrFPUOpmodes[_M68KInstrMasterTypeFPUOpmodes[BankedInstruction.Type]].IFlags;

                if ((IFlags & IF_BANK_EW_A) != 0)
                {
                    // the first check we can do is the size of the whole opcode
                    BANK_EXTENSION_INFO BankExtensionInfo;
                    _M68KDisGetBankExtensionInfo(BankExtensionWord, &BankExtensionInfo);

                    if ((M68K_WORD)(BankedResult - Start) == (BankExtensionInfo.Size >> 1))
                    {
                        M68K_WORD Bits;
                        M68K_BOOL CanContinueChecking;

                        // can we use the A bits?
                        if ((IFlags & IF_BANK_EW_A) != 0)
                            CanContinueChecking = ExtendOperandWithBankBits(BankedInstruction.Operands + 0, BankExtensionInfo.A, &Bits);
                        else
                            CanContinueChecking = (BankExtensionInfo.A == 0);

                        if (CanContinueChecking)
                        {
                            // can we use the B bits?
                            if ((IFlags & IF_BANK_EW_B) != 0)
                                CanContinueChecking = ExtendOperandWithBankBits(BankedInstruction.Operands + 1, BankExtensionInfo.B, &Bits);
                            else
                                CanContinueChecking = (BankExtensionInfo.B == 0);

                            if (CanContinueChecking)
                            {
                                // can we use the C bits?
                                if ((IFlags & IF_BANK_EW_C) != 0)
                                {
                                    // the operand must be "none" because the second operand and the C bits will tell us how to identify it
                                    if (BankedInstruction.Operands[1].Type == M68K_OT_REGISTER &&
                                        BankedInstruction.Operands[2].Type == M68K_OT_NONE)
                                    {
                                        // the second operand must be a floating point register or an extended data register
                                        M68K_REGISTER_TYPE_VALUE Register = BankedInstruction.Operands[1].Info.Register;
                                        if ((Register >= M68K_RT_FP0 && Register <= M68K_RT_FP7) || (Register >= M68K_RT_E0 && Register <= M68K_RT_E23))
                                        {
                                            // the C bits are all 0?
                                            if (BankExtensionInfo.C != 0)
                                            {
                                                // xor the bits of B with C
                                                Bits ^= BankExtensionInfo.C;

                                                // convert to a register
                                                switch (Bits >> 3)
                                                {
                                                case 0:
                                                    Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_FP0 + (Bits & 7));
                                                    break;

                                                case 1:
                                                    Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E0 + (Bits & 7));
                                                    break;

                                                case 2:
                                                    Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E8 + (Bits & 7));
                                                    break;

                                                case 3:
                                                    Register = (M68K_REGISTER_TYPE_VALUE)(M68K_RT_E16 + (Bits & 7));
                                                    break;

                                                default:
                                                    CanContinueChecking = M68K_FALSE;
                                                    break;
                                                }

                                                if (CanContinueChecking)
                                                {
                                                    BankedInstruction.Operands[2].Type = M68K_OT_REGISTER;
                                                    BankedInstruction.Operands[2].Info.Register = Register;
                                                }
                                            }
                                            else
                                                CanContinueChecking = M68K_TRUE;
                                        }
                                    }
                                }
                                else
                                    CanContinueChecking = (BankExtensionInfo.C == 0);

                                if (CanContinueChecking)
                                {
                                    if (Instruction != M68K_NULL)
                                    {
                                        // the extended instruction is valid
                                        *Instruction = BankedInstruction;
                                        Instruction->Start = LocalInstruction.Start;
                                        Instruction->PCOffset += sizeof(M68K_WORD);

                                        return BankedResult;
                                    }
                                }
                            }
                        }
                    }
                }

                // make sure we don't return the values from the second instruction
                if (TotalNumberValues != M68K_NULL)
                    *TotalNumberValues = 0;
            }
        }
        else if (ForcedInstructionType != M68K_IT_INVALID && ForcedInstructionType != LocalInstruction.Type)
            Result = M68K_NULL;

        if (Result)
        {
            // if we got here we need to return the first instruction that was disassembled
            if (Instruction != M68K_NULL)
                *Instruction = LocalInstruction;
        }
    }

    return Result;
}
