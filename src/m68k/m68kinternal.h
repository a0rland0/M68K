#ifndef __M68KINTERNAL_H__
#define __M68KINTERNAL_H__

#include "m68k.h"
#include "m68kinternal_amodes.h"

// use this macro to catch internal errors with an assert
#define MIN_BYTE        (M68K_SDWORD)(-MAX_BYTE - 1)
#define MIN_LONG        (-MAX_LONG - 1)
#define MIN_WORD        (M68K_SDWORD)(-MAX_WORD - 1)
#define MAX_BYTE        (127L)
#define MAX_LONG        (2147483647L)
#define MAX_WORD        (32767L)
#define NUM_WORDS_INSTR 8

// Bit 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
// Val 0  1  1  1  C3 C4  C5 1  S1 S2 C1 C2 A1 A2 B1 B2
typedef struct BANK_EXTENSION_INFO
{
    M68K_WORD   A;      // A1 A2
    M68K_WORD   B;      // B1 B2
    M68K_WORD   C;      // C1 C2 C3 C4 C5
    M68K_WORD   Size;   // S1 S2
} BANK_EXTENSION_INFO, *PBANK_EXTENSION_INFO;

/*
    naming rules:
        <action for X>_<action for N>_<action for Z>_<action for V>_<action for C>
        
    actions:
        C   Clear
        N   No operation i.e. unchanged
        O   Operation
        S   Set
        T   Test
        U   Unknown
*/
typedef enum CONDITION_CODE_FLAGS
{
    CCF_N_N_N_N_N,      // ----- / -----
    CCF_N_C_S_C_C,      // ----- / -0100
    CCF_N_N_O_N_N,      // ----- / --*--
    CCF_N_O_O_C_C,      // ----- / -**00
    CCF_N_O_O_C_O,      // ----- / -**0*
    CCF_N_O_O_O_C,      // ----- / -***0
    CCF_N_O_O_O_O,      // ----- / -****
    CCF_N_O_U_U_U,      // ----- / -*UUU
    CCF_N_U_O_U_O,      // ----- / -U*U*
    CCF_O_O_O_C_O,      // ----- / ***0*
    CCF_O_O_O_O_O,      // ----- / *****
    CCF_TO_O_O_C_O,     // T---- / ***0*
    CCF_TO_O_TO_O_O,    // T-T-- / *****
    CCF_TO_U_TO_U_O,    // T-T-- / *U*U*
    CCF__SIZEOF__,

    // this value can be used to identify instructions that have different flags according to the operands
    CCF__CONDITIONAL__ = 0x80,
} CONDITION_CODE_FLAGS, *PCONDITION_CODE_FLAGS;

typedef M68K_BYTE CONDITION_CODE_FLAGS_VALUE, *PCONDITION_CODE_FLAGS_VALUE;

typedef struct CONTROL_REGISTER
{
    M68K_WORD                   Value;          // value for the M68K_IT_MOVEC instruction
    M68K_REGISTER_TYPE_VALUE    Register;       // register that is associated with the value
    M68K_ARCHITECTURE_VALUE     Architectures;  // architectures that support the control register
} CONTROL_REGISTER, *PCONTROL_REGISTER;

typedef CONTROL_REGISTER M68K_CONST C_CONTROL_REGISTER, *PC_CONTROL_REGISTER;

typedef M68K_DWORD IEEE_VALUE_DOUBLE[2];    // [0] 63..32, [1] 31..0
typedef M68K_DWORD IEEE_VALUE_EXTENDED[3];  // [0] 95..64, [1] 63..32, [2] 31..0
typedef M68K_DWORD IEEE_VALUE_SINGLE[1];    // [0] 31..0

typedef enum INSTRUCTION_FLAGS
{
    IF_CPX                  = 0x0001,   // cpX instruction
    IF_EXTENSION_WORD_1     = 0x0002,   // requires an extension word
    IF_EXTENSION_WORD_2     = 0x0004,   // requires a second extension word
    IF_FMOVE_KFACTOR        = 0x0008,   // instruction is a fpu move with k-factor
    IF_FPU_OPMODE           = 0x0010,   // requires an fpu opmode
    IF_LIST_REGISTERS       = 0x0020,   // one of the operands is a list of registers which needs to be inverted when "predecrement" addressing mode is used
    IF_BANK_EW_A            = 0x0040,   // allows the use of the A bits in the bank extension word
    IF_BANK_EW_B            = 0x0080,   // allows the use of the B bits in the bank extension word
    IF_BANK_EW_C            = 0x0100,   // allows the use of the C bits in the bank extension word
} INSTRUCTION_FLAGS, *PINSTRUCTION_FLAGS;

typedef M68K_WORD INSTRUCTION_FLAGS_VALUE, *PINSTRUCTION_FLAGS_VALUE;

typedef struct INSTRUCTION_FPU_OPMODE_INFO
{
    M68K_INSTRUCTION_TYPE_VALUE MasterType;     // master instruction type
    INSTRUCTION_FLAGS           IFlags;         // flags for the instruction
    M68K_ARCHITECTURE_VALUE     Architectures;  // architectures that support the instruction
} INSTRUCTION_FPU_OPMODE_INFO, *PINSTRUCTION_FPU_OPMODE_INFO;

typedef INSTRUCTION_FPU_OPMODE_INFO M68K_CONST C_INSTRUCTION_FPU_OPMODE_INFO, *PC_INSTRUCTION_FPU_OPMODE_INFO;

typedef enum INSTRUCTION_MASTER_FLAGS
{
    IMF_CONDITION_CODE  = 0x01, // requires a condition code that is the first operand
    IMF_FPU_OPMODE      = 0x02, // instruction requires an fpu opmode
    IMF_IMPLICIT_SIZE   = 0x04, // the size of the instruction is implicit
} INSTRUCTION_MASTER_FLAGS, *PINSTRUCTION_MASTER_FLAGS;

typedef M68K_BYTE INSTRUCTION_MASTER_FLAGS_VALUE, *PINSTRUCTION_MASTER_FLAGS_VALUE;
typedef INSTRUCTION_MASTER_FLAGS_VALUE M68K_CONST C_INSTRUCTION_MASTER_FLAGS_VALUE, *PC_INSTRUCTION_MASTER_FLAGS_VALUE;

