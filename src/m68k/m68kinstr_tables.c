#include "m68kinternal.h"

#define _A20        M68K_ARCH_68020
#define _AAC80      M68K_ARCH_AC68080
#define _AC32       M68K_ARCH_CPU32
#define _AFPU       M68K_ARCH__FPU__
#define _AFPU4060   M68K_ARCH__FPU4060__
#define _AMMU       M68K_ARCH__MMU__
#define _AMMU30     M68K_ARCH__MMU30__
#define _AMMU3051   M68K_ARCH__MMU3051__
#define _AMMU30EC   M68K_ARCH__MMU30EC__
#define _AMMU40     M68K_ARCH__MMU40__
#define _AMMU4040EC M68K_ARCH__MMU4040EC__
#define _AMMU4060   M68K_ARCH__MMU4060__
#define _AMMU51     M68K_ARCH__MMU51__
#define _AP08H      M68K_ARCH__P08H__
#define _AP10H      M68K_ARCH__P10H__
#define _AP2030     M68K_ARCH__P2030__
#define _AP20304060 M68K_ARCH__P20304060__
#define _AP20H      M68K_ARCH__P20H__
#define _AP40H      M68K_ARCH__P40H__
#define _AP4060     M68K_ARCH__P4060__
#define _AP60LC     M68K_ARCH__P60LC__
#define _APFAM      M68K_ARCH__PFAM__
#define _C(t)       (CCF_##t)
#define _COND(t)    (_C(t) | CCF__CONDITIONAL__)

#define _IFBEWA     (IF_BANK_EW_A)
#define _IFBEWAB    (IF_BANK_EW_A | IF_BANK_EW_B)
#define _IFBEWABC   (IF_BANK_EW_A | IF_BANK_EW_B | IF_BANK_EW_C)
#define _IFCPX      IF_CPX
#define _IFEW1      IF_EXTENSION_WORD_1
#define _IFEW2      IF_EXTENSION_WORD_2
#define _IFFPOP     IF_FPU_OPMODE
#define _IFFPMKF    IF_FMOVE_KFACTOR
#define _IFLREGS    IF_LIST_REGISTERS

#define _IMFCC      IMF_CONDITION_CODE
#define _IMFOP      IMF_FPU_OPMODE
#define _IMFIS      IMF_IMPLICIT_SIZE

