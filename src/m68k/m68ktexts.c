#include "m68kinternal.h"

// table of texts for each error type
PM68KC_STR _M68KTextAsmErrorTypes[M68K_AET__SIZEOF__] =
{
    ""                                                                      , // M68K_AET_NONE
    "unexpected char"                                                       , // M68K_AET_UNEXPECTED_CHAR
    "invalid mnemonic"                                                      , // M68K_AET_INVALID_MNEMONIC
    "invalid size"                                                          , // M68K_AET_INVALID_SIZE
    "too many operands"                                                     , // M68K_AET_TOO_MANY_OPERANDS
    "invalid operand type"                                                  , // M68K_AET_INVALID_OPERAND_TYPE
    "operand type is not supported"                                         , // M68K_AET_OPERAND_TYPE_NOT_SUPPORTED
    "overflow when converting a decimal value"                              , // M68K_AET_DECIMAL_OVERFLOW
    "overflow when converting an hexadecimal value"                         , // M68K_AET_HEXADECIMAL_OVERFLOW
    "value is out of range"                                                 , // M68K_AET_VALUE_OUT_OF_RANGE
    "invalid condition code"                                                , // M68K_AET_INVALID_CONDITION_CODE
    "invalid register"                                                      , // M68K_AET_INVALID_REGISTER
    "register is not supported"                                             , // M68K_AET_REGISTER_NOT_SUPPORTED
    "invalid cache type"                                                    , // M68K_AET_INVALID_CACHE_TYPE
    "one or more registers have already been added to the list"             , // M68K_AET_REGISTERS_ALREADY_IN_LIST
    "invalid range of registers"                                            , // M68K_AET_INVALID_RANGE_REGISTERS
    "invalid ieee value"                                                    , // M68K_AET_INVALID_IEEE_VALUE
    "ieee value is not supported"                                           , // M68K_AET_IEEE_VALUE_NOT_SUPPORTED
    "invalid exponent"                                                      , // M68K_AET_INVALID_EXPONENT
    "an address has already been specified for the memory operand"          , // M68K_AET_MEMORY_ADDRESS_ALREADY_SPECIFIED
    "a base register has already been specified for the memory operand"     , // M68K_AET_MEMORY_BASE_ALREADY_SPECIFIED
    "an index register has already been specified for the memory operand"   , // M68K_AET_MEMORY_INDEX_ALREADY_SPECIFIED
    "register can not be used as a base register for the memory operand"    , // M68K_AET_INVALID_REGISTER_MEMORY_BASE
    "register can not be used as a index register for the memory operand"   , // M68K_AET_INVALID_REGISTER_MEMORY_INDEX
    "a displacement was already specified for the memory operand"           , // M68K_AET_MEMORY_INNER_DISP_ALREADY_SPECIFIED
    "a displacement was already specified for the memory operand"           , // M68K_AET_MEMORY_OUTER_DISP_ALREADY_SPECIFIED
    "invalid memory operand"                                                , // M68K_AET_INVALID_MEMORY_OPERAND
    "invalid instruction"                                                   , // M68K_AET_INVALID_INSTRUCTION
};

// table of texts for the operand types
PM68KC_STR _M68KTextAsmOperandTypes[ATOT__SIZEOF__] =
{
    M68K_NULL,  // ATOT_UNKNOWN
    "ab",       // ATOT_ADDRESS_B
    "al",       // ATOT_ADDRESS_L
    "aw",       // ATOT_ADDRESS_W
    "cc",       // ATOT_CONDITION_CODE
    "cid",      // ATOT_COPROCESSOR_ID
    "cidcc",    // ATOT_COPROCESSOR_ID_CONDITION_CODE
    "ct",       // ATOT_CACHE_TYPE
    "db",       // ATOT_DISPLACEMENT_B
    "dkf",      // ATOT_DYNAMIC_KFACTOR
    "dl",       // ATOT_DISPLACEMENT_L
    "dw",       // ATOT_DISPLACEMENT_W
    "fcc",      // ATOT_FPU_CONDITION_CODE
    "fcl",      // ATOT_FPU_CONTROL_REGISTER_LIST
    "fl",       // ATOT_FPU_REGISTER_LIST
    "ib",       // ATOT_IMMEDIATE_B
    "id",       // ATOT_IMMEDIATE_D
    "il",       // ATOT_IMMEDIATE_L
    "ip",       // ATOT_IMMEDIATE_P
    "iq",       // ATOT_IMMEDIATE_Q
    "is",       // ATOT_IMMEDIATE_S
    "iw",       // ATOT_IMMEDIATE_W
    "ix",       // ATOT_IMMEDIATE_X
    "m",        // ATOT_MEMORY
    "mcc",      // ATOT_MMU_CONDITION_CODE
    "md",       // ATOT_MEMORY_PRE_DECREMENT
    "mi",       // ATOT_MEMORY_POST_INCREMENT
    "mp",       // ATOT_MEMORY_PAIR
    "ow",       // ATOT_OFFSET_WIDTH
    "r",        // ATOT_REGISTER
    "rl",       // ATOT_REGISTER_LIST
    "rp",       // ATOT_REGISTER_PAIR
    "skf",      // ATOT_STATIC_KFACTOR
    "vrp2",     // ATOT_VREGISTER_PAIR_M2
    "vrp4",     // ATOT_VREGISTER_PAIR_M4
};

// table of texts for the cache types
PM68KC_STR _M68KTextCacheTypes[M68K_CT__SIZEOF__] =
{
    "nc",   // M68K_CT_NONE
    "dc",   // M68K_CT_DATA
    "ic",   // M68K_CT_INSTRUCTION
    "bc",   // M68K_CT_BOTH
};