typedef enum INSTRUCTION_TYPE
{
    IT_INVALID = 0,
    IT_ABCD,
    IT_ADD,
    IT_ADDA,
    IT_ADDI,
    IT_ADDIW,
    IT_ADDQ,
    IT_ADDX,
    IT_AND,
    IT_ANDI,
    IT_ANDI_TO_CCR,
    IT_ANDI_TO_SR,
    IT_ASL,
    IT_ASR,
    IT_BANK,
    IT_BCC,
    IT_BCC_LONG,
    IT_BCHG,
    IT_BCHG_IMM,
    IT_BCLR,
    IT_BCLR_IMM,
    IT_BFCHG,
    IT_BFCLR,
    IT_BFEXTS,
    IT_BFEXTU,
    IT_BFFFO,
    IT_BFINS,
    IT_BFLYB,
    IT_BFLYW,
    IT_BFSET,
    IT_BFTST,
    IT_BGND,
    IT_BKPT,
    IT_BRA,
    IT_BSEL,
    IT_BSET,
    IT_BSET_IMM,
    IT_BSR,
    IT_BTST,
    IT_BTST_IMM,
    IT_C2P,
    IT_CALLM,
    IT_CAS,
    IT_CAS2,
    IT_CHK,
    IT_CHK_LONG,
    IT_CHK2,
    IT_CINVA,
    IT_CINVL,
    IT_CINVP,
    IT_CLR,
    IT_CMP,
    IT_CMP2,
    IT_CMPA,
    IT_CMPI,
    IT_CMPIW,
    IT_CMPM,
    IT_CPBCC,
    IT_CPDBCC,
    IT_CPGEN,
    IT_CPRESTORE,
    IT_CPSAVE,
    IT_CPSCC,
    IT_CPTRAPCC,
    IT_CPUSHA,
    IT_CPUSHL,
    IT_CPUSHP,
    IT_DBCC,
    IT_DIVS,
    IT_DIVS_LONG,
    IT_DIVSL,
    IT_DIVU,
    IT_DIVU_LONG,
    IT_DIVUL,
    IT_EOR,
    IT_EORI,
    IT_EORI_TO_CCR,
    IT_EORI_TO_SR,
    IT_EXG_A_A,
    IT_EXG_D_A,
    IT_EXG_D_D,
    IT_EXT,
    IT_EXTB,
    IT_FBCC,
    IT_FBCC_LONG,
    IT_FDBCC,
    IT_FMOVE_DKFACTOR,
    IT_FMOVE_FPCR,
    IT_FMOVE_NKFACTOR,
    IT_FMOVE_SKFACTOR,
    IT_FMOVECR,
    IT_FMOVEM,
    IT_FNOP,
    IT_FPOP,
    IT_FPOP_REG,
    IT_FRESTORE,
    IT_FSAVE,
    IT_FSCC,
    IT_FSINCOS,
    IT_FTRAPCC,
    IT_FTST,
    IT_ILLEGAL,
    IT_JMP,
    IT_JSR,
    IT_LEA,
    IT_LINK,
    IT_LINK_LONG,
    IT_LOAD,
    IT_LOADI,
    IT_LPSTOP,
    IT_LSL,
    IT_LSLQ,
    IT_LSR,
    IT_LSRQ,
    IT_MINTERM,
    IT_MOVE,
    IT_MOVE_AC80,
    IT_MOVE_FROM_CCR,
    IT_MOVE_FROM_SR,
    IT_MOVE_FROM_USP,
    IT_MOVE_TO_CCR,
    IT_MOVE_TO_SR,
    IT_MOVE_TO_USP,
    IT_MOVE16,
    IT_MOVE16_PD_PD,
    IT_MOVEA,
    IT_MOVEC,
    IT_MOVEM,
    IT_MOVEP,
    IT_MOVEQ,
    IT_MOVES,
    IT_MOVES_AC80,
    IT_MOVEX,
    IT_MULS,
    IT_MULS_LONG,
    IT_MULU,
    IT_MULU_LONG,
    IT_NBCD,
    IT_NEG,
    IT_NEGX,
    IT_NOP,
    IT_NOT,
    IT_OR,
    IT_ORI,
    IT_ORI_TO_CCR,
    IT_ORI_TO_SR,
    IT_PABSB,
    IT_PABSW,
    IT_PACK,
    IT_PACK3216,
    IT_PACKUSWB,
    IT_PADDB,
    IT_PADDUSB,
    IT_PADDUSW,
    IT_PADDW,
    IT_PAND,
    IT_PANDN,
    IT_PAVG,
    IT_PBCC,
    IT_PBCC_LONG,
    IT_PCMPEQB,
    IT_PCMPEQW,
    IT_PCMPGEB,
    IT_PCMPGEW,
    IT_PCMPGTB,
    IT_PCMPGTW,
    IT_PCMPHIB,
    IT_PCMPHIW,
    IT_PDBCC,
    IT_PEA,
    IT_PEOR,
    IT_PERM,
    IT_PFLUSH_3051,
    IT_PFLUSH_4060,
    IT_PFLUSHA_3051,
    IT_PFLUSHA_4060,
    IT_PFLUSHAN_4060,
    IT_PFLUSHN_4060,
    IT_PFLUSHR_51,
    IT_PFLUSHS_51,
    IT_PLOADR_3051,
    IT_PLOADW_3051,
    IT_PLPAR,
    IT_PLPAW,
    IT_PMAXSB,
    IT_PMAXSW,
    IT_PMAXUB,
    IT_PMAXUW,
    IT_PMINSB,
    IT_PMINSW,
    IT_PMINUB,
    IT_PMINUW,
    IT_PMOVE_30,
    IT_PMOVE_30EC,
    IT_PMOVE_51,
    IT_PMOVEFD_30,
    IT_PMUL88,
    IT_PMULA,
    IT_PMULH,
    IT_PMULL,
    IT_POR,
    IT_PRESTORE,
    IT_PSAVE,
    IT_PSCC,
    IT_PSUBB,
    IT_PSUBUSB,
    IT_PSUBUSW,
    IT_PSUBW,
    IT_PTESTR_3051,
    IT_PTESTR_30EC,
    IT_PTESTR_4040EC,
    IT_PTESTW_3051,
    IT_PTESTW_30EC,
    IT_PTESTW_4040EC,
    IT_PTRAPCC_51,
    IT_PVALID_51,
    IT_RESET,
    IT_ROL,
    IT_ROR,
    IT_ROXL,
    IT_ROXR,
    IT_RPC,
    IT_RTD,
    IT_RTE,
    IT_RTM,
    IT_RTR,
    IT_RTS,
    IT_SCC,
    IT_STOP,
    IT_STORE,
    IT_STOREC,
    IT_STOREI,
    IT_STOREILM,
    IT_STOREM,
    IT_STOREM3,
    IT_SUB,
    IT_SUBA,
    IT_SUBI,
    IT_SUBQ,
    IT_SUBX,
    IT_SWAP,
    IT_TAS,
    IT_TBLS,
    IT_TBLSN,
    IT_TBLU,
    IT_TBLUN,
    IT_TOUCH,
    IT_TRANSHI,
    IT_TRANSILO,
    IT_TRANSLO,
    IT_TRAP,
    IT_TRAPCC,
    IT_TRAPV,
    IT_TST,
    IT_UNLK,
    IT_UNPACK1632,
    IT_UNPK,
    IT_VPERM,
    IT__SIZEOF__,
} INSTRUCTION_TYPE, *PINSTRUCTION_TYPE;