// master instruction type for each fpu opmode
C_INSTRUCTION_FPU_OPMODE_INFO _M68KInstrFPUOpmodes[128] =
{
    // 00
    {M68K_IT_FMOVE          , _IFBEWAB  , _AFPU     },
    {M68K_IT_FINT           , _IFBEWAB  , _AFPU     },
    {M68K_IT_FSINH          , 0         , _AFPU     },
    {M68K_IT_FINTRZ         , _IFBEWAB  , _AFPU     },
    {M68K_IT_FSQRT          , _IFBEWAB  , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FLOGNP1        , 0         , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    
    // 08
    {M68K_IT_FETOXM1        , 0         , _AFPU     },
    {M68K_IT_FTANH          , 0         , _AFPU     },
    {M68K_IT_FATAN          , 0         , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FASIN          , 0         , _AFPU     },
    {M68K_IT_FATANH         , 0         , _AFPU     },
    {M68K_IT_FSIN           , 0         , _AFPU     },
    {M68K_IT_FTAN           , 0         , _AFPU     },

    // 10
    {M68K_IT_FETOX          , 0         , _AFPU     },
    {M68K_IT_FTWOTOX        , 0         , _AFPU     },
    {M68K_IT_FTENTOX        , 0         , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FLOGN          , 0         , _AFPU     },
    {M68K_IT_FLOG10         , 0         , _AFPU     },
    {M68K_IT_FLOG2          , 0         , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },

    // 18
    {M68K_IT_FABS           , _IFBEWAB  , _AFPU     },
    {M68K_IT_FCOSH          , 0         , _AFPU     },
    {M68K_IT_FNEG           , _IFBEWAB  , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FACOS          , 0         , _AFPU     },
    {M68K_IT_FCOS           , 0         , _AFPU     },
    {M68K_IT_FGETEXP        , 0         , _AFPU     },
    {M68K_IT_FGETMAN        , 0         , _AFPU     },

    // 20
    {M68K_IT_FDIV           , _IFBEWABC , _AFPU     },
    {M68K_IT_FMOD           , 0         , _AFPU     },
    {M68K_IT_FADD           , _IFBEWABC , _AFPU     },
    {M68K_IT_FMUL           , _IFBEWABC , _AFPU     },
    {M68K_IT_FSGLDIV        , _IFBEWABC , _AFPU     },
    {M68K_IT_FREM           , 0         , _AFPU     },
    {M68K_IT_FSCALE         , 0         , _AFPU     },
    {M68K_IT_FSGLMUL        , _IFBEWABC , _AFPU     },
    
    // 28
    {M68K_IT_FSUB           , _IFBEWABC , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 30
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    
    // 38
    {M68K_IT_FCMP           , _IFBEWAB  , _AFPU     },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         }, // M68K_IT_FTST but can't be used because it only uses one operand
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 40
    {M68K_IT_FSMOVE         , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_FSSQRT         , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FDMOVE         , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_FDSQRT         , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 48
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 50
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 58
    {M68K_IT_FSABS          , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FSNEG          , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FDABS          , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FDNEG          , _IFBEWAB  , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },

    // 60
    {M68K_IT_FSDIV          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FSADD          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_FSMUL          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_FDDIV          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FDADD          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_FDMUL          , _IFBEWABC , _AFPU4060 },
    
    // 68
    {M68K_IT_FSSUB          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_FDSUB          , _IFBEWABC , _AFPU4060 },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },

    // 70
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    
    // 78
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
    {M68K_IT_INVALID        , 0         , 0         },
};

// flags for each master type
C_INSTRUCTION_MASTER_FLAGS_VALUE _M68KInstrMasterTypeFlags[M68K_IT__SIZEOF__] =
{
    0,                  // M68K_IT_INVALID
    _IMFIS,             // M68K_IT_ABCD
    0,                  // M68K_IT_ADD
    0,                  // M68K_IT_ADDA
    0,                  // M68K_IT_ADDI
    _IMFIS,             // M68K_IT_ADDIW
    0,                  // M68K_IT_ADDQ
    0,                  // M68K_IT_ADDX
    0,                  // M68K_IT_AND
    0,                  // M68K_IT_ANDI
    0,                  // M68K_IT_ASL
    0,                  // M68K_IT_ASR
    0,                  // M68K_IT_BANK
    _IMFCC,             // M68K_IT_BCC
    0,                  // M68K_IT_BCHG
    0,                  // M68K_IT_BCLR
    0,                  // M68K_IT_BFCHG
    0,                  // M68K_IT_BFCLR
    0,                  // M68K_IT_BFEXTS
    0,                  // M68K_IT_BFEXTU
    0,                  // M68K_IT_BFFFO
    0,                  // M68K_IT_BFINS
    _IMFIS,             // M68K_IT_BFLYB
    _IMFIS,             // M68K_IT_BFLYW
    0,                  // M68K_IT_BFSET
    0,                  // M68K_IT_BFTST
    0,                  // M68K_IT_BGND
    0,                  // M68K_IT_BKPT
    0,                  // M68K_IT_BRA
    _IMFIS,             // M68K_IT_BSEL
    0,                  // M68K_IT_BSET
    0,                  // M68K_IT_BSR
    0,                  // M68K_IT_BTST
    _IMFIS,             // M68K_IT_C2P
    0,                  // M68K_IT_CALLM
    0,                  // M68K_IT_CAS
    0,                  // M68K_IT_CAS2
    0,                  // M68K_IT_CHK
    0,                  // M68K_IT_CHK2
    0,                  // M68K_IT_CINVA
    0,                  // M68K_IT_CINVL
    0,                  // M68K_IT_CINVP
    0,                  // M68K_IT_CLR
    0,                  // M68K_IT_CMP
    0,                  // M68K_IT_CMP2
    0,                  // M68K_IT_CMPA
    0,                  // M68K_IT_CMPI
    _IMFIS,             // M68K_IT_CMPIW
    0,                  // M68K_IT_CMPM
    0,                  // M68K_IT_CPBCC
    0,                  // M68K_IT_CPDBCC
    0,                  // M68K_IT_CPGEN
    0,                  // M68K_IT_CPRESTORE
    0,                  // M68K_IT_CPSAVE
    0,                  // M68K_IT_CPSCC
    0,                  // M68K_IT_CPTRAPCC
    0,                  // M68K_IT_CPUSHA
    0,                  // M68K_IT_CPUSHL
    0,                  // M68K_IT_CPUSHP
    _IMFCC | _IMFIS,    // M68K_IT_DBCC
    0,                  // M68K_IT_DIVS
    0,                  // M68K_IT_DIVSL
    0,                  // M68K_IT_DIVU
    0,                  // M68K_IT_DIVUL
    0,                  // M68K_IT_EOR
    0,                  // M68K_IT_EORI
    0,                  // M68K_IT_EXG
    0,                  // M68K_IT_EXT
    0,                  // M68K_IT_EXTB
    _IMFOP,             // M68K_IT_FABS
    _IMFOP,             // M68K_IT_FACOS
    _IMFOP,             // M68K_IT_FADD
    _IMFOP,             // M68K_IT_FASIN
    _IMFOP,             // M68K_IT_FATAN
    _IMFOP,             // M68K_IT_FATANH
    _IMFCC,             // M68K_IT_FBCC
    _IMFOP,             // M68K_IT_FCMP
    _IMFOP,             // M68K_IT_FCOS
    _IMFOP,             // M68K_IT_FCOSH
    _IMFOP,             // M68K_IT_FDABS
    _IMFOP,             // M68K_IT_FDADD
    _IMFCC,             // M68K_IT_FDBCC
    _IMFOP,             // M68K_IT_FDDIV
    _IMFOP,             // M68K_IT_FDIV
    _IMFOP,             // M68K_IT_FDMOVE
    _IMFOP,             // M68K_IT_FDMUL
    _IMFOP,             // M68K_IT_FDNEG
    _IMFOP,             // M68K_IT_FDSQRT
    _IMFOP,             // M68K_IT_FDSUB
    _IMFOP,             // M68K_IT_FETOX
    _IMFOP,             // M68K_IT_FETOXM1
    _IMFOP,             // M68K_IT_FGETEXP
    _IMFOP,             // M68K_IT_FGETMAN
    _IMFOP,             // M68K_IT_FINT
    _IMFOP,             // M68K_IT_FINTRZ
    _IMFOP,             // M68K_IT_FLOG10
    _IMFOP,             // M68K_IT_FLOG2
    _IMFOP,             // M68K_IT_FLOGN
    _IMFOP,             // M68K_IT_FLOGNP1
    _IMFOP,             // M68K_IT_FMOD
    _IMFOP,             // M68K_IT_FMOVE
    _IMFIS,             // M68K_IT_FMOVECR
    0,                  // M68K_IT_FMOVEM
    _IMFOP,             // M68K_IT_FMUL
    _IMFOP,             // M68K_IT_FNEG
    0,                  // M68K_IT_FNOP
    _IMFOP,             // M68K_IT_FREM
    0,                  // M68K_IT_FRESTORE
    _IMFOP,             // M68K_IT_FSABS
    _IMFOP,             // M68K_IT_FSADD
    0,                  // M68K_IT_FSAVE
    _IMFOP,             // M68K_IT_FSCALE
    _IMFCC | _IMFIS,    // M68K_IT_FSCC
    _IMFOP,             // M68K_IT_FSDIV
    _IMFOP,             // M68K_IT_FSGLDIV
    _IMFOP,             // M68K_IT_FSGLMUL
    _IMFOP,             // M68K_IT_FSIN
    0,                  // M68K_IT_FSINCOS
    _IMFOP,             // M68K_IT_FSINH
    _IMFOP,             // M68K_IT_FSMOVE
    _IMFOP,             // M68K_IT_FSMUL
    _IMFOP,             // M68K_IT_FSNEG
    _IMFOP,             // M68K_IT_FSQRT
    _IMFOP,             // M68K_IT_FSSQRT
    _IMFOP,             // M68K_IT_FSSUB
    _IMFOP,             // M68K_IT_FSUB
    _IMFOP,             // M68K_IT_FTAN
    _IMFOP,             // M68K_IT_FTANH
    _IMFOP,             // M68K_IT_FTENTOX
    _IMFCC,             // M68K_IT_FTRAPCC
    0,                  // M68K_IT_FTST
    _IMFOP,             // M68K_IT_FTWOTOX
    0,                  // M68K_IT_ILLEGAL
    0,                  // M68K_IT_JMP
    0,                  // M68K_IT_JSR
    _IMFIS,             // M68K_IT_LEA
    0,                  // M68K_IT_LINK
    _IMFIS,             // M68K_IT_LOAD
    _IMFIS,             // M68K_IT_LOADI
    _IMFIS,             // M68K_IT_LPSTOP
    0,                  // M68K_IT_LSL
    _IMFIS,             // M68K_IT_LSLQ
    0,                  // M68K_IT_LSR
    _IMFIS,             // M68K_IT_LSRQ
    _IMFIS,             // M68K_IT_MINTERM
    0,                  // M68K_IT_MOVE
    0,                  // M68K_IT_MOVE16
    0,                  // M68K_IT_MOVEA
    0,                  // M68K_IT_MOVEC
    0,                  // M68K_IT_MOVEM
    0,                  // M68K_IT_MOVEP
    _IMFIS,             // M68K_IT_MOVEQ
    0,                  // M68K_IT_MOVES
    0,                  // M68K_IT_MOVEX
    0,                  // M68K_IT_MULS
    0,                  // M68K_IT_MULU
    _IMFIS,             // M68K_IT_NBCD
    0,                  // M68K_IT_NEG
    0,                  // M68K_IT_NEGX
    0,                  // M68K_IT_NOP
    0,                  // M68K_IT_NOT
    0,                  // M68K_IT_OR
    0,                  // M68K_IT_ORI
    _IMFIS,             // M68K_IT_PABSB
    _IMFIS,             // M68K_IT_PABSW
    0,                  // M68K_IT_PACK
    _IMFIS,             // M68K_IT_PACK3216
    _IMFIS,             // M68K_IT_PACKUSWB
    _IMFIS,             // M68K_IT_PADDB
    _IMFIS,             // M68K_IT_PADDUSB
    _IMFIS,             // M68K_IT_PADDUSW
    _IMFIS,             // M68K_IT_PADDW
    _IMFIS,             // M68K_IT_PAND
    _IMFIS,             // M68K_IT_PANDN
    _IMFIS,             // M68K_IT_PAVG
    _IMFCC,             // M68K_IT_PBCC
    _IMFIS,             // M68K_IT_PCMPEQB
    _IMFIS,             // M68K_IT_PCMPEQW
    _IMFIS,             // M68K_IT_PCMPGEB
    _IMFIS,             // M68K_IT_PCMPGEW
    _IMFIS,             // M68K_IT_PCMPGTB
    _IMFIS,             // M68K_IT_PCMPGTW
    _IMFIS,             // M68K_IT_PCMPHIB
    _IMFIS,             // M68K_IT_PCMPHIW
    _IMFCC,             // M68K_IT_PDBCC
    _IMFIS,             // M68K_IT_PEA
    _IMFIS,             // M68K_IT_PEOR
    _IMFIS,             // M68K_IT_PERM
    0,                  // M68K_IT_PFLUSH
    0,                  // M68K_IT_PFLUSHA
    0,                  // M68K_IT_PFLUSHAN
    0,                  // M68K_IT_PFLUSHN
    _IMFIS,             // M68K_IT_PFLUSHR
    0,                  // M68K_IT_PFLUSHS
    0,                  // M68K_IT_PLOADR
    0,                  // M68K_IT_PLOADW
    0,                  // M68K_IT_PLPAR
    0,                  // M68K_IT_PLPAW
    _IMFIS,             // M68K_IT_PMAXSB
    _IMFIS,             // M68K_IT_PMAXSW
    _IMFIS,             // M68K_IT_PMAXUB
    _IMFIS,             // M68K_IT_PMAXUW
    _IMFIS,             // M68K_IT_PMINSB
    _IMFIS,             // M68K_IT_PMINSW
    _IMFIS,             // M68K_IT_PMINUB
    _IMFIS,             // M68K_IT_PMINUW
    0,                  // M68K_IT_PMOVE
    0,                  // M68K_IT_PMOVEFD
    _IMFIS,             // M68K_IT_PMUL88
    _IMFIS,             // M68K_IT_PMULA
    _IMFIS,             // M68K_IT_PMULH
    _IMFIS,             // M68K_IT_PMULL
    _IMFIS,             // M68K_IT_POR
    0,                  // M68K_IT_PRESTORE
    0,                  // M68K_IT_PSAVE
    _IMFCC | _IMFIS,    // M68K_IT_PSCC
    _IMFIS,             // M68K_IT_PSUBB
    _IMFIS,             // M68K_IT_PSUBUSB
    _IMFIS,             // M68K_IT_PSUBUSW
    _IMFIS,             // M68K_IT_PSUBW
    0,                  // M68K_IT_PTESTR
    0,                  // M68K_IT_PTESTW
    _IMFCC,             // M68K_IT_PTRAPCC
    0,                  // M68K_IT_PVALID
    0,                  // M68K_IT_RESET
    0,                  // M68K_IT_ROL
    0,                  // M68K_IT_ROR
    0,                  // M68K_IT_ROXL
    0,                  // M68K_IT_ROXR
    0,                  // M68K_IT_RPC
    0,                  // M68K_IT_RTD
    0,                  // M68K_IT_RTE
    0,                  // M68K_IT_RTM
    0,                  // M68K_IT_RTR
    0,                  // M68K_IT_RTS
    _IMFCC | _IMFIS,    // M68K_IT_SCC
    0,                  // M68K_IT_STOP
    _IMFIS,             // M68K_IT_STORE
    _IMFIS,             // M68K_IT_STOREC
    _IMFIS,             // M68K_IT_STOREI
    _IMFIS,             // M68K_IT_STOREILM
    _IMFIS,             // M68K_IT_STOREM
    _IMFIS,             // M68K_IT_STOREM3
    0,                  // M68K_IT_SUB
    0,                  // M68K_IT_SUBA
    0,                  // M68K_IT_SUBI
    0,                  // M68K_IT_SUBQ
    0,                  // M68K_IT_SUBX
    _IMFIS,             // M68K_IT_SWAP
    _IMFIS,             // M68K_IT_TAS
    0,                  // M68K_IT_TBLS
    0,                  // M68K_IT_TBLSN
    0,                  // M68K_IT_TBLU
    0,                  // M68K_IT_TBLUN
    0,                  // M68K_IT_TOUCH
    _IMFIS,             // M68K_IT_TRANSHI
    _IMFIS,             // M68K_IT_TRANSILO
    _IMFIS,             // M68K_IT_TRANSLO
    0,                  // M68K_IT_TRAP
    _IMFCC,             // M68K_IT_TRAPCC
    0,                  // M68K_IT_TRAPV
    0,                  // M68K_IT_TST
    0,                  // M68K_IT_UNLK
    _IMFIS,             // M68K_IT_UNPACK1632
    0,                  // M68K_IT_UNPK
    _IMFIS,             // M68K_IT_VPERM
};

// fpu opmode for each master type
M68K_BYTE _M68KInstrMasterTypeFPUOpmodes[M68K_IT__SIZEOF__] =
{
    128,    // M68K_IT_INVALID
    128,    // M68K_IT_ABCD
    128,    // M68K_IT_ADD
    128,    // M68K_IT_ADDA
    128,    // M68K_IT_ADDI
    128,    // M68K_IT_ADDIW
    128,    // M68K_IT_ADDQ
    128,    // M68K_IT_ADDX
    128,    // M68K_IT_AND
    128,    // M68K_IT_ANDI
    128,    // M68K_IT_ASL
    128,    // M68K_IT_ASR
    128,    // M68K_IT_BANK
    128,    // M68K_IT_BCC
    128,    // M68K_IT_BCHG
    128,    // M68K_IT_BCLR
    128,    // M68K_IT_BFCHG
    128,    // M68K_IT_BFCLR
    128,    // M68K_IT_BFEXTS
    128,    // M68K_IT_BFEXTU
    128,    // M68K_IT_BFFFO
    128,    // M68K_IT_BFINS
    128,    // M68K_IT_BFLYB
    128,    // M68K_IT_BFLYW
    128,    // M68K_IT_BFSET
    128,    // M68K_IT_BFTST
    128,    // M68K_IT_BGND
    128,    // M68K_IT_BKPT
    128,    // M68K_IT_BRA
    128,    // M68K_IT_BSEL
    128,    // M68K_IT_BSET
    128,    // M68K_IT_BSR
    128,    // M68K_IT_BTST
    128,    // M68K_IT_C2P
    128,    // M68K_IT_CALLM
    128,    // M68K_IT_CAS
    128,    // M68K_IT_CAS2
    128,    // M68K_IT_CHK
    128,    // M68K_IT_CHK2
    128,    // M68K_IT_CINVA
    128,    // M68K_IT_CINVL
    128,    // M68K_IT_CINVP
    128,    // M68K_IT_CLR
    128,    // M68K_IT_CMP
    128,    // M68K_IT_CMP2
    128,    // M68K_IT_CMPA
    128,    // M68K_IT_CMPI
    128,    // M68K_IT_CMPIW
    128,    // M68K_IT_CMPM
    128,    // M68K_IT_CPBCC
    128,    // M68K_IT_CPDBCC
    128,    // M68K_IT_CPGEN
    128,    // M68K_IT_CPRESTORE
    128,    // M68K_IT_CPSAVE
    128,    // M68K_IT_CPSCC
    128,    // M68K_IT_CPTRAPCC
    128,    // M68K_IT_CPUSHA
    128,    // M68K_IT_CPUSHL
    128,    // M68K_IT_CPUSHP
    128,    // M68K_IT_DBCC
    128,    // M68K_IT_DIVS
    128,    // M68K_IT_DIVSL
    128,    // M68K_IT_DIVU
    128,    // M68K_IT_DIVUL
    128,    // M68K_IT_EOR
    128,    // M68K_IT_EORI
    128,    // M68K_IT_EXG
    128,    // M68K_IT_EXT
    128,    // M68K_IT_EXTB
    0x18,   // M68K_IT_FABS
    0x1c,   // M68K_IT_FACOS
    0x22,   // M68K_IT_FADD
    0x0c,   // M68K_IT_FASIN
    0x0a,   // M68K_IT_FATAN
    0x0d,   // M68K_IT_FATANH
    128,    // M68K_IT_FBCC
    0x38,   // M68K_IT_FCMP
    0x1d,   // M68K_IT_FCOS
    0x19,   // M68K_IT_FCOSH
    0x5c,   // M68K_IT_FDABS
    0x66,   // M68K_IT_FDADD
    128,    // M68K_IT_FDBCC
    0x64,   // M68K_IT_FDDIV
    0x20,   // M68K_IT_FDIV
    0x44,   // M68K_IT_FDMOVE
    0x67,   // M68K_IT_FDMUL
    0x5e,   // M68K_IT_FDNEG
    0x45,   // M68K_IT_FDSQRT
    0x6c,   // M68K_IT_FDSUB
    0x10,   // M68K_IT_FETOX
    0x08,   // M68K_IT_FETOXM1
    0x1e,   // M68K_IT_FGETEXP
    0x1f,   // M68K_IT_FGETMAN
    0x01,   // M68K_IT_FINT
    0x03,   // M68K_IT_FINTRZ
    0x15,   // M68K_IT_FLOG10
    0x16,   // M68K_IT_FLOG2
    0x14,   // M68K_IT_FLOGN
    0x06,   // M68K_IT_FLOGNP1
    0x21,   // M68K_IT_FMOD
    0x00,   // M68K_IT_FMOVE
    128,    // M68K_IT_FMOVECR
    128,    // M68K_IT_FMOVEM
    0x23,   // M68K_IT_FMUL
    0x1a,   // M68K_IT_FNEG
    128,    // M68K_IT_FNOP
    0x25,   // M68K_IT_FREM
    128,    // M68K_IT_FRESTORE
    0x58,   // M68K_IT_FSABS
    0x62,   // M68K_IT_FSADD
    128,    // M68K_IT_FSAVE
    0x26,   // M68K_IT_FSCALE
    128,    // M68K_IT_FSCC
    0x60,   // M68K_IT_FSDIV
    0x24,   // M68K_IT_FSGLDIV
    0x27,   // M68K_IT_FSGLMUL
    0x0e,   // M68K_IT_FSIN
    128,    // M68K_IT_FSINCOS
    0x02,   // M68K_IT_FSINH
    0x40,   // M68K_IT_FSMOVE
    0x63,   // M68K_IT_FSMUL
    0x5a,   // M68K_IT_FSNEG
    0x04,   // M68K_IT_FSQRT
    0x41,   // M68K_IT_FSSQRT
    0x68,   // M68K_IT_FSSUB
    0x28,   // M68K_IT_FSUB
    0x0f,   // M68K_IT_FTAN
    0x09,   // M68K_IT_FTANH
    0x12,   // M68K_IT_FTENTOX
    128,    // M68K_IT_FTRAPCC
    128,    // M68K_IT_FTST
    0x11,   // M68K_IT_FTWOTOX
    128,    // M68K_IT_ILLEGAL
    128,    // M68K_IT_JMP
    128,    // M68K_IT_JSR
    128,    // M68K_IT_LEA
    128,    // M68K_IT_LINK
    128,    // M68K_IT_LOAD
    128,    // M68K_IT_LOADI
    128,    // M68K_IT_LPSTOP
    128,    // M68K_IT_LSL
    128,    // M68K_IT_LSLQ
    128,    // M68K_IT_LSR
    128,    // M68K_IT_LSRQ
    128,    // M68K_IT_MINTERM
    128,    // M68K_IT_MOVE
    128,    // M68K_IT_MOVE16
    128,    // M68K_IT_MOVEA
    128,    // M68K_IT_MOVEC
    128,    // M68K_IT_MOVEM
    128,    // M68K_IT_MOVEP
    128,    // M68K_IT_MOVEQ
    128,    // M68K_IT_MOVES
    128,    // M68K_IT_MOVEX
    128,    // M68K_IT_MULS
    128,    // M68K_IT_MULU
    128,    // M68K_IT_NBCD
    128,    // M68K_IT_NEG
    128,    // M68K_IT_NEGX
    128,    // M68K_IT_NOP
    128,    // M68K_IT_NOT
    128,    // M68K_IT_OR
    128,    // M68K_IT_ORI
    128,    // M68K_IT_PABSB
    128,    // M68K_IT_PABSW
    128,    // M68K_IT_PACK
    128,    // M68K_IT_PACK3216
    128,    // M68K_IT_PACKUSWB
    128,    // M68K_IT_PADDB
    128,    // M68K_IT_PADDUSB
    128,    // M68K_IT_PADDUSW
    128,    // M68K_IT_PADDW
    128,    // M68K_IT_PAND
    128,    // M68K_IT_PANDN
    128,    // M68K_IT_PAVG
    128,    // M68K_IT_PBCC
    128,    // M68K_IT_PCMPEQB
    128,    // M68K_IT_PCMPEQW
    128,    // M68K_IT_PCMPGEB
    128,    // M68K_IT_PCMPGEW
    128,    // M68K_IT_PCMPGTB
    128,    // M68K_IT_PCMPGTW
    128,    // M68K_IT_PCMPHIB
    128,    // M68K_IT_PCMPHIW
    128,    // M68K_IT_PDBCC
    128,    // M68K_IT_PEA
    128,    // M68K_IT_PEOR
    128,    // M68K_IT_PERM
    128,    // M68K_IT_PFLUSH
    128,    // M68K_IT_PFLUSHA
    128,    // M68K_IT_PFLUSHAN
    128,    // M68K_IT_PFLUSHN
    128,    // M68K_IT_PFLUSHR
    128,    // M68K_IT_PFLUSHS
    128,    // M68K_IT_PLOADR
    128,    // M68K_IT_PLOADW
    128,    // M68K_IT_PLPAR
    128,    // M68K_IT_PLPAW
    128,    // M68K_IT_PMAXSB
    128,    // M68K_IT_PMAXSW
    128,    // M68K_IT_PMAXUB
    128,    // M68K_IT_PMAXUW
    128,    // M68K_IT_PMINSB
    128,    // M68K_IT_PMINSW
    128,    // M68K_IT_PMINUB
    128,    // M68K_IT_PMINUW
    128,    // M68K_IT_PMOVE
    128,    // M68K_IT_PMOVEFD
    128,    // M68K_IT_PMUL88
    128,    // M68K_IT_PMULA
    128,    // M68K_IT_PMULH
    128,    // M68K_IT_PMULL
    128,    // M68K_IT_POR
    128,    // M68K_IT_PRESTORE
    128,    // M68K_IT_PSAVE
    128,    // M68K_IT_PSCC
    128,    // M68K_IT_PSUBB
    128,    // M68K_IT_PSUBUSB
    128,    // M68K_IT_PSUBUSW
    128,    // M68K_IT_PSUBW
    128,    // M68K_IT_PTESTR
    128,    // M68K_IT_PTESTW
    128,    // M68K_IT_PTRAPCC
    128,    // M68K_IT_PVALID
    128,    // M68K_IT_RESET
    128,    // M68K_IT_ROL
    128,    // M68K_IT_ROR
    128,    // M68K_IT_ROXL
    128,    // M68K_IT_ROXR
    128,    // M68K_IT_RPC
    128,    // M68K_IT_RTD
    128,    // M68K_IT_RTE
    128,    // M68K_IT_RTM
    128,    // M68K_IT_RTR
    128,    // M68K_IT_RTS
    128,    // M68K_IT_SCC
    128,    // M68K_IT_STOP
    128,    // M68K_IT_STORE
    128,    // M68K_IT_STOREC
    128,    // M68K_IT_STOREI
    128,    // M68K_IT_STOREILM
    128,    // M68K_IT_STOREM
    128,    // M68K_IT_STOREM3
    128,    // M68K_IT_SUB
    128,    // M68K_IT_SUBA
    128,    // M68K_IT_SUBI
    128,    // M68K_IT_SUBQ
    128,    // M68K_IT_SUBX
    128,    // M68K_IT_SWAP
    128,    // M68K_IT_TAS
    128,    // M68K_IT_TBLS
    128,    // M68K_IT_TBLSN
    128,    // M68K_IT_TBLU
    128,    // M68K_IT_TBLUN
    128,    // M68K_IT_TOUCH
    128,    // M68K_IT_TRANSHI
    128,    // M68K_IT_TRANSILO
    128,    // M68K_IT_TRANSLO
    128,    // M68K_IT_TRAP
    128,    // M68K_IT_TRAPCC
    128,    // M68K_IT_TRAPV
    128,    // M68K_IT_TST
    128,    // M68K_IT_UNLK
    128,    // M68K_IT_UNPACK1632
    128,    // M68K_IT_UNPK
    128,    // M68K_IT_VPERM
};

// information for each instruction type (internal)
C_INSTRUCTION_TYPE_INFO _M68KInstrTypeInfos[IT__SIZEOF__] =
{
    {M68K_IT_INVALID    , _C(N_N_N_N_N)     , 0                 , 0             }, // IT_INVALID
    {M68K_IT_ABCD       , _C(TO_U_TO_U_O)   , 0                 , _APFAM        }, // IT_ABCD
    {M68K_IT_ADD        , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_ADD
    {M68K_IT_ADDA       , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_ADDA
    {M68K_IT_ADDI       , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_ADDI
    {M68K_IT_ADDIW      , _C(N_N_N_N_N)     , 0                 , _AAC80        }, // IT_ADDIW
    {M68K_IT_ADDQ       , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_ADDQ
    {M68K_IT_ADDX       , _C(TO_U_TO_U_O)   , 0                 , _APFAM        }, // IT_ADDX
    {M68K_IT_AND        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_AND
    {M68K_IT_ANDI       , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_ANDI
    {M68K_IT_ANDI       , _C(O_O_O_O_O)     , _IFEW1            , _APFAM        }, // IT_ANDI_TO_CCR
    {M68K_IT_ANDI       , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_ANDI_TO_SR
    {M68K_IT_ASL        , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_ASL
    {M68K_IT_ASR        , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_ASR
    {M68K_IT_BANK       , _C(N_N_N_N_N)     , 0                 , _AAC80        }, // IT_BANK
    {M68K_IT_BCC        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_BCC
    {M68K_IT_BCC        , _C(N_N_N_N_N)     , 0                 , _AP20H        }, // IT_BCC_LONG
    {M68K_IT_BCHG       , _C(N_N_O_N_N)     , 0                 , _APFAM        }, // IT_BCHG
    {M68K_IT_BCHG       , _C(N_N_O_N_N)     , _IFEW1            , _APFAM        }, // IT_BCHG_IMM
    {M68K_IT_BCLR       , _C(N_N_O_N_N)     , 0                 , _APFAM        }, // IT_BCLR
    {M68K_IT_BCLR       , _C(N_N_O_N_N)     , _IFEW1            , _APFAM        }, // IT_BCLR_IMM
    {M68K_IT_BFCHG      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFCHG
    {M68K_IT_BFCLR      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFCLR
    {M68K_IT_BFEXTS     , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFEXTS
    {M68K_IT_BFEXTU     , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFEXTU
    {M68K_IT_BFFFO      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFFFO
    {M68K_IT_BFINS      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFINS
    {M68K_IT_BFLYB      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_BFLYB
    {M68K_IT_BFLYW      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_BFLYW
    {M68K_IT_BFSET      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFSET
    {M68K_IT_BFTST      , _C(N_O_O_C_C)     , _IFEW1            , _AP20304060   }, // IT_BFTST
    {M68K_IT_BGND       , _C(N_N_N_N_N)     , 0                 , _AC32         }, // IT_BGND
    {M68K_IT_BKPT       , _C(N_N_N_N_N)     , 0                 , _AP10H        }, // IT_BKPT
    {M68K_IT_BRA        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_BRA
    {M68K_IT_BSEL       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_BSEL
    {M68K_IT_BSET       , _C(N_N_O_N_N)     , 0                 , _APFAM        }, // IT_BSET
    {M68K_IT_BSET       , _C(N_N_O_N_N)     , _IFEW1            , _APFAM        }, // IT_BSET_IMM
    {M68K_IT_BSR        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_BSR
    {M68K_IT_BTST       , _C(N_N_O_N_N)     , 0                 , _APFAM        }, // IT_BTST
    {M68K_IT_BTST       , _C(N_N_O_N_N)     , _IFEW1            , _APFAM        }, // IT_BTST_IMM
    {M68K_IT_C2P        , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_C2P
    {M68K_IT_CALLM      , _C(N_N_N_N_N)     , _IFEW1            , _A20          }, // IT_CALLM
    {M68K_IT_CAS        , _C(N_O_O_O_O)     , _IFEW1            , _AP20304060   }, // IT_CAS
    {M68K_IT_CAS2       , _C(N_O_O_O_O)     , _IFEW1 | _IFEW2   , _AP20304060   }, // IT_CAS2
    {M68K_IT_CHK        , _C(N_O_U_U_U)     , 0                 , _APFAM        }, // IT_CHK
    {M68K_IT_CHK        , _C(N_O_U_U_U)     , 0                 , _AP20H        }, // IT_CHK_LONG
    {M68K_IT_CHK2       , _C(N_U_O_U_O)     , _IFEW1            , _AP20H        }, // IT_CHK2
    {M68K_IT_CINVA      , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CINVA
    {M68K_IT_CINVL      , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CINVL
    {M68K_IT_CINVP      , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CINVP
    {M68K_IT_CLR        , _C(N_C_S_C_C)     , _IFBEWAB          , _APFAM        }, // IT_CLR
    {M68K_IT_CMP        , _C(N_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_CMP
    {M68K_IT_CMP2       , _C(N_U_O_U_O)     , _IFEW1            , _AP20H        }, // IT_CMP2
    {M68K_IT_CMPA       , _C(N_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_CMPA
    {M68K_IT_CMPI       , _C(N_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_CMPI
    {M68K_IT_CMPIW      , _C(N_O_O_O_O)     , 0                 , _AAC80        }, // IT_CMPIW
    {M68K_IT_CMPM       , _C(N_O_O_O_O)     , 0                 , _APFAM        }, // IT_CMPM
    {M68K_IT_CPBCC      , _C(N_N_N_N_N)     , _IFCPX            , _AP2030       }, // IT_CPBCC
    {M68K_IT_CPDBCC     , _C(N_N_N_N_N)     , _IFEW1 | _IFCPX   , _AP2030       }, // IT_CPDBCC
    {M68K_IT_CPGEN      , _C(N_N_N_N_N)     , _IFCPX            , _AP2030       }, // IT_CPGEN
    {M68K_IT_CPRESTORE  , _C(N_N_N_N_N)     , _IFCPX            , _AP2030       }, // IT_CPRESTORE
    {M68K_IT_CPSAVE     , _C(N_N_N_N_N)     , _IFCPX            , _AP2030       }, // IT_CPSAVE
    {M68K_IT_CPSCC      , _C(N_N_N_N_N)     , _IFEW1 | _IFCPX   , _AP2030       }, // IT_CPSCC
    {M68K_IT_CPTRAPCC   , _C(N_N_N_N_N)     , _IFEW1 | _IFCPX   , _AP2030       }, // IT_CPTRAPCC
    {M68K_IT_CPUSHA     , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CPUSHA
    {M68K_IT_CPUSHL     , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CPUSHL
    {M68K_IT_CPUSHP     , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_CPUSHP
    {M68K_IT_DBCC       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_DBCC
    {M68K_IT_DIVS       , _C(N_O_O_O_C)     , 0                 , _APFAM        }, // IT_DIVS
    {M68K_IT_DIVS       , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_DIVS_LONG
    {M68K_IT_DIVSL      , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_DIVSL
    {M68K_IT_DIVU       , _C(N_O_O_O_C)     , 0                 , _APFAM        }, // IT_DIVU
    {M68K_IT_DIVU       , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_DIVU_LONG
    {M68K_IT_DIVUL      , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_DIVUL
    {M68K_IT_EOR        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_EOR
    {M68K_IT_EORI       , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_EORI
    {M68K_IT_EORI       , _C(O_O_O_O_O)     , _IFEW1            , _APFAM        }, // IT_EORI_TO_CCR
    {M68K_IT_EORI       , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_EORI_TO_SR
    {M68K_IT_EXG        , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_EXG_A_A
    {M68K_IT_EXG        , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_EXG_D_A
    {M68K_IT_EXG        , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_EXG_D_D
    {M68K_IT_EXT        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_EXT
    {M68K_IT_EXTB       , _C(N_O_O_C_C)     , _IFBEWAB          , _AP20H        }, // IT_EXTB
    {M68K_IT_FBCC       , _C(N_N_N_N_N)     , 0                 , _AFPU         }, // IT_FBCC
    {M68K_IT_FBCC       , _C(N_N_N_N_N)     , 0                 , _AFPU         }, // IT_FBCC_LONG
    {M68K_IT_FDBCC      , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FDBCC
    {M68K_IT_FMOVE      , _C(N_N_N_N_N)     , _IFEW1 | _IFFPMKF , _AFPU         }, // IT_FMOVE_DKFACTOR
    {M68K_IT_FMOVE      , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FMOVE_FPCR
    {M68K_IT_FMOVE      , _C(N_N_N_N_N)     , _IFEW1 | _IFFPMKF , _AFPU         }, // IT_FMOVE_NKFACTOR
    {M68K_IT_FMOVE      , _C(N_N_N_N_N)     , _IFEW1 | _IFFPMKF , _AFPU         }, // IT_FMOVE_SKFACTOR
    {M68K_IT_FMOVECR    , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FMOVECR
    {M68K_IT_FMOVEM     , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FMOVEM
    {M68K_IT_FNOP       , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FNOP
    {M68K_IT_INVALID    , _C(N_N_N_N_N)     , _IFEW1 | _IFFPOP  , _AFPU         }, // IT_FPOP
    {M68K_IT_INVALID    , _C(N_N_N_N_N)     , _IFEW1 | _IFFPOP  , _AFPU         }, // IT_FPOP_REG
    {M68K_IT_FRESTORE   , _C(N_N_N_N_N)     , 0                 , _AFPU         }, // IT_FRESTORE
    {M68K_IT_FSAVE      , _C(N_N_N_N_N)     , 0                 , _AFPU         }, // IT_FSAVE
    {M68K_IT_FSCC       , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FSCC
    {M68K_IT_FSINCOS    , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FSINCOS
    {M68K_IT_FTRAPCC    , _C(N_N_N_N_N)     , _IFEW1            , _AFPU         }, // IT_FTRAPCC
    {M68K_IT_FTST       , _C(N_N_N_N_N)     , _IFEW1 | _IFBEWA  , _AFPU         }, // IT_FTST
    {M68K_IT_ILLEGAL    , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_ILLEGAL
    {M68K_IT_JMP        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_JMP
    {M68K_IT_JSR        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_JSR
    {M68K_IT_LEA        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_LEA
    {M68K_IT_LINK       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_LINK
    {M68K_IT_LINK       , _C(N_N_N_N_N)     , 0                 , _AP20H        }, // IT_LINK_LONG
    {M68K_IT_LOAD       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_LOAD
    {M68K_IT_LOADI      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_LOADI
    {M68K_IT_LPSTOP     , _C(O_O_O_O_O)     , _IFEW1            , _AC32         }, // IT_LPSTOP
    {M68K_IT_LSL        , _C(O_O_O_C_O)     , _IFBEWAB          , _APFAM        }, // IT_LSL
    {M68K_IT_LSLQ       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_LSLQ
    {M68K_IT_LSR        , _C(O_O_O_C_O)     , _IFBEWAB          , _APFAM        }, // IT_LSR
    {M68K_IT_LSRQ       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_LSRQ
    {M68K_IT_MINTERM    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_MINTERM
    {M68K_IT_MOVE       , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_MOVE
    {M68K_IT_MOVE       , _C(N_O_O_C_C)     , 0                 , _AAC80        }, // IT_MOVE_AC80
    {M68K_IT_MOVE       , _C(N_N_N_N_N)     , 0                 , _AP10H        }, // IT_MOVE_FROM_CCR
    {M68K_IT_MOVE       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_MOVE_FROM_SR
    {M68K_IT_MOVE       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_MOVE_FROM_USP
    {M68K_IT_MOVE       , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_MOVE_TO_CCR
    {M68K_IT_MOVE       , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_MOVE_TO_SR
    {M68K_IT_MOVE       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_MOVE_TO_USP
    {M68K_IT_MOVE16     , _C(N_N_N_N_N)     , 0                 , _AP4060       }, // IT_MOVE16
    {M68K_IT_MOVE16     , _C(N_N_N_N_N)     , _IFEW1            , _AP40H        }, // IT_MOVE16_PD_PD
    {M68K_IT_MOVEA      , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_MOVEA
    {M68K_IT_MOVEC      , _C(N_N_N_N_N)     , _IFEW1            , _AP10H        }, // IT_MOVEC
    {M68K_IT_MOVEM      , _C(N_N_N_N_N)     , _IFEW1 | _IFLREGS , _APFAM        }, // IT_MOVEM
    {M68K_IT_MOVEP      , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_MOVEP
    {M68K_IT_MOVEQ      , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_MOVEQ
    {M68K_IT_MOVES      , _C(N_N_N_N_N)     , _IFEW1            , _AP10H        }, // IT_MOVES
    {M68K_IT_MOVES      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_MOVES_AC80
    {M68K_IT_MOVEX      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_MOVEX
    {M68K_IT_MULS       , _C(N_O_O_O_C)     , 0                 , _APFAM        }, // IT_MULS
    {M68K_IT_MULS       , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_MULS_LONG
    {M68K_IT_MULU       , _C(N_O_O_O_C)     , 0                 , _APFAM        }, // IT_MULU
    {M68K_IT_MULU       , _C(N_O_O_O_C)     , _IFEW1            , _AP20H        }, // IT_MULU_LONG
    {M68K_IT_NBCD       , _C(TO_U_TO_U_O)   , 0                 , _APFAM        }, // IT_NBCD
    {M68K_IT_NEG        , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_NEG
    {M68K_IT_NEGX       , _C(TO_O_TO_O_O)   , 0                 , _APFAM        }, // IT_NEGX
    {M68K_IT_NOP        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_NOP
    {M68K_IT_NOT        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_NOT
    {M68K_IT_OR         , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_OR
    {M68K_IT_ORI        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_ORI
    {M68K_IT_ORI        , _C(O_O_O_O_O)     , _IFEW1            , _APFAM        }, // IT_ORI_TO_CCR
    {M68K_IT_ORI        , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_ORI_TO_SR
    {M68K_IT_PABSB      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PABSB
    {M68K_IT_PABSW      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PABSW
    {M68K_IT_PACK       , _C(N_N_N_N_N)     , 0                 , _AP20304060   }, // IT_PACK
    {M68K_IT_PACK3216   , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PACK3216
    {M68K_IT_PACKUSWB   , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PACKUSWB
    {M68K_IT_PADDB      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PADDB
    {M68K_IT_PADDUSB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PADDUSB
    {M68K_IT_PADDUSW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PADDUSW
    {M68K_IT_PADDW      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PADDW
    {M68K_IT_PAND       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PAND
    {M68K_IT_PANDN      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PANDN
    {M68K_IT_PAVG       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PAVG
    {M68K_IT_PBCC       , _C(N_N_N_N_N)     , 0                 , _AMMU51       }, // IT_PBCC
    {M68K_IT_PBCC       , _C(N_N_N_N_N)     , 0                 , _AMMU51       }, // IT_PBCC_LONG
    {M68K_IT_PCMPEQB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPEQB
    {M68K_IT_PCMPEQW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPEQW
    {M68K_IT_PCMPGEB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPGEB
    {M68K_IT_PCMPGEW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPGEW
    {M68K_IT_PCMPGTB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPGTB
    {M68K_IT_PCMPGTW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPGTW
    {M68K_IT_PCMPHIB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPHIB
    {M68K_IT_PCMPHIW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PCMPHIW
    {M68K_IT_PDBCC      , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PDBCC
    {M68K_IT_PEA        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_PEA
    {M68K_IT_PEOR       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PEOR
    {M68K_IT_PERM       , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PERM
    {M68K_IT_PFLUSH     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PFLUSH_3051
    {M68K_IT_PFLUSH     , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_PFLUSH_4060
    {M68K_IT_PFLUSHA    , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PFLUSHA_30
    {M68K_IT_PFLUSHA    , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_PFLUSHA_4060
    {M68K_IT_PFLUSHAN   , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_PFLUSHAN_4060
    {M68K_IT_PFLUSHN    , _C(N_N_N_N_N)     , 0                 , _AMMU4060     }, // IT_PFLUSHN_4060
    {M68K_IT_PFLUSHR    , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PFLUSHR_51
    {M68K_IT_PFLUSHS    , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PFLUSHS_51
    {M68K_IT_PLOADR     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PLOADR_3051
    {M68K_IT_PLOADW     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PLOADW_3051
    {M68K_IT_PLPAR      , _C(N_N_N_N_N)     , 0                 , _AP60LC       }, // IT_PLPAR
    {M68K_IT_PLPAW      , _C(N_N_N_N_N)     , 0                 , _AP60LC       }, // IT_PLPAW
    {M68K_IT_PMAXSB     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXSB
    {M68K_IT_PMAXSW     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXSW
    {M68K_IT_PMAXUB     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXUB
    {M68K_IT_PMAXUW     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXUW
    {M68K_IT_PMINSB     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXSB
    {M68K_IT_PMINSW     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXSW
    {M68K_IT_PMINUB     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXUB
    {M68K_IT_PMINUW     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMAXUW
    {M68K_IT_PMOVE      , _C(N_N_N_N_N)     , _IFEW1            , _AMMU30       }, // IT_PMOVE_30
    {M68K_IT_PMOVE      , _C(N_N_N_N_N)     , _IFEW1            , _AMMU30EC     }, // IT_PMOVE_30EC
    {M68K_IT_PMOVE      , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PMOVE_51
    {M68K_IT_PMOVEFD    , _C(N_N_N_N_N)     , _IFEW1            , _AMMU30       }, // IT_PMOVEFD_30
    {M68K_IT_PMUL88     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMUL88
    {M68K_IT_PMULA      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMULA
    {M68K_IT_PMULH      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMULH
    {M68K_IT_PMULL      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PMULL
    {M68K_IT_POR        , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_POR
    {M68K_IT_PRESTORE   , _C(N_N_N_N_N)     , 0                 , _AMMU51       }, // IT_PRESTORE
    {M68K_IT_PSAVE      , _C(N_N_N_N_N)     , 0                 , _AMMU51       }, // IT_PSAVE
    {M68K_IT_PSCC       , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PSCC
    {M68K_IT_PSUBB      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PSUBB
    {M68K_IT_PSUBUSB    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PSUBUSB
    {M68K_IT_PSUBUSW    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PSUBUSW
    {M68K_IT_PSUBW      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_PSUBW
    {M68K_IT_PTESTR     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PTESTR_3051
    {M68K_IT_PTESTR     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU30EC     }, // IT_PTESTR_30EC
    {M68K_IT_PTESTR     , _C(N_N_N_N_N)     , 0                 , _AMMU4040EC   }, // IT_PTESTR_4040EC
    {M68K_IT_PTESTW     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU3051     }, // IT_PTESTW_3051
    {M68K_IT_PTESTW     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU30EC     }, // IT_PTESTW_30EC
    {M68K_IT_PTESTW     , _C(N_N_N_N_N)     , 0                 , _AMMU4040EC   }, // IT_PTESTW_4040EC
    {M68K_IT_PTRAPCC    , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PTRAPCC
    {M68K_IT_PVALID     , _C(N_N_N_N_N)     , _IFEW1            , _AMMU51       }, // IT_PVALID_51
    {M68K_IT_RESET      , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_RESET
    {M68K_IT_ROL        , _C(N_O_O_C_O)     , 0                 , _APFAM        }, // IT_ROL
    {M68K_IT_ROR        , _C(N_O_O_C_O)     , 0                 , _APFAM        }, // IT_ROR
    {M68K_IT_ROXL       , _C(TO_O_O_C_O)    , 0                 , _APFAM        }, // IT_ROXL
    {M68K_IT_ROXR       , _C(TO_O_O_C_O)    , 0                 , _APFAM        }, // IT_ROXR
    {M68K_IT_RPC        , _C(O_O_O_O_O)     , 0                 , _AAC80        }, // IT_RPC
    {M68K_IT_RTD        , _C(N_N_N_N_N)     , 0                 , _AP10H        }, // IT_RTD
    {M68K_IT_RTE        , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_RTE
    {M68K_IT_RTM        , _C(O_O_O_O_O)     , 0                 , _A20          }, // IT_RTM
    {M68K_IT_RTR        , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_RTR
    {M68K_IT_RTS        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_RTS
    {M68K_IT_SCC        , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_SCC
    {M68K_IT_STOP       , _C(O_O_O_O_O)     , 0                 , _APFAM        }, // IT_STOP
    {M68K_IT_STORE      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STORE
    {M68K_IT_STOREC     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STOREC
    {M68K_IT_STOREI     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STOREI
    {M68K_IT_STOREILM   , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STOREILM
    {M68K_IT_STOREM     , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STOREM
    {M68K_IT_STOREM3    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_STOREM3
    {M68K_IT_SUB        , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_SUB
    {M68K_IT_SUBA       , _C(N_N_N_N_N)     , _IFBEWAB          , _APFAM        }, // IT_SUBA
    {M68K_IT_SUBI       , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_SUBI
    {M68K_IT_SUBQ       , _C(O_O_O_O_O)     , _IFBEWAB          , _APFAM        }, // IT_SUBQ
    {M68K_IT_SUBX       , _C(TO_U_TO_U_O)   , 0                 , _APFAM        }, // IT_SUBX
    {M68K_IT_SWAP       , _C(N_O_O_C_C)     , 0                 , _APFAM        }, // IT_SWAP
    {M68K_IT_TAS        , _C(N_O_O_C_C)     , 0                 , _APFAM        }, // IT_TAS
    {M68K_IT_TBLS       , _C(N_O_O_O_C)     , _IFEW1            , _AC32         }, // IT_TBLS
    {M68K_IT_TBLSN      , _C(N_O_O_O_C)     , _IFEW1            , _AC32         }, // IT_TBLSN
    {M68K_IT_TBLU       , _C(N_O_O_O_C)     , _IFEW1            , _AC32         }, // IT_TBLU
    {M68K_IT_TBLUN      , _C(N_O_O_O_C)     , _IFEW1            , _AC32         }, // IT_TBLUN
    {M68K_IT_TOUCH      , _C(N_N_N_N_N)     , 0                 , _AAC80        }, // IT_TOUCH
    {M68K_IT_TRANSHI    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_TRANSHI
    {M68K_IT_TRANSILO   , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_TRANSILO
    {M68K_IT_TRANSLO    , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_TRANSLO
    {M68K_IT_TRAP       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_TRAP
    {M68K_IT_TRAPCC     , _C(N_N_N_N_N)     , 0                 , _AP20H        }, // IT_TRAPCC
    {M68K_IT_TRAPV      , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_TRAPV
    {M68K_IT_TST        , _C(N_O_O_C_C)     , _IFBEWAB          , _APFAM        }, // IT_TST
    {M68K_IT_UNLK       , _C(N_N_N_N_N)     , 0                 , _APFAM        }, // IT_UNLK
    {M68K_IT_UNPACK1632 , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_UNPACK1632
    {M68K_IT_UNPK       , _C(N_N_N_N_N)     , 0                 , _AP20304060   }, // IT_UNPK
    {M68K_IT_VPERM      , _C(N_N_N_N_N)     , _IFEW1            , _AAC80        }, // IT_VPERM
};