// table of texts for the condition codes
PM68KC_STR _M68KTextConditionCodes[M68K_CC__SIZEOF__] = 
{
    "t",    // M68K_CC_T
    "f",    // M68K_CC_F
    "hi",   // M68K_CC_HI
    "ls",   // M68K_CC_LS
    "cc",   // M68K_CC_CC
    "cs",   // M68K_CC_CS
    "ne",   // M68K_CC_NE
    "eq",   // M68K_CC_EQ
    "vc",   // M68K_CC_VC
    "vs",   // M68K_CC_VS
    "pl",   // M68K_CC_PL
    "mi",   // M68K_CC_MI
    "ge",   // M68K_CC_GE
    "lt",   // M68K_CC_LT
    "gt",   // M68K_CC_GT
    "le",   // M68K_CC_LE
};

// table of texts for the fpu condition codes
PM68KC_STR _M68KTextFpuConditionCodes[M68K_FPCC__SIZEOF__] =
{
    "f",        // M68K_FPCC_F
    "eq",       // M68K_FPCC_EQ
    "ogt",      // M68K_FPCC_OGT
    "oge",      // M68K_FPCC_OGE
    "olt",      // M68K_FPCC_OLT
    "ole",      // M68K_FPCC_OLE
    "olg",      // M68K_FPCC_OLG
    "or",       // M68K_FPCC_OR
    "un",       // M68K_FPCC_UN
    "ueq",      // M68K_FPCC_UEQ
    "ugt",      // M68K_FPCC_UGT
    "uge",      // M68K_FPCC_UGE
    "ult",      // M68K_FPCC_ILT
    "ule",      // M68K_FPCC_ULE
    "ne",       // M68K_FPCC_NE
    "t",        // M68K_FPCC_T
    "sf",       // M68K_FPCC_SF
    "seq",      // M68K_FPCC_SEQ
    "gt",       // M68K_FPCC_GT
    "ge",       // M68K_FPCC_GE
    "lt",       // M68K_FPCC_LT
    "le",       // M68K_FPCC_LE
    "gl",       // M68K_FPCC_GL
    "gle",      // M68K_FPCC_GLE
    "ngle",     // M68K_FPCC_NGLE
    "ngl",      // M68K_FPCC_NGL
    "nle",      // M68K_FPCC_NLE
    "nlt",      // M68K_FPCC_NLT
    "nge",      // M68K_FPCC_NGE
    "ngt",      // M68K_FPCC_NGT
    "sne",      // M68K_FPCC_SNE
    "st",       // M68K_FPCC_ST
};