typedef M68K_WORD INSTRUCTION_TYPE_VALUE, *PINSTRUCTION_TYPE_VALUE;

typedef struct INSTRUCTION_TYPE_INFO
{
    M68K_INSTRUCTION_TYPE_VALUE MasterType;     // master instruction type
    CONDITION_CODE_FLAGS_VALUE  CCFlags;        // flags for the condition codes
    INSTRUCTION_FLAGS_VALUE     IFlags;         // flags for the instruction
    M68K_ARCHITECTURE_VALUE     Architectures;  // architectures where the instructions is supported
} INSTRUCTION_TYPE_INFO, *PINSTRUCTION_TYPE_INFO;

typedef INSTRUCTION_TYPE_INFO M68K_CONST C_INSTRUCTION_TYPE_INFO, *PC_INSTRUCTION_TYPE_INFO;

typedef enum TEXT_CASE
{
    TC_NONE,
    TC_LOWER,
    TC_UPPER,
} TEXT_CASE, *PTEXT_CASE;

typedef struct VALUE_MASK
{
    M68K_WORD   Value;
    M68K_WORD   Mask;
} VALUE_MASK, *PVALUE_MASK;

typedef VALUE_MASK M68K_CONST C_VALUE_MASK, *PC_VALUE_MASK;

// assembler
typedef enum ASM_CTX_FLAGS
{
    ACF_AMODE1B             = 0x0001,   // addressing mode 1 (An/Bn) is allowed in instructions with size M68K_SIZE_B
    ACF_BANK_EXTENSION_WORD = 0x0002,   // the instruction is only valid when we have a bank extension word
} ASM_CTX_FLAGS, *PASM_CTX_FLAGS;

typedef M68K_WORD DISASM_CTX_FLAGS_VALUE, *PDISASM_CTX_FLAGS_VALUE;

typedef struct ASM_CTX_OPCODES
{
    M68K_WORD   NumberWords;                                // total number of words used by the opcodes
    M68K_BYTE   PCOffset;                                   // offset in bytes (from the start of the instruction) that is used to calculate final addresses
    M68K_WORD   Words[M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS]; // buffer for the words
} ASM_CTX_OPCODES, *PASM_CTX_OPCODES;

typedef struct ASM_CTX
{
    PM68K_WORD                  Address;                                    // base address for the instruction (used by relative displacemens)
    PM68K_INSTRUCTION           Instruction;                                // output instruction
    PC_INSTRUCTION_TYPE_INFO    IInfo;                                      // information for the instruction
    PM68K_OPERAND               NextOperand;                                // next operand in the instruction
    PM68K_OPERAND               NextConvOperand;                            // next converted operand 
    ASM_CTX_OPCODES             Opcodes;                                    // information for the opcodes
    M68K_OPERAND                ConvOperands[M68K_MAXIMUM_NUMBER_OPERANDS]; // table of converted operands
    BANK_EXTENSION_INFO         BankExtensionInfo;                          // information for the bank extension word
    M68K_ARCHITECTURE           Architectures;                              // architectures that are supported
    M68K_ASM_FLAGS              Flags;                                      // flags for the assembler
    ASM_CTX_FLAGS               AFlags;                                     // internal flags for the assembler
    M68K_BOOL                   UsesPredecrement;                           // uses the predecrement addressing mode?
    M68K_BOOL                   UsesRegisterList;                           // uses a list of registers?
} ASM_CTX, *PASM_CTX;

typedef enum ASM_TEXT_OPERAND_TYPE
{
    // note: must be sorted in ascending order according to the text (see _M68KTextAsmOperandTypes)
    ATOT_UNKNOWN = 0,
    ATOT_ADDRESS_B,                     // address using byte displacement
    ATOT_ADDRESS_L,                     // address using word displacement
    ATOT_ADDRESS_W,                     // address using long displacement
    ATOT_CONDITION_CODE,                // condition code
    ATOT_COPROCESSOR_ID,                // coprocessor id
    ATOT_COPROCESSOR_ID_CONDITION_CODE, // coprocessor id and condition code
    ATOT_CACHE_TYPE,                    // cache type
    ATOT_DISPLACEMENT_B,                // byte displacement
    ATOT_DYNAMIC_KFACTOR,               // dynamic k-factor
    ATOT_DISPLACEMENT_L,                // word displacement
    ATOT_DISPLACEMENT_W,                // long displacement
    ATOT_FPU_CONDITION_CODE,            // fpu condition code
    ATOT_FPU_CONTROL_REGISTER_LIST,     // fpu control register list
    ATOT_FPU_REGISTER_LIST,             // fpu register list
    ATOT_IMMEDIATE_B,                   // immediate byte
    ATOT_IMMEDIATE_D,                   // immediate double
    ATOT_IMMEDIATE_L,                   // immediate long
    ATOT_IMMEDIATE_P,                   // immediate packed decimal
    ATOT_IMMEDIATE_Q,                   // immediate quad
    ATOT_IMMEDIATE_S,                   // immediate single
    ATOT_IMMEDIATE_W,                   // immediate word
    ATOT_IMMEDIATE_X,                   // immediate extended
    ATOT_MEMORY,                        // memory
    ATOT_MMU_CONDITION_CODE,            // mmu condition code
    ATOT_MEMORY_PRE_DECREMENT,          // memory with pre-decrement
    ATOT_MEMORY_POST_INCREMENT,         // memory with post-increment
    ATOT_MEMORY_PAIR,                   // memory pair
    ATOT_OFFSET_WIDTH,                  // offset and width
    ATOT_REGISTER,                      // register
    ATOT_REGISTER_LIST,                 // register list
    ATOT_REGISTER_PAIR,                 // register pair
    ATOT_STATIC_KFACTOR,                // static k-factor
    ATOT_VREGISTER_PAIR_M2,             // vregister pair multiple of 2
    ATOT_VREGISTER_PAIR_M4,             // vregister pair multiple of 4
    ATOT__SIZEOF__
} ASM_TEXT_OPERAND_TYPE, *PASM_TEXT_OPERAND_TYPE;

typedef struct ASM_TEXT_CTX
{
    PM68K_WORD          Address;        // base address for the instruction (used by relative displacemens)
    M68K_ASM_ERROR      Error;          // error information; also used to keep track of input
    M68K_INSTRUCTION    Instruction;    // instruction that will be assembled
} ASM_TEXT_CTX, *PASM_TEXT_CTX;

// disassembler
/*
    naming rules:

        <type>(_<detail>)?)_<source>

    type:
        C               check (should appear right after the instruction type)
        F               flags
        FPOP            fpu opmode
        I               ignore
        O               (next) operand
        S               (set) size

    detail C:
        VvvvvMmmmm      value vvvv applying mask mmmm

    detail F:
        AMode1B         addressing mode 1 (An/Bn) is allowed in instructions with size M68K_SIZE_B

    detail O:
        AC80AMode       apollo core 68080: all addressing modes; the address register is banked using one bit
        AC80AModeA      apollo core 68080: alterable addressing modes; the address register is banked using one bit
        AC80AReg        apollo core 68080: banked address register using one bit
        AC80BankEWSize  apollo core 68080: size of the instruction that is saved in the bank extension word
        AC80BReg        apollo core 68080: address register using bank 1 i.e. Bn
        AC80VAMode      apollo core 68080: vector addressing modes
        AC80VAModeA     apollo core 68080: alterable vector addressing modes
        AC80VReg        apollo core 68080: vector register i.e. d0-d7, e0-e23
        AC80VRegPM2     apollo core 68080: vector register pair multiple of 2
        AC80VRegPM4     apollo core 68080: vector register pair multiple of 4
        AbsL            absolute long addressing mode
        AMode           all addressing modes (An requires W or L)
        AModeA          alterable addressing modes (An requires W or L)
        AModeC          control addressing modes
        AModeCA         control alterable addressing modes
        AModeCAPD       control alterable or predecrement or postincrement addressing modes
        AModeCPD        control or predecrement addressing modes
        AModeCPDPI      control or predecrement or postincrement addressing modes
        AModeCPI        control or postincrement addressing modes
        AModeD          data addressing modes
        AModeDA         data alterable addressing modes
        AModeDAM        data alterable (memory only) addressing modes
        AModeDAPC       data alterable and program counter addressing modes (but only for 68020 or higher)
        AModeDRDCA      data register direct or control alterable addressing modes
        AModeIA         indirect address register addressing mode i.e. (An)
        AModeIAD        indirect address register with displacement addressing modes i.e. (d16, An)
        AModeMA         memory alterable addressing modes
        AModePD         pre-decrement addressing modes i.e. -(An)
        AModePFR        PFLUSHR addressing modes (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-42)
        AModePI         post-increment addressing modes i.e. (An)+
        AModeTOUCH      TOUCH addressing modes
        AModeTST        TST addressing modes (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 4-193)
        AReg            address register
        BACReg          BADx register
        BADReg          BADx register
        CC              condition code
        CC2H            condition code with value 2 or higher
        CPID            coprocessor id
        CPIDCC          coprocessor id and condition code
        CReg            control register (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-23)
        CT              cache type
        Disp            displacement
        DKFac           dynamic k-factor
        DReg            data register
        DReg_B1         banked data register using one bit (not allowed)
        DRegP           data register pair
        FC              function code i.e. sfc, dfc, immediate or data register (small differences for 68030 and 68851)
        FPCC            floating point condition code
        FPCRRegList     floating point control register list
        FPCRRegListCR   floating point control register list with FPCR
        FPCRRegListIAR  floating point control register list with FPIAR
        FPReg           floating point register
        FPRegIList      floating point inverted register list
        FPRegList       floating point register list
        ImmL            long immediate value
        ImmB            byte immediate value
        ImmB0           byte (0) immediate value
        ImmS            immediate value according to the instruction size (read from next word(s))
        ImmW            word immediate value
        MMUCC           mmu condition code
        MMUMask         mmu mask (3 bits for 68030 and 4 bits for 68851)
        MRegP           memory register pair
        OW              offset and width (applied to the previous operand)
        PReg            p-register for 68851 (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-55)
        RegNNN          NNN register
        Shift8          8-bit immediate value for shift operation: 0(=8) or 1-7
        SKFac           static k-factor

    detail S:
        B               byte
        BWL             byte (00), word (01) or long (10)
        FP              fpu size
        L               long
        P               packed decimal
        PReg            size according to p-register
        ST              short
        W               word
        WL              word (0) or long (1)
        X               extended

    source:
        EW1             All bits in extension word 1
        EW1EW2          All bits in extension word 1 and extension word 2
        I               Implicit
        NW              All bits in the next word(s)
        SnEmW0          Bits n-m in word 0
        SnEmEW1         Bits n-m in extension word 1
*/
typedef enum DISASM_ACTION_TYPE
{
    DAT_NONE,
    DAT_F_AMode1B,
    DAT_FPOP_S0E7EW1,
    DAT_O_AbsL_NW,
    DAT_O_AC80AMode_S0E5W0_S8E8EW1,
    DAT_O_AC80AModeA_S0E5W0,
    DAT_O_AC80AModeA_S0E5W0_S8E8EW1,
    DAT_O_AC80AReg_S12E14EW1_S7E7EW1,
    DAT_O_AC80BankEWSize_S6E7W0,
    DAT_O_AC80BReg_S0E2W0,
    DAT_O_AC80BReg_S9E11W0,
    DAT_O_AC80DReg_S12E14EW1_S7E7EW1,
    DAT_O_AC80VAMode_S0E5W0_S8E8W0,
    DAT_O_AC80VAModeA_S0E5W0_S8E8W0,
    DAT_O_AC80VReg_S0E3EW1_S8E8W0,
    DAT_O_AC80VReg_S12E15EW1_S7E7W0,
    DAT_O_AC80VReg_S8E11EW1_S6E6W0,
    DAT_O_AC80VRegPM2_S8E11EW1_S6E6W0,
    DAT_O_AC80VRegPM4_S0E3W0_S8E8W0,
    DAT_O_AMode_S0E5W0,
    DAT_O_AModeA_S0E5W0,
    DAT_O_AModeC_S0E5W0,
    DAT_O_AModeCA_S0E5W0,
    DAT_O_AModeCAPD_S0E5W0,
    DAT_O_AModeCAPDPI_S0E5W0,
    DAT_O_AModeCPD_S0E5W0,
    DAT_O_AModeCPDPI_S0E5W0,
    DAT_O_AModeCPI_S0E5W0,
    DAT_O_AModeD_S0E5W0,
    DAT_O_AModeDA_S0E5W0,
    DAT_O_AModeDA_S6E11W0,
    DAT_O_AModeDAM_S0E5W0,
    DAT_O_AModeDAPC_S0E5W0,
    DAT_O_AModeDRDCA_S0E5W0,
    DAT_O_AModeIA_S0E2W0,
    DAT_O_AModeIAD_S0E2W0,
    DAT_O_AModeMA_S0E5W0,
    DAT_O_AModePD_S0E2W0,
    DAT_O_AModePD_S0E5W0,
    DAT_O_AModePD_S9E11W0,
    DAT_O_AModePFR_S0E5W0,
    DAT_O_AModePI_S0E2W0,
    DAT_O_AModePI_S12E14W1,
    DAT_O_AModePI_S9E11W0,
    DAT_O_AModeTOUCH_S0E5W0,
    DAT_O_AModeTST_S0E5W0,
    DAT_O_AReg_S0E2W0,
    DAT_O_AReg_S0E2EW1,
    DAT_O_AReg_S12E14EW1,
    DAT_O_AReg_S5E7EW1,
    DAT_O_AReg_S9E11W0,
    DAT_O_BACReg_S2E4EW1,
    DAT_O_BADReg_S2E4EW1,
    DAT_O_CC_S8E11W0,
    DAT_O_CC2H_S8E11W0,
    DAT_O_CPID_S9E11W0,
    DAT_O_CPIDCC_S9E11W0_S0E5W0,
    DAT_O_CPIDCC_S9E11W0_S0E5EW1,
    DAT_O_CReg_S0E11EW1,
    DAT_O_CT_S6E7W0,
    DAT_O_DispB_S0E7W0,
    DAT_O_DispL_NW,
    DAT_O_DispW_NW,
    DAT_O_DKFac_S4E6EW1,
    DAT_O_DReg_S0E2W0,
    DAT_O_DReg_S0E2EW1,
    DAT_O_DReg_S12E14EW1,
    DAT_O_DReg_S4E6EW1,
    DAT_O_DReg_S6E8EW1,
    DAT_O_DReg_S9E11W0,
    DAT_O_DRegP_S0E2W0_S0E2EW1,
    DAT_O_DRegP_S0E2EW1_S12E14EW1,
    DAT_O_DRegP_S0E2EW1_S0E2EW2,
    DAT_O_DRegP_S6E8EW1_S6E8EW2,
    DAT_O_FC_S0E4EW1,
    DAT_O_FPCC_S0E4W0,
    DAT_O_FPCC_S0E4EW1,
    DAT_O_FPCRRegList_S10E12EW1,
    DAT_O_FPReg_S0E2EW1,
    DAT_O_FPReg_S7E9EW1,
    DAT_O_FPReg_S10E12EW1,
    DAT_O_FPRegIList_S0E7EW1,
    DAT_O_FPRegList_S0E7EW1,
    DAT_O_ImmB0_I,
    DAT_O_ImmB_S0E1W0,
    DAT_O_ImmB_S0E2W0,
    DAT_O_ImmB_S0E3W0,
    DAT_O_ImmB_S0E6EW1,
    DAT_O_ImmB_S0E7W0,
    DAT_O_ImmB_S0E7EW1,
    DAT_O_ImmB_S10E12EW1,
    DAT_O_ImmB_S2E3W0,
    DAT_O_ImmB_S8E9EW1,
    DAT_O_ImmB_S9E11W0,
    DAT_O_ImmB_S9E11W0_S4E5W0,
    DAT_O_ImmL_NW,
    DAT_O_ImmS,
    DAT_O_ImmW_S0E11EW1,
    DAT_O_ImmW_NW,
    DAT_O_MMUCC_S0E3W0,
    DAT_O_MMUCC_S0E3EW1,
    DAT_O_MMUMask_S5E8EW1,
    DAT_O_MRegP_S12E15EW1_S12E15EW2,
    DAT_O_OW_S0E11EW1,
    DAT_O_PReg_S10E12EW1,
    DAT_O_RegAC0_I,
    DAT_O_RegAC1_I,
    DAT_O_RegACUSR_I,
    DAT_O_RegCCR_I,
    DAT_O_RegCRP_I,
    DAT_O_RegFPCR_I,
    DAT_O_RegFPIAR_I,
    DAT_O_RegFPSR_I,
    DAT_O_RegList_EW1,
    DAT_O_RegPCSR_I,
    DAT_O_RegPSR_I,
    DAT_O_RegSR_I,
    DAT_O_RegSRP_I,
    DAT_O_RegTC_I,
    DAT_O_RegTT0_I,
    DAT_O_RegTT1_I,
    DAT_O_RegUSP_I,
    DAT_O_RegVAL_I,
    DAT_O_Shift8_S9E11W0,
    DAT_O_SKFac_S0E6EW1,
    DAT_S_B_I,
    DAT_S_BWL_S6E7W0,
    DAT_S_BWL_S6E7EW1,
    DAT_S_BWL_S9E10W0,
    DAT_S_FP_S10E12EW1,
    DAT_S_L_I,
    DAT_S_P_I,
    DAT_S_PReg_S10E12EW1,
    DAT_S_Q_I,
    DAT_S_ST_I,
    DAT_S_W_I,
    DAT_S_WL_S6E6W0,
    DAT_S_WL_S8E8W0,
    DAT_S_X_I,
    // keep the values and masks at the end
    DAT_C__VMSTART__,
    DAT_C__VMEW1START__ = DAT_C__VMSTART__,
    DAT_C_V0000M00f0_EW1 = DAT_C__VMEW1START__,
    DAT_C_V0000M0e38_EW1,
    DAT_C_V0000M8000_EW1,
    DAT_C_V0000M8e7f_EW1,
    DAT_C_V0000M8f38_EW1,
    DAT_C_V0000M8ff8_EW1,
    DAT_C_V0000M8fff_EW1,
    DAT_C_V0000Me000_EW1,
    DAT_C_V0000Mf000_EW1,
    DAT_C_V0000Mfe1c_EW1,
    DAT_C_V0000Mff00_EW1,
    DAT_C_V0000Mffc0_EW1,
    DAT_C_V0000Mffe0_EW1,
    DAT_C_V0000Mfff0_EW1,
    DAT_C_V0000Mffff_EW1,
    DAT_C_V0001Mf0ff_EW1,
    DAT_C_V0002Mf0ff_EW1,
    DAT_C_V0003Mf0ff_EW1,
    DAT_C_V0004M0fff_EW1,
    DAT_C_V0005M00ff_EW1,
    DAT_C_V0006M00ff_EW1,
    DAT_C_V0007M00ff_EW1,
    DAT_C_V0008M00ff_EW1,
    DAT_C_V0009M00ff_EW1,
    DAT_C_V000aM00ff_EW1,
    DAT_C_V000bM00ff_EW1,
    DAT_C_V000cM00ff_EW1,
    DAT_C_V000eM00ff_EW1,
    DAT_C_V000fM00ff_EW1,
    DAT_C_V0010M00ff_EW1,
    DAT_C_V0010M8e7f_EW1,
    DAT_C_V0011M00ff_EW1,
    DAT_C_V0012M00ff_EW1,
    DAT_C_V0013M00ff_EW1,
    DAT_C_V0014M00ff_EW1,
    DAT_C_V0015M00ff_EW1,
    DAT_C_V0016M00ff_EW1,
    DAT_C_V0017M00ff_EW1,
    DAT_C_V0018M00ff_EW1,
    DAT_C_V0019M00ff_EW1,
    DAT_C_V001aM00ff_EW1,
    DAT_C_V001bM00ff_EW1,
    DAT_C_V001cM00ff_EW1,
    DAT_C_V001dM00ff_EW1,
    DAT_C_V001eMf0ff_EW1,
    DAT_C_V0020M00ff_EW1,
    DAT_C_V0021M00ff_EW1,
    DAT_C_V0022M00ff_EW1,
    DAT_C_V0023M00ff_EW1,
    DAT_C_V0024M00ff_EW1,
    DAT_C_V0025M00ff_EW1,
    DAT_C_V0026M0cff_EW1,
    DAT_C_V0028Mf0ff_EW1,
    DAT_C_V0029M00ff_EW1,
    DAT_C_V002aMf0ff_EW1,
    DAT_C_V002cM00ff_EW1,
    DAT_C_V002dM00ff_EW1,
    DAT_C_V002eM00ff_EW1,
    DAT_C_V002fM00ff_EW1,
    DAT_C_V0030Me078_EW1,
    DAT_C_V0030M00ff_EW1,
    DAT_C_V0031M00ff_EW1,
    DAT_C_V0032M00ff_EW1,
    DAT_C_V0033M00ff_EW1,
    DAT_C_V0034M00ff_EW1,
    DAT_C_V0035M00ff_EW1,
    DAT_C_V0036M00ff_EW1,
    DAT_C_V0037M00ff_EW1,
    DAT_C_V0038M00ff_EW1,
    DAT_C_V0039M00ff_EW1,
    DAT_C_V003aMe07f_EW1,
    DAT_C_V0100M8f38_EW1,
    DAT_C_V0104M0fff_EW1,
    DAT_C_V01c0Mffff_EW1,
    DAT_C_V0400M8f38_EW1,
    DAT_C_V0400M8ff8_EW1,
    DAT_C_V0500M8f38_EW1,
    DAT_C_V0800M8e7f_EW1,
    DAT_C_V0800M8f38_EW1,
    DAT_C_V0800M8ff8_EW1,
    DAT_C_V0800M8fff_EW1,
    DAT_C_V0800Mffff_EW1,
    DAT_C_V0810M8e7f_EW1,
    DAT_C_V0900M8f38_EW1,
    DAT_C_V0900Mffff_EW1,
    DAT_C_V0a00Mffff_EW1,
    DAT_C_V0c00M8f38_EW1,
    DAT_C_V0c00M8ff8_EW1,
    DAT_C_V0c00Mffff_EW1,
    DAT_C_V0d00M8f38_EW1,
    DAT_C_V0d00Mffff_EW1,
    DAT_C_V0e00Mffff_EW1,
    DAT_C_V1001Mf0ff_EW1,
    DAT_C_V1003Mf0ff_EW1,
    DAT_C_V2000Mffe0_EW1,
    DAT_C_V2200Mffe0_EW1,
    DAT_C_V2400Mffff_EW1,
    DAT_C_V2800Mffff_EW1,
    DAT_C_V2c00Mfff8_EW1,
    DAT_C_V3000Mfe00_EW1,
    DAT_C_V3400Mfe00_EW1,
    DAT_C_V3800Mfe00_EW1,
    DAT_C_V3c00Mfe00_EW1,
    DAT_C_V4000Me000_EW1,
    DAT_C_V4000Me3ff_EW1,
    DAT_C_V4000Mffff_EW1,
    DAT_C_V4030Me078_EW1,
    DAT_C_V403aMe07f_EW1,
    DAT_C_V4100Mffff_EW1,
    DAT_C_V4200Me3ff_EW1,
    DAT_C_V4200Mffff_EW1,
    DAT_C_V4800Mffff_EW1,
    DAT_C_V4900Mffff_EW1,
    DAT_C_V4a00Mffff_EW1,
    DAT_C_V4c00Mffff_EW1,
    DAT_C_V4d00Mffff_EW1,
    DAT_C_V4e00Mffff_EW1,
    DAT_C_V5c00Mfc00_EW1,
    DAT_C_V6000Me07f_EW1,
    DAT_C_V6000Mffff_EW1,
    DAT_C_V6200Mffff_EW1,
    DAT_C_V6400Mffff_EW1,
    DAT_C_V6600Mffff_EW1,
    DAT_C_V6c00Mfc00_EW1,
    DAT_C_V7000Mffe3_EW1,
    DAT_C_V7200Mffe3_EW1,
    DAT_C_V7400Mffe3_EW1,
    DAT_C_V7600Mffe3_EW1,
    DAT_C_V7c00Mfc0f_EW1,
    DAT_C_V8000M8000_EW1,
    DAT_C_V8000M8e7f_EW1,
    DAT_C_V8000M8fff_EW1,
    DAT_C_V8000Me1ff_EW1,
    DAT_C_V8000Me3e0_EW1,
    DAT_C_V8000Mffe0_EW1,
    DAT_C_V8010M8e7f_EW1,
    DAT_C_V8100Me300_EW1,
    DAT_C_V8200Me3e0_EW1,
    DAT_C_V8300Me300_EW1,
    DAT_C_V8400Mffff_EW1,
    DAT_C_V8800M8e7f_EW1,
    DAT_C_V8800M8fff_EW1,
    DAT_C_V8800Mffff_EW1,
    DAT_C_V8810M8e7f_EW1,
    DAT_C_V9000Mffff_EW1,
    DAT_C_Va000Me1ff_EW1,
    DAT_C_Va000Mffff_EW1,
    DAT_C_Va400Mffff_EW1,
    DAT_C_Va800Mffff_EW1,
    DAT_C_Vb000Mffff_EW1,
    DAT_C_Vd000Mff00_EW1,
    DAT_C_Vd800Mff8f_EW1,
    DAT_C_Ve000Mff00_EW1,
    DAT_C_Ve800Mff8f_EW1,
    DAT_C_Vf000Mff00_EW1,
    DAT_C_Vf800Mff8f_EW1,
    DAT_C__VMEW2START__,
    DAT_C_V0000M0e38_EW2 = DAT_C__VMEW2START__,
    DAT_C__VMEND__,
} DISASM_ACTION_TYPE, *PDISASM_ACTION_TYPE;