// table of hex chars
M68KC_CHAR _M68KTextHexChars[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

// table of ieee values
PM68KC_STR _M68KTextIEEEValues[M68K_IEEE_VT__SIZEOF__] =
{
    "Ind",  // IEEE_IND
    "Inf",  // IEEE_INF
    "PInf", // IEEE_PINF
    "PNaN", // IEEE_PNAN
    "QNaN", // IEEE_QNAN
    "NaN",  // IEEE_NAN
    "0",    // IEEE_ZERO
};

// table of ieee values for the XL language
PM68KC_STR _M68KTextIEEEValuesXL[M68K_IEEE_VT__SIZEOF__] =
{
    "IEEE_IND",     // IEEE_IND
    "IEEE_INF",     // IEEE_INF
    "IEEE_PINF",    // IEEE_PINF
    "IEEE_PNAN",    // IEEE_PNAN
    "IEEE_QNAN",    // IEEE_QNAN
    "IEEE_NAN",     // IEEE_NAN
    "IEEE_ZERO",    // IEEE_ZERO
};

// table of texts for the mmu condition codes
PM68KC_STR _M68KTextMMUConditionCodes[M68K_MMUCC__SIZEOF__] =
{
    "bs",   // M68K_MMUCC_BS
    "bc",   // M68K_MMUCC_BC
    "ls",   // M68K_MMUCC_LS
    "lc",   // M68K_MMUCC_LC
    "ss",   // M68K_MMUCC_SS
    "sc",   // M68K_MMUCC_SC
    "as",   // M68K_MMUCC_AS
    "ac",   // M68K_MMUCC_AC
    "ws",   // M68K_MMUCC_WS
    "wc",   // M68K_MMUCC_WC
    "is",   // M68K_MMUCC_IS
    "ic",   // M68K_MMUCC_IC
    "gs",   // M68K_MMUCC_GS
    "gc",   // M68K_MMUCC_GC
    "cs",   // M68K_MMUCC_CS
    "cc",   // M68K_MMUCC_CC
};

// table with the text of each mnemonics alias
PM68KC_STR _M68KTextMnemonicAliases[MAT__SIZEOF__] =
{
    "bcc",         // MAT_BCC
    "bcs",         // MAT_BCS
    "beq",         // MAT_BEQ
    "bge",         // MAT_BGE
    "bgt",         // MAT_BGT
    "bhi",         // MAT_BHI
    "bhs",         // MAT_BHS
    "ble",         // MAT_BLE
    "blo",         // MAT_BLO
    "bls",         // MAT_BLS
    "blt",         // MAT_BLT
    "bmi",         // MAT_BMI
    "bne",         // MAT_BNE
    "bpl",         // MAT_BPL
    "bvc",         // MAT_BVC
    "bvs",         // MAT_BVS

    "dbcc",        // MAT_DBCC
    "dbcs",        // MAT_DBCS
    "dbeq",        // MAT_DBEQ
    "dbf",         // MAT_DBF
    "dbge",        // MAT_DBGE
    "dbgt",        // MAT_DBGT
    "dbhi",        // MAT_DBHI
    "dbhs",        // MAT_DBHS
    "dble",        // MAT_DBLE
    "dblo",        // MAT_DBLO
    "dbls",        // MAT_DBLS
    "dblt",        // MAT_DBLT
    "dbmi",        // MAT_DBMI
    "dbne",        // MAT_DBNE
    "dbpl",        // MAT_DBPL
    "dbra",        // MAT_DBRA
    "dbt",         // MAT_DBT
    "dbvc",        // MAT_DBVC
    "dbvs",        // MAT_DBVS
    
    "fbeq",        // MAT_FBEQ
    "fbf",         // MAT_FBF
    "fbge",        // MAT_FBGE
    "fbgl",        // MAT_FBGL
    "fbgle",       // MAT_FBGLE
    "fbgt",        // MAT_FBGT
    "fble",        // MAT_FBLE
    "fblt",        // MAT_FBLT
    "fbne",        // MAT_FBNE
    "fbnge",       // MAT_FBNGE
    "fbngl",       // MAT_FBNGL
    "fbngle",      // MAT_FBNGLE
    "fbngt",       // MAT_FBNGT
    "fbnle",       // MAT_FBNLE
    "fbnlt",       // MAT_FBNLT
    "fboge",       // MAT_FBOGE
    "fbogt",       // MAT_FBOGT
    "fbole",       // MAT_FBOLE
    "fbolg",       // MAT_FBOLG
    "fbolt",       // MAT_FBOLT
    "fbor",        // MAT_FBOR
    "fbseq",       // MAT_FBSEQ
    "fbsf",        // MAT_FBSF
    "fbsne",       // MAT_FBSNE
    "fbst",        // MAT_FBST
    "fbt",         // MAT_FBT
    "fbueq",       // MAT_FBUEQ
    "fbuge",       // MAT_FBUGE
    "fbugt",       // MAT_FBUGT
    "fbule",       // MAT_FBULE
    "fbult",       // MAT_FBULT
    "fbun",        // MAT_FBUN
    
    "fdbeq",       // MAT_FDBEQ
    "fdbf",        // MAT_FDBF
    "fdbge",       // MAT_FDBGE
    "fdbgl",       // MAT_FDBGL
    "fdbgle",      // MAT_FDBGLE
    "fdbgt",       // MAT_FDBGT
    "fdble",       // MAT_FDBLE
    "fdblt",       // MAT_FDBLT
    "fdbne",       // MAT_FDBNE
    "fdbnge",      // MAT_FDBNGE
    "fdbngl",      // MAT_FDBNGL
    "fdbngle",     // MAT_FDBNGLE
    "fdbngt",      // MAT_FDBNGT
    "fdbnle",      // MAT_FDBNLE
    "fdbnlt",      // MAT_FDBNLT
    "fdboge",      // MAT_FDBOGE
    "fdbogt",      // MAT_FDBOGT
    "fdbole",      // MAT_FDBOLE
    "fdbolg",      // MAT_FDBOLG
    "fdbolt",      // MAT_FDBOLT
    "fdbor",       // MAT_FDBOR
    "fdbseq",      // MAT_FDBSEQ
    "fdbsf",       // MAT_FDBSF
    "fdbsne",      // MAT_FDBSNE
    "fdbst",       // MAT_FDBST
    "fdbt",        // MAT_FDBT
    "fdbueq",      // MAT_FDBUEQ
    "fdbuge",      // MAT_FDBUGE
    "fdbugt",      // MAT_FDBUGT
    "fdbule",      // MAT_FDBULE
    "fdbult",      // MAT_FDBULT
    "fdbun",       // MAT_FDBUN
    
    "fseq",        // MAT_FSEQ
    "fsf",         // MAT_FSF
    "fsge",        // MAT_FSGE
    "fsgl",        // MAT_FSGL
    "fsgle",       // MAT_FSGLE
    "fsgt",        // MAT_FSGT
    "fsle",        // MAT_FSLE
    "fslt",        // MAT_FSLT
    "fsne",        // MAT_FSNE
    "fsnge",       // MAT_FSNGE
    "fsngl",       // MAT_FSNGL
    "fsngle",      // MAT_FSNGLE
    "fsngt",       // MAT_FSNGT
    "fsnle",       // MAT_FSNLE
    "fsnlt",       // MAT_FSNLT
    "fsoge",       // MAT_FSOGE
    "fsogt",       // MAT_FSOGT
    "fsole",       // MAT_FSOLE
    "fsolg",       // MAT_FSOLG
    "fsolt",       // MAT_FSOLT
    "fsor",        // MAT_FSOR
    "fsseq",       // MAT_FSSEQ
    "fssf",        // MAT_FSSF
    "fssne",       // MAT_FSSNE
    "fsst",        // MAT_FSST
    "fst",         // MAT_FST
    "fsueq",       // MAT_FSUEQ
    "fsuge",       // MAT_FSUGE
    "fsugt",       // MAT_FSUGT
    "fsule",       // MAT_FSULE
    "fsult",       // MAT_FSULT
    "fsun",        // MAT_FSUN
    
    "ftrap",       // MAT_FTRAP
    "ftrapeq",     // MAT_FTRAPEQ
    "ftrapf",      // MAT_FTRAPF
    "ftrapge",     // MAT_FTRAPGE
    "ftrapgl",     // MAT_FTRAPGL
    "ftrapgle",    // MAT_FTRAPGLE
    "ftrapgt",     // MAT_FTRAPGT
    "ftraple",     // MAT_FTRAPLE
    "ftraplt",     // MAT_FTRAPLT
    "ftrapne",     // MAT_FTRAPNE
    "ftrapnge",    // MAT_FTRAPNGE
    "ftrapngl",    // MAT_FTRAPNGL
    "ftrapngle",   // MAT_FTRAPNGLE
    "ftrapngt",    // MAT_FTRAPNGT
    "ftrapnle",    // MAT_FTRAPNLE
    "ftrapnlt",    // MAT_FTRAPNLT
    "ftrapoge",    // MAT_FTRAPOGE
    "ftrapogt",    // MAT_FTRAPOGT
    "ftrapole",    // MAT_FTRAPOLE
    "ftrapolg",    // MAT_FTRAPOLG
    "ftrapolt",    // MAT_FTRAPOLT
    "ftrapor",     // MAT_FTRAPOR
    "ftrapseq",    // MAT_FTRAPSEQ
    "ftrapsf",     // MAT_FTRAPSF
    "ftrapsne",    // MAT_FTRAPSNE
    "ftrapst",     // MAT_FTRAPST
    "ftrapt",      // MAT_FTRAPT
    "ftrapueq",    // MAT_FTRAPUEQ
    "ftrapuge",    // MAT_FTRAPUGE
    "ftrapugt",    // MAT_FTRAPUGT
    "ftrapule",    // MAT_FTRAPULE
    "ftrapult",    // MAT_FTRAPULT
    "ftrapun",     // MAT_FTRAPUN
    
    "pbac",        // MAT_PBAC
    "pbas",        // MAT_PBAS
    "pbbc",        // MAT_PBBC
    "pbbs",        // MAT_PBBS
    "pbcc",        // MAT_PBCC
    "pbcs",        // MAT_PBCS
    "pbgc",        // MAT_PBGC
    "pbgs",        // MAT_PBGS
    "pbic",        // MAT_PBIC
    "pbis",        // MAT_PBIS
    "pblc",        // MAT_PBLC
    "pbls",        // MAT_PBLS
    "pbsc",        // MAT_PBSC
    "pbss",        // MAT_PBSS
    "pbwc",        // MAT_PBWC
    "pbws",        // MAT_PBWS
    
    "pdbac",       // MAT_PDBAC
    "pdbas",       // MAT_PDBAS
    "pdbbc",       // MAT_PDBBC
    "pdbbs",       // MAT_PDBBS
    "pdbcc",       // MAT_PDBCC
    "pdbcs",       // MAT_PDBCS
    "pdbgc",       // MAT_PDBGC
    "pdbgs",       // MAT_PDBGS
    "pdbic",       // MAT_PDBIC
    "pdbis",       // MAT_PDBIS
    "pdblc",       // MAT_PDBLC
    "pdbls",       // MAT_PDBLS
    "pdbsc",       // MAT_PDBSC
    "pdbss",       // MAT_PDBSS
    "pdbwc",       // MAT_PDBWC
    "pdbws",       // MAT_PDBWS
    
    "psac",        // MAT_PSAC
    "psas",        // MAT_PSAS
    "psbc",        // MAT_PSBC
    "psbs",        // MAT_PSBS
    "pscc",        // MAT_PSCC
    "pscs",        // MAT_PSCS
    "psgc",        // MAT_PSGC
    "psgs",        // MAT_PSGS
    "psic",        // MAT_PSIC
    "psis",        // MAT_PSIS
    "pslc",        // MAT_PSLC
    "psls",        // MAT_PSLS
    "pssc",        // MAT_PSSC
    "psss",        // MAT_PSSS
    "pswc",        // MAT_PSWC
    "psws",        // MAT_PSWS
    
    "ptrapac",     // MAT_PTRAPAC
    "ptrapas",     // MAT_PTRAPAS
    "ptrapbc",     // MAT_PTRAPBC
    "ptrapbs",     // MAT_PTRAPBS
    "ptrapcc",     // MAT_PTRAPCC
    "ptrapcs",     // MAT_PTRAPCS
    "ptrapgc",     // MAT_PTRAPGC
    "ptrapgs",     // MAT_PTRAPGS
    "ptrapic",     // MAT_PTRAPIC
    "ptrapis",     // MAT_PTRAPIS
    "ptraplc",     // MAT_PTRAPLC
    "ptrapls",     // MAT_PTRAPLS
    "ptrapsc",     // MAT_PTRAPSC
    "ptrapss",     // MAT_PTRAPSS
    "ptrapwc",     // MAT_PTRAPWC
    "ptrapws",     // MAT_PTRAPWS
    
    "scc",         // MAT_SCC
    "scs",         // MAT_SCS
    "seq",         // MAT_SEQ
    "sf",          // MAT_SF
    "sge",         // MAT_SGE
    "sgt",         // MAT_SGT
    "shi",         // MAT_SHI
    "shs",         // MAT_SHS
    "sle",         // MAT_SLE
    "slo",         // MAT_SLO
    "sls",         // MAT_SLS
    "slt",         // MAT_SLT
    "smi",         // MAT_SMI
    "sne",         // MAT_SNE
    "spl",         // MAT_SPL
    "st",          // MAT_ST
    "svc",         // MAT_SVC
    "svs",         // MAT_SVS
    
    "trapcc",      // MAT_TRAPCC
    "trapcs",      // MAT_TRAPCS
    "trapeq",      // MAT_TRAPEQ
    "trapf",       // MAT_TRAPF
    "trapge",      // MAT_TRAPGE
    "trapgt",      // MAT_TRAPGT
    "traphi",      // MAT_TRAPHI
    "traphs",      // MAT_TRAPHS
    "traple",      // MAT_TRAPLE
    "traplo",      // MAT_TRAPLO
    "trapls",      // MAT_TRAPLS
    "traplt",      // MAT_TRAPLT
    "trapmi",      // MAT_TRAPMI
    "trapne",      // MAT_TRAPNE
    "trappl",      // MAT_TRAPPL
    "trapt",       // MAT_TRAPT
    "trapvc",      // MAT_TRAPVC
    "trapvs",      // MAT_TRAPVS
};

// table with the mnemonics for each master type
PM68KC_STR _M68KTextMnemonics[M68K_IT__SIZEOF__] = 
{
    "?",            // M68K_IT_INVALID
    "abcd",         // M68K_IT_ABCD
    "add",          // M68K_IT_ADD
    "adda",         // M68K_IT_ADDA
    "addi",         // M68K_IT_ADDI
    "addiw",        // M68K_IT_ADDIW
    "addq",         // M68K_IT_ADDQ
    "addx",         // M68K_IT_ADDX
    "and",          // M68K_IT_AND
    "andi",         // M68K_IT_ANDI
    "asl",          // M68K_IT_ASL
    "asr",          // M68K_IT_ASR
    "bank",         // M68K_IT_BANK
    "bcc",          // M68K_IT_BCC
    "bchg",         // M68K_IT_BCHG
    "bclr",         // M68K_IT_BCLR
    "bfchg",        // M68K_IT_BFCHG
    "bfclr",        // M68K_IT_BFCLR
    "bfexts",       // M68K_IT_BFEXTS
    "bfextu",       // M68K_IT_BFEXTU
    "bfffo",        // M68K_IT_BFFFO
    "bfins",        // M68K_IT_BFINS
    "bflyb",        // M68K_IT_BFLYB
    "bflyw",        // M68K_IT_BFLYW
    "bfset",        // M68K_IT_BFSET
    "bftst",        // M68K_IT_BFTST
    "bgnd",         // M68K_IT_BGND
    "bkpt",         // M68K_IT_BKPT
    "bra",          // M68K_IT_BRA
    "bsel",         // M68K_IT_BSEL
    "bset",         // M68K_IT_BSET
    "bsr",          // M68K_IT_BSR
    "btst",         // M68K_IT_BTST
    "c2p",          // M68K_IT_C2P
    "callm",        // M68K_IT_CALLM
    "cas",          // M68K_IT_CAS
    "cas2",         // M68K_IT_CAS2
    "chk",          // M68K_IT_CHK
    "chk2",         // M68K_IT_CHK2
    "cinva",        // M68K_IT_CINVA
    "cinvl",        // M68K_IT_CINVL
    "cinvp",        // M68K_IT_CINVP
    "clr",          // M68K_IT_CLR
    "cmp",          // M68K_IT_CMP
    "cmp2",         // M68K_IT_CMP2
    "cmpa",         // M68K_IT_CMPA
    "cmpi",         // M68K_IT_CMPI
    "cmpiw",        // M68K_IT_CMPIW
    "cmpm",         // M68K_IT_CMPM
    "cpbcc",        // M68K_IT_CPBCC
    "cpdbcc",       // M68K_IT_CPDBCC
    "cpgen",        // M68K_IT_CPGEN
    "cprestore",    // M68K_IT_CPRESTORE
    "cpsave",       // M68K_IT_CPSAVE
    "cpscc",        // M68K_IT_CPSCC
    "cptrapcc",     // M68K_IT_CPTRAPCC
    "cpusha",       // M68K_IT_CPUSHA
    "cpushl",       // M68K_IT_CPUSHL
    "cpushp",       // M68K_IT_CPUSHP
    "dbcc",         // M68K_IT_DBCC
    "divs",         // M68K_IT_DIVS
    "divsl",        // M68K_IT_DIVSL
    "divu",         // M68K_IT_DIVU
    "divul",        // M68K_IT_DIVUL
    "eor",          // M68K_IT_EOR
    "eori",         // M68K_IT_EORI
    "exg",          // M68K_IT_EXG
    "ext",          // M68K_IT_EXT
    "extb",         // M68K_IT_EXTB
    "fabs",         // M68K_IT_FABS
    "facos",        // M68K_IT_FACOS
    "fadd",         // M68K_IT_FADD
    "fasin",        // M68K_IT_FASIN
    "fatan",        // M68K_IT_FATAN
    "fatanh",       // M68K_IT_FATANH
    "fbcc",         // M68K_IT_FBCC
    "fcmp",         // M68K_IT_FCMP
    "fcos",         // M68K_IT_FCOS
    "fcosh",        // M68K_IT_FCOSH
    "fdabs",        // M68K_IT_FDABS
    "fdadd",        // M68K_IT_FDADD
    "fdbcc",        // M68K_IT_FDBCC
    "fddiv",        // M68K_IT_FDDIV
    "fdiv",         // M68K_IT_FDIV
    "fdmove",       // M68K_IT_FDMOVE
    "fdmul",        // M68K_IT_FDMUL
    "fdneg",        // M68K_IT_FDNEG
    "fdsqrt",       // M68K_IT_FDSQRT
    "fdsub",        // M68K_IT_FDSUB
    "fetox",        // M68K_IT_FETOX
    "fetoxm1",      // M68K_IT_FETOXM1
    "fgetexp",      // M68K_IT_FGETEXP
    "fgetman",      // M68K_IT_FGETMAN
    "fint",         // M68K_IT_FINT
    "fintrz",       // M68K_IT_FINTRZ
    "flog10",       // M68K_IT_FLOG10
    "flog2",        // M68K_IT_FLOG2
    "flogn",        // M68K_IT_FLOGN
    "flognp1",      // M68K_IT_FLOGNP1
    "fmod",         // M68K_IT_FMOD
    "fmove",        // M68K_IT_FMOVE
    "fmovecr",      // M68K_IT_FMOVECR
    "fmovem",       // M68K_IT_FMOVEM
    "fmul",         // M68K_IT_FMUL
    "fneg",         // M68K_IT_FNEG
    "fnop",         // M68K_IT_FNOP
    "frem",         // M68K_IT_FREM
    "frestore",     // M68K_IT_FRESTORE
    "fsabs",        // M68K_IT_FSABS
    "fsadd",        // M68K_IT_FSADD
    "fsave",        // M68K_IT_FSAVE
    "fscale",       // M68K_IT_FSCALE
    "fscc",         // M68K_IT_FSCC
    "fsdiv",        // M68K_IT_FSDIV
    "fsgldiv",      // M68K_IT_FSGLDIV
    "fsglmul",      // M68K_IT_FSGLMUL
    "fsin",         // M68K_IT_FSIN
    "fsincos",      // M68K_IT_FSINCOS
    "fsinh",        // M68K_IT_FSINH
    "fsmove",       // M68K_IT_FSMOVE
    "fsmul",        // M68K_IT_FSMUL
    "fsneg",        // M68K_IT_FSNEG
    "fsqrt",        // M68K_IT_FSQRT
    "fssqrt",       // M68K_IT_FSSQRT
    "fssub",        // M68K_IT_FSSUB
    "fsub",         // M68K_IT_FSUB
    "ftan",         // M68K_IT_FTAN
    "ftanh",        // M68K_IT_FTANH
    "ftentox",      // M68K_IT_FTENTOX
    "ftrapcc",      // M68K_IT_FTRAPCC
    "ftst",         // M68K_IT_FTST
    "ftwotox",      // M68K_IT_FTWOTOX
    "illegal",      // M68K_IT_ILLEGAL
    "jmp",          // M68K_IT_JMP
    "jsr",          // M68K_IT_JSR
    "lea",          // M68K_IT_LEA
    "link",         // M68K_IT_LINK
    "load",         // M68K_IT_LOAD
    "loadi",        // M68K_IT_LOADI
    "lpstop",       // M68K_IT_LPSTOP
    "lsl",          // M68K_IT_LSL
    "lslq",         // M68K_IT_LSLQ
    "lsr",          // M68K_IT_LSR
    "lsrq",         // M68K_IT_LSRQ
    "minterm",      // M68K_IT_MINTERM
    "move",         // M68K_IT_MOVE
    "move16",       // M68K_IT_MOVE16
    "movea",        // M68K_IT_MOVEA
    "movec",        // M68K_IT_MOVEC
    "movem",        // M68K_IT_MOVEM
    "movep",        // M68K_IT_MOVEP
    "moveq",        // M68K_IT_MOVEQ
    "moves",        // M68K_IT_MOVES
    "movex",        // M68K_IT_MOVEX
    "muls",         // M68K_IT_MULS
    "mulu",         // M68K_IT_MULU
    "nbcd",         // M68K_IT_NBCD
    "neg",          // M68K_IT_NEG
    "negx",         // M68K_IT_NEGX
    "nop",          // M68K_IT_NOP
    "not",          // M68K_IT_NOT
    "or",           // M68K_IT_OR
    "ori",          // M68K_IT_ORI
    "pabsb",        // M68K_IT_PABSB
    "pabsw",        // M68K_IT_PABSW
    "pack",         // M68K_IT_PACK
    "pack3216",     // M68K_IT_PACK3216
    "packuswb",     // M68K_IT_PACKUSWB
    "paddb",        // M68K_IT_PADDB
    "paddusb",      // M68K_IT_PADDUSB
    "paddusw",      // M68K_IT_PADDUSW
    "paddw",        // M68K_IT_PADDW
    "pand",         // M68K_IT_PAND
    "pandn",        // M68K_IT_PANDN
    "pavg",         // M68K_IT_PAVG
    "pbcc",         // M68K_IT_PBCC
    "pcmpeqb",      // M68K_IT_PCMPEQB
    "pcmpeqw",      // M68K_IT_PCMPEQW
    "pcmpgeb",      // M68K_IT_PCMPGTB
    "pcmpgew",      // M68K_IT_PCMPGTW
    "pcmpgtb",      // M68K_IT_PCMPGTB
    "pcmpgtw",      // M68K_IT_PCMPGTW
    "pcmphib",      // M68K_IT_PCMPHIB
    "pcmphiw",      // M68K_IT_PCMPHIW
    "pdbcc",        // M68K_IT_PDBCC
    "pea",          // M68K_IT_PEA
    "peor",         // M68K_IT_PEOR
    "perm",         // M68K_IT_PERM
    "pflush",       // M68K_IT_PFLUSH
    "pflusha",      // M68K_IT_PFLUSHA
    "pflushan",     // M68K_IT_PFLUSHAN
    "pflushn",      // M68K_IT_PFLUSHN
    "pflushr",      // M68K_IT_PFLUSHR
    "pflushs",      // M68K_IT_PFLUSHS
    "ploadr",       // M68K_IT_PLOADR
    "ploadw",       // M68K_IT_PLOADW
    "plpar",        // M68K_IT_PLPAR
    "plpaw",        // M68K_IT_PLPAW
    "pmaxsb",       // M68K_IT_PMAXSB
    "pmaxsw",       // M68K_IT_PMAXSW
    "pmaxub",       // M68K_IT_PMAXUB
    "pmaxuw",       // M68K_IT_PMAXUW
    "pminsb",       // M68K_IT_PMINSB
    "pminsw",       // M68K_IT_PMINSW
    "pminub",       // M68K_IT_PMINUB
    "pminuw",       // M68K_IT_PMINUW
    "pmove",        // M68K_IT_PMOVE
    "pmovefd",      // M68K_IT_PMOVEFD
    "pmul88",       // M68K_IT_PMUL88
    "pmula",        // M68K_IT_PMULA
    "pmulh",        // M68K_IT_PMULH
    "pmull",        // M68K_IT_PMULL
    "por",          // M68K_IT_POR
    "prestore",     // M68K_IT_PRESTORE
    "psave",        // M68K_IT_PSAVE
    "pscc",         // M68K_IT_PSCC
    "psubb",        // M68K_IT_PSUBB
    "psubusb",      // M68K_IT_PSUBUSB
    "psubusw",      // M68K_IT_PSUBUSW
    "psubw",        // M68K_IT_PSUBW
    "ptestr",       // M68K_IT_PTESTR
    "ptestw",       // M68K_IT_PTESTW
    "ptrapcc",      // M68K_IT_PTRAPCC
    "pvalid",       // M68K_IT_PVALID
    "reset",        // M68K_IT_RESET
    "rol",          // M68K_IT_ROL
    "ror",          // M68K_IT_ROR
    "roxl",         // M68K_IT_ROXL
    "roxr",         // M68K_IT_ROXR
    "rpc",          // M68K_IT_RPC
    "rtd",          // M68K_IT_RTD
    "rte",          // M68K_IT_RTE
    "rtm",          // M68K_IT_RTM
    "rtr",          // M68K_IT_RTR
    "rts",          // M68K_IT_RTS
    "scc",          // M68K_IT_SCC
    "stop",         // M68K_IT_STOP
    "store",        // M68K_IT_STORE
    "storec",       // M68K_IT_STOREC
    "storei",       // M68K_IT_STOREI
    "storeilm",     // M68K_IT_STOREILM
    "storem",       // M68K_IT_STOREM
    "storem3",      // M68K_IT_STOREM3
    "sub",          // M68K_IT_SUB
    "suba",         // M68K_IT_SUBA
    "subi",         // M68K_IT_SUBI
    "subq",         // M68K_IT_SUBQ
    "subx",         // M68K_IT_SUBX
    "swap",         // M68K_IT_SWAP
    "tas",          // M68K_IT_TAS
    "tbls",         // M68K_IT_TBLS
    "tblsn",        // M68K_IT_TBLSN
    "tblu",         // M68K_IT_TBLU
    "tblun",        // M68K_IT_TBLUN
    "touch",        // M68K_IT_TOUCH
    "transhi",      // M68K_IT_TRANSHI
    "transilo",     // M68K_IT_TRANSILO
    "translo",      // M68K_IT_TRANSLO
    "trap",         // M68K_IT_TRAP
    "trapcc",       // M68K_IT_TRAPCC
    "trapv",        // M68K_IT_TRAPV
    "tst",          // M68K_IT_TST
    "unlk",         // M68K_IT_UNLK
    "unpack1632",   // M68K_IT_UNPACK1632
    "unpk",         // M68K_IT_UNPK
    "vperm",        // M68K_IT_VPERM
};

// table of texts for each register (unsorted)
PM68KC_STR _M68KTextRegisters[M68K_RT__SIZEOF__] =
{
    M68K_NULL,  // M68K_RT_NONE
    "a0",       // M68K_RT_A0
    "a1",       // M68K_RT_A1
    "a2",       // M68K_RT_A2
    "a3",       // M68K_RT_A3
    "a4",       // M68K_RT_A4
    "a5",       // M68K_RT_A5
    "a6",       // M68K_RT_A6
    "a7",       // M68K_RT_A7
    "ac",       // M68K_RT_AC
    "ac0",      // M68K_RT_AC0
    "ac1",      // M68K_RT_AC1
    "acusr",    // M68K_RT_ACUSR
    "b0",       // M68K_RT_B0
    "b1",       // M68K_RT_B1
    "b2",       // M68K_RT_B2
    "b3",       // M68K_RT_B3
    "b4",       // M68K_RT_B4
    "b5",       // M68K_RT_B5
    "b6",       // M68K_RT_B6
    "b7",       // M68K_RT_B7
    "bac0",     // M68K_RT_BAC0
    "bac1",     // M68K_RT_BAC1
    "bac2",     // M68K_RT_BAC2
    "bac3",     // M68K_RT_BAC3
    "bac4",     // M68K_RT_BAC4
    "bac5",     // M68K_RT_BAC5
    "bac6",     // M68K_RT_BAC6
    "bac7",     // M68K_RT_BAC7
    "bad0",     // M68K_RT_BAD0
    "bad1",     // M68K_RT_BAD1
    "bad2",     // M68K_RT_BAD2
    "bad3",     // M68K_RT_BAD3
    "bad4",     // M68K_RT_BAD4
    "bad5",     // M68K_RT_BAD5
    "bad6",     // M68K_RT_BAD6
    "bad7",     // M68K_RT_BAD7
    "bpc",      // M68K_RT_BPC
    "bpw",      // M68K_RT_BPW
    "buscr",    // M68K_RT_BUSCR
    "caar",     // M68K_RT_CAAR
    "cacr",     // M68K_RT_CACR
    "cal",      // M68K_RT_CAL
    "ccc",      // M68K_RT_CCC
    "ccr",      // M68K_RT_CCR
    "cmw",      // M68K_RT_CMW
    "crp",      // M68K_RT_CRP
    "d0",       // M68K_RT_D0
    "d1",       // M68K_RT_D1
    "d2",       // M68K_RT_D2
    "d3",       // M68K_RT_D3
    "d4",       // M68K_RT_D4
    "d5",       // M68K_RT_D5
    "d6",       // M68K_RT_D6
    "d7",       // M68K_RT_D7
    "dacr0",    // M68K_RT_DACR0
    "dacr1",    // M68K_RT_DACR1
    "dch",      // M68K_RT_DCH
    "dcm",      // M68K_RT_DCM
    "dfc",      // M68K_RT_DFC
    "drp",      // M68K_RT_DRP
    "dtt0",     // M68K_RT_DTT0
    "dtt1",     // M68K_RT_DTT1
    "e0",       // M68K_RT_E0
    "e1",       // M68K_RT_E1
    "e2",       // M68K_RT_E2
    "e3",       // M68K_RT_E3
    "e4",       // M68K_RT_E4
    "e5",       // M68K_RT_E5
    "e6",       // M68K_RT_E6
    "e7",       // M68K_RT_E7
    "e8",       // M68K_RT_E8
    "e9",       // M68K_RT_E9
    "e10",      // M68K_RT_E10
    "e11",      // M68K_RT_E11
    "e12",      // M68K_RT_E12
    "e13",      // M68K_RT_E13
    "e14",      // M68K_RT_E14
    "e15",      // M68K_RT_E15
    "e16",      // M68K_RT_E16
    "e17",      // M68K_RT_E17
    "e18",      // M68K_RT_E18
    "e19",      // M68K_RT_E19
    "e20",      // M68K_RT_E20
    "e21",      // M68K_RT_E21
    "e22",      // M68K_RT_E22
    "e23",      // M68K_RT_E23
    "fp0",      // M68K_RT_FP0
    "fp1",      // M68K_RT_FP1
    "fp2",      // M68K_RT_FP2
    "fp3",      // M68K_RT_FP3
    "fp4",      // M68K_RT_FP4
    "fp5",      // M68K_RT_FP5
    "fp6",      // M68K_RT_FP6
    "fp7",      // M68K_RT_FP7
    "fpcr",     // M68K_RT_FPCR
    "fpiar",    // M68K_RT_FPIAR
    "fpsr",     // M68K_RT_FPSR
    "iacr0",    // M68K_RT_IACR0
    "iacr1",    // M68K_RT_IACR1
    "iep1",     // M68K_RT_IEP1
    "iep2",     // M68K_RT_IEP2
    "isp",      // M68K_RT_ISP
    "itt0",     // M68K_RT_ITT0
    "itt1",     // M68K_RT_ITT1
    "mmusr",    // M68K_RT_MMUSR
    "msp",      // M68K_RT_MSP
    "pc",       // M68K_RT_PC
    "pcr",      // M68K_RT_PCR
    "pcsr",     // M68K_RT_PCSR
    "psr",      // M68K_RT_PSR
    "scc",      // M68K_RT_SCC
    "sfc",      // M68K_RT_SFC
    "sr",       // M68K_RT_SR
    "srp",      // M68K_RT_SRP
    "stb",      // M68K_RT_STB
    "stc",      // M68K_RT_STC
    "sth",      // M68K_RT_STH
    "str",      // M68K_RT_STR
    "tc",       // M68K_RT_TC
    "tt0",      // M68K_RT_TT0
    "tt1",      // M68K_RT_TT1
    "urp",      // M68K_RT_URP
    "usp",      // M68K_RT_USP
    "val",      // M68K_RT_VAL
    "vbr",      // M68K_RT_VBR
    "zpc",      // M68K_RT_ZPC
};

// char for each scale value
M68K_CHAR _M68KTextScaleChars[M68K_SCALE__SIZEOF__] = 
{
    '1',    // M68K_SCALE_1
    '2',    // M68K_SCALE_2 
    '4',    // M68K_SCALE_4
    '8',    // M68K_SCALE_8
};

// table of (sorted) chars for the sizes
M68K_CHAR _M68KTextSizeChars[M68K_SIZE__SIZEOF__] =
{
    0,      // M68K_SIZE_NONE
    'b',    // M68K_SIZE_B
    'd',    // M68K_SIZE_D
    'l',    // M68K_SIZE_L
    'p',    // M68K_SIZE_P
    'q',    // M68K_SIZE_Q
    's',    // M68K_SIZE_S
    'w',    // M68K_SIZE_W
    'x',    // M68K_SIZE_X
};

// table of registers sorted by text;
// this table saves the type of register and the text can be retrieved from _M68KTextRegisters
M68KC_REGISTER_TYPE_VALUE _M68KTextSortedRegisters[M68K_RT__SIZEOF__] =
{
    M68K_RT_NONE,
    M68K_RT_A0,
    M68K_RT_A1,
    M68K_RT_A2,
    M68K_RT_A3,
    M68K_RT_A4,
    M68K_RT_A5,
    M68K_RT_A6,
    M68K_RT_A7,
    M68K_RT_AC,
    M68K_RT_AC0,
    M68K_RT_AC1,
    M68K_RT_ACUSR,
    M68K_RT_B0,
    M68K_RT_B1,
    M68K_RT_B2,
    M68K_RT_B3,
    M68K_RT_B4,
    M68K_RT_B5,
    M68K_RT_B6,
    M68K_RT_B7,
    M68K_RT_BAC0,
    M68K_RT_BAC1,
    M68K_RT_BAC2,
    M68K_RT_BAC3,
    M68K_RT_BAC4,
    M68K_RT_BAC5,
    M68K_RT_BAC6,
    M68K_RT_BAC7,
    M68K_RT_BAD0,
    M68K_RT_BAD1,
    M68K_RT_BAD2,
    M68K_RT_BAD3,
    M68K_RT_BAD4,
    M68K_RT_BAD5,
    M68K_RT_BAD6,
    M68K_RT_BAD7,
    M68K_RT_BPC,
    M68K_RT_BPW,
    M68K_RT_BUSCR,
    M68K_RT_CAAR,
    M68K_RT_CACR,
    M68K_RT_CAL,
    M68K_RT_CCC,
    M68K_RT_CCR,
    M68K_RT_CMW,
    M68K_RT_CRP,
    M68K_RT_D0,
    M68K_RT_D1,
    M68K_RT_D2,
    M68K_RT_D3,
    M68K_RT_D4,
    M68K_RT_D5,
    M68K_RT_D6,
    M68K_RT_D7,
    M68K_RT_DACR0,
    M68K_RT_DACR1,
    M68K_RT_DCH,
    M68K_RT_DCM,
    M68K_RT_DFC,
    M68K_RT_DRP,
    M68K_RT_DTT0,
    M68K_RT_DTT1,
    M68K_RT_E0,
    M68K_RT_E1,
    M68K_RT_E10,
    M68K_RT_E11,
    M68K_RT_E12,
    M68K_RT_E13,
    M68K_RT_E14,
    M68K_RT_E15,
    M68K_RT_E16,
    M68K_RT_E17,
    M68K_RT_E18,
    M68K_RT_E19,
    M68K_RT_E2,
    M68K_RT_E20,
    M68K_RT_E21,
    M68K_RT_E22,
    M68K_RT_E23,
    M68K_RT_E3,
    M68K_RT_E4,
    M68K_RT_E5,
    M68K_RT_E6,
    M68K_RT_E7,
    M68K_RT_E8,
    M68K_RT_E9,
    M68K_RT_FP0,
    M68K_RT_FP1,
    M68K_RT_FP2,
    M68K_RT_FP3,
    M68K_RT_FP4,
    M68K_RT_FP5,
    M68K_RT_FP6,
    M68K_RT_FP7,
    M68K_RT_FPCR,
    M68K_RT_FPIAR,
    M68K_RT_FPSR,
    M68K_RT_IACR0,
    M68K_RT_IACR1,
    M68K_RT_IEP1,
    M68K_RT_IEP2,
    M68K_RT_ISP,
    M68K_RT_ITT0,
    M68K_RT_ITT1,
    M68K_RT_MMUSR,
    M68K_RT_MSP,
    M68K_RT_PC,
    M68K_RT_PCR,
    M68K_RT_PCSR,
    M68K_RT_PSR,
    M68K_RT_SCC,
    M68K_RT_SFC,
    M68K_RT_SR,
    M68K_RT_SRP,
    M68K_RT_STB,
    M68K_RT_STC,
    M68K_RT_STH,
    M68K_RT_STR,
    M68K_RT_TC,
    M68K_RT_TT0,
    M68K_RT_TT1,
    M68K_RT_URP,
    M68K_RT_USP,
    M68K_RT_VAL,
    M68K_RT_VBR,
    M68K_RT_ZPC,
};