typedef M68K_BYTE DISASM_ACTION_TYPE_VALUE, *PDISASM_ACTION_TYPE_VALUE;

typedef enum DISASM_CTX_FLAGS
{
    DCF_EXTENSION_WORD_1    = 0x0001,   // first extension word was already read
    DCF_EXTENSION_WORD_2    = 0x0002,   // second extension word was already read
    DCF_AMODE1B             = 0x0004,   // addressing mode 1 (An/Bn) is allowed in instructions with size M68K_SIZE_B
} DISASM_CTX_FLAGS, *PDISASM_CTX_FLAGS;

typedef M68K_WORD DISASM_CTX_FLAGS_VALUE, *PDISASM_CTX_FLAGS_VALUE;

typedef struct DISASM_CTX_INPUT
{
    PM68K_WORD  Start;  // first word in the input
    PM68K_WORD  End;    // upper limit for the input (can't read from any address >= End)
} DISASM_CTX_INPUT, *PDISASM_CTX_INPUT;

typedef struct DISASM_CTX_OPCODES
{
    M68K_WORD   NumberWords;    // total number of words used by the instruction
    M68K_WORD   Word;           // first word of the opcode
    M68K_WORD   EWord1;         // second word i.e. first extension word of the opcode
    M68K_WORD   EWord2;         // third word i.e. second extension word of the opcode
    M68K_BYTE   PCOffset;       // offset in bytes (from the start of the instruction) that is used to calculate final addresses
} DISASM_CTX_OPCODES, *PDISASM_CTX_OPCODES;

typedef struct DISASM_CTX_VALUES
{
    PM68K_VALUE_LOCATION        NextValue;              // pointer to the next value that we need to return; M68K_NULL = not used
    M68K_VALUE_OPERANDS_MASK    OperandsMask;           // mask of operands tht can be used to save values
    M68K_UINT                   MaximumNumberValues;    // maximum numebr of values that we can save
    M68K_UINT                   TotalNumberValues;      // total number of values that we can save
} DISASM_CTX_VALUES, *PDISASM_CTX_VALUES;

typedef struct DISASM_CTX
{
    PM68K_WORD                  Address;                    // base address for the instruction (used by relative displacemens)
    PM68K_INSTRUCTION           Instruction;                // output instruction
    PC_INSTRUCTION_TYPE_INFO    IInfo;                      // information for the instruction
    DISASM_CTX_INPUT            Input;                      // information for the input
    PM68K_OPERAND               NextOperand;                // next operand in the instruction
    DISASM_CTX_OPCODES          Opcodes;                    // information for the opcodes
    DISASM_CTX_VALUES           Values;                     // information for the values
    M68K_ARCHITECTURE           Architectures;              // architectures that are supported
    M68K_DISASM_FLAGS           Flags;                      // flags
    M68K_INSTRUCTION_TYPE       MasterType;                 // master instruction type
    INSTRUCTION_TYPE            IType;                      // instruction type
    DISASM_CTX_FLAGS            DFlags;                     // internal flags for the disassembler
    M68K_BOOL                   UsesPredecrement;           // uses the predecrement addressing mode?
    M68K_BYTE                   IndexOperandRegisterList;   // index of the operand that saves the register list
} DISASM_CTX, *PDISASM_CTX;

typedef struct DISASM_TEXT_OUTPUT
{
    PM68K_CHAR  Buffer;     // next char in the output buffer
    M68K_LUINT  AvailSize;  // number of available chars
    M68K_LUINT  TotalSize;  // total number of chars required by the text
} DISASM_TEXT_OUTPUT, *PDISASM_TEXT_OUTPUT;

typedef struct DISASM_TEXT_CONTEXT
{
    PM68K_WORD                      Address;            // base address for the instruction (used by relative displacemens)
    PM68K_DISASSEMBLE_TEXT_FUNCTION DisTextFunc;        // custom function
    PM68K_VOID                      DisTextFuncData;    // data for the custom function
    PM68K_INSTRUCTION               Instruction;        // instruction being converted to text
    DISASM_TEXT_OUTPUT              Output;             // information for the output
    M68K_DISASM_TEXT_FLAGS_VALUE    TextFlags;          // flags for the text
    M68K_DISASM_TEXT_INFO           TextInfo;           // structure used to communicate with the custom function
} DISASM_TEXT_CONTEXT, *PDISASM_TEXT_CONTEXT;

// instruction in the binary file i.e. the file that we can generate with all instructions
#define BINARY_INSTR_MINIMUM_NUMBER_WORDS   (BINARY_INSTR_WINDEX_OPCODES + 1)
#define BINARY_INSTR_WINDEX_NUMBER_WORDS    0
#define BINARY_INSTR_WINDEX_TYPE            1
#define BINARY_INSTR_WINDEX_ARCHITECTURES   2
#define BINARY_INSTR_WINDEX_OPCODES         4

typedef struct BINARY_INSTR_HEADER
{
    M68K_WORD   NumberWords;    // number of words
    M68K_WORD   Type;           // instruction type
    M68K_DWORD  Architectures;  // architectures for the instruction
    /*
    M68K_WORD   Opcodes[NumberWords];
    */
} BINARY_INSTR_HEADER, *PBINARY_INSTR_HEADER;

// m68kasm_text.c
extern M68K_REGISTER_TYPE_VALUE     _M68KAsmTextBinarySearchRegister(PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd);
extern M68K_UINT                    _M68KAsmTextBinarySearchText(PM68KC_STR Table[], M68K_UINT Min, M68K_UINT Max, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd);
extern M68K_BOOL                    _M68KAsmTextCheckFixImmediateSize(M68K_SDWORD SValue, PM68K_SIZE_VALUE Size);
extern M68K_INSTRUCTION_TYPE_VALUE  _M68KAsmTextCheckMnemonic(PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd);
extern M68K_CHAR                    _M68KAsmTextConvertToLowerCase(M68K_CHAR Char);

// m68kasm_tables.c
extern M68KC_WORD   _M68KAsmIndexFirstWord[M68K_IT__SIZEOF__ - 1];
extern M68KC_WORD   _M68KAsmOpcodeMaps[M68K_IT__SIZEOF__ - 1];
extern M68KC_WORD   _M68KAsmWords[];

// m68kcc_tables.c
extern M68KC_CONDITION_CODE_FLAG_ACTIONS _M68KConditionCodeFlagActions[CCF__SIZEOF__];

// m68kctl_tabels.c
extern C_CONTROL_REGISTER   _M68KControlRegisters[];

// m68kdis.c
extern void         _M68KDisGetBankExtensionInfo(M68K_WORD BankExtensionWord, PBANK_EXTENSION_INFO BankExtensionInfo);
extern M68K_BYTE    _M68KDisGetImplicitPCOffset(M68K_INSTRUCTION_TYPE InstrType);
extern M68K_WORD    _M68KDisInvertWordBits(M68K_WORD Word);
extern M68K_WORD    _M68KDisReadWord(PDISASM_CTX_INPUT DInput, M68K_UINT Index);
extern M68K_DWORD   _M68KDisReadDWord(PDISASM_CTX_INPUT DInput, M68K_UINT Index);

// m68kdis_tables.c
extern M68KC_WORD               _M68KDisOpcodeMap0[];
extern M68KC_WORD               _M68KDisOpcodeMap1[];
extern M68KC_WORD               _M68KDisOpcodeMap2[];
extern M68KC_WORD               _M68KDisOpcodeMap3[];
extern M68KC_WORD               _M68KDisOpcodeMap4[];
extern M68KC_WORD               _M68KDisOpcodeMap5[];
extern M68KC_WORD               _M68KDisOpcodeMap6[];
extern M68KC_WORD               _M68KDisOpcodeMap7[];
extern M68KC_WORD               _M68KDisOpcodeMap8[];
extern M68KC_WORD               _M68KDisOpcodeMap9[];
extern M68KC_WORD               _M68KDisOpcodeMap10[];
extern M68KC_WORD               _M68KDisOpcodeMap11[];
extern M68KC_WORD               _M68KDisOpcodeMap12[];
extern M68KC_WORD               _M68KDisOpcodeMap13[];
extern M68KC_WORD               _M68KDisOpcodeMap14[];
extern M68KC_WORD               _M68KDisOpcodeMap15[];
extern M68KC_WORD               _M68KDisOpcodeMap16[];
extern PM68KC_WORD M68K_CONST   _M68KDisOpcodeMaps[16];
extern C_VALUE_MASK             _M68KDisValueMasks[DAT_C__VMEND__ - DAT_C__VMSTART__];

// m68kfpu_tables.c
extern M68KC_SIZE_VALUE _M68KFpuSizes[8];
extern M68KC_BYTE       _M68KFpuSizeCodes[M68K_SIZE__SIZEOF__];

// m68kieee_tables.c
extern M68K_CONST IEEE_VALUE_DOUBLE    _M68KIEEEDoubleValues[M68K_IEEE_VT__SIZEOF__];
extern M68K_CONST IEEE_VALUE_EXTENDED  _M68KIEEEExtendedValues[M68K_IEEE_VT__SIZEOF__];
extern M68K_CONST IEEE_VALUE_SINGLE    _M68KIEEESingleValues[M68K_IEEE_VT__SIZEOF__];

// m68kinstr_tables.c
extern C_INSTRUCTION_FPU_OPMODE_INFO    _M68KInstrFPUOpmodes[128];
extern C_INSTRUCTION_MASTER_FLAGS_VALUE _M68KInstrMasterTypeFlags[M68K_IT__SIZEOF__];
extern M68KC_BYTE                       _M68KInstrMasterTypeFPUOpmodes[M68K_IT__SIZEOF__];
extern C_INSTRUCTION_TYPE_INFO          _M68KInstrTypeInfos[IT__SIZEOF__];

// m68kmmu_tables.c
extern M68KC_REGISTER_TYPE_VALUE    _M68KMMUPRegisters[8];
extern M68KC_SIZE_VALUE             _M68KMMUPRegisterSizes[8];

// m68ktexts.c
extern PM68KC_STR                   _M68KTextAsmErrorTypes[M68K_AET__SIZEOF__];
extern PM68KC_STR                   _M68KTextAsmOperandTypes[ATOT__SIZEOF__];
extern PM68KC_STR                   _M68KTextCacheTypes[M68K_CT__SIZEOF__];
extern PM68KC_STR                   _M68KTextConditionCodes[M68K_CC__SIZEOF__];
extern PM68KC_STR                   _M68KTextFpuConditionCodes[M68K_FPCC__SIZEOF__];
extern M68KC_CHAR                   _M68KTextHexChars[16];
extern PM68KC_STR                   _M68KTextIEEEValues[M68K_IEEE_VT__SIZEOF__];
extern PM68KC_STR                   _M68KTextIEEEValuesXL[M68K_IEEE_VT__SIZEOF__];
extern PM68KC_STR                   _M68KTextMMUConditionCodes[M68K_MMUCC__SIZEOF__];
extern PM68KC_STR                   _M68KTextMnemonics[M68K_IT__SIZEOF__];
extern PM68KC_STR                   _M68KTextRegisters[M68K_RT__SIZEOF__];
extern M68K_CHAR                    _M68KTextScaleChars[M68K_SCALE__SIZEOF__];
extern M68K_CHAR                    _M68KTextSizeChars[M68K_SIZE__SIZEOF__];
extern M68KC_REGISTER_TYPE_VALUE    _M68KTextSortedRegisters[M68K_RT__SIZEOF__];

#endif
