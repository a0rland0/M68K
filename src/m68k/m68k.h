#ifndef __M68K_H__
#define __M68K_H__

//
// 2022 a0r1
//
// mt safe, no sync
//
// format specifier for the M68KDisassembleText functions (except M68KDisassembleTextForAssembler):
//
//      %[case][size]specifier
//
// where
//  
//      case = one of the following chars
//
//          L   = final text is converted to lowercase
//          U   = final text is converted to uppercase
//
//      size = decimal number that defines the maximum size for the converted text;
//             padding chars are always added after the text (right)
//
//      specifier = one of the following chars
//
//          %   = generate the % char
//          -   = generate a separator i.e. ' ' (or a custom separator)
//          a   = instruction address (according to the supplied Address parameter)
//          m   = instruction mnemonic (without size)
//          o   = instruction operands (includes operand separators and space separators)
//          s   = instruction size (separator + size; ex.: .b)
//          w   = opcode words
//
// format of the instructions that are supported by the M68KAssembleText function
//
//  <space>*<mnemonic><size>?<space>+(<operand>(<space>*,<space>*<operand>)*)?
//
// where
//
//      space       = [\t\n\r ]
//      mnemonic    = [a-z][0-9a-z]*
//      size        = \.[bdlpqswx]
//      operand     = %<type>\(<space>*[value]<space>*(,<space>*[value]<space>*)*\)
//      type        = one of
//          ab      = address using byte displacement: <addr>
//          al      = address using word displacement: <addr>
//          aw      = address using long displacement: <addr>
//          cc      = condition code: <cc>
//          cid     = coprocessor id: <imm:0-7>
//          cidcc   = coprocessor id and condition code: <imm:0..7>,<imm:0..0x3f>
//          ct      = cache type: <cache type>
//          db      = byte displacement: <imm:0..0xff>
//          dkf     = dynamic k-factor: <dreg>
//          dl      = word displacement: <imm:0..0xffffffff>
//          dw      = long displacement: <imm:0..0xffff>
//          fcc     = fpu condition code: <fpu cc>
//          fcl     = fpu control register list: <fcreg list>
//          fl      = fpu register list: <freg list>
//          ib      = immediate byte: <imm:0..0xff>
//          id      = immediate double: <double>
//          il      = immediate long: <imm:0..0xffffffff>
//          ip      = immediate packed decimal: <immpd:0..0xffffffffffffffffffffffff>
//          iq      = immediate quad: <immq:0..0xffffffffffffffff>
//          is      = immediate single: <single>
//          iw      = immediate word: <imm:0..0xffff>
//          ix      = immediate extended: <extended>
//          m       = memory: <mem param>(,<mem param>)*
//          mcc     = mmu condition code: <mmu cc>
//          md      = memory with pre-decrement: <areg>
//          mi      = memory with post-increment: <areg>
//          mp      = memory pair: <dreg>|<areg>,<dreg>|<areg>
//          ow      = offset and width: <imm:0..31>|<dreg>,<imm:1..32>|<dreg>
//          r       = register: <reg>
//          rl      = register list: <reg list>
//          rp      = register pair: <dreg>,<dreg>
//          skf     = static k-factor: <imm:-64..63>
//          vrp2    = virtual register pair multiple of 2: <dreg>,<dreg>
//          vrp4    = virtual register pair multiple of 4: <dreg>,<dreg>
//      addr        = &<imm:0..0xffffffff>
//      areg        = address register; check <reg>
//      areg range  = <areg>(-<areg>)?
//      base reg    = <areg>|pc|zpc
//      index reg   = (<areg>|<dreg>)(\.[wl])?
//      cache type  = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-4
//      cc          = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 4-172
//      disp        = <imm:0..0xffffffff><size>?
//      double      = [+-]?(Inf|NaN|0(x[01]\.[0-9a-f]{1,13}p[+-]?[0-9]{1,4})?)
//      dreg        = data register; check <reg>
//      dreg range  = <dreg>(-<dreg>)?
//      extended    = [+-]?(Ind|Inf|NaN|PInf|PNaN|QNaN|QInf|0(x[01]\.[0-9a-f]{1,16}p[+-]?[0-9]{1,5})?)
//      fpu cc      = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 3-31
//      fcreg       = fpu control register; check <reg>
//      fcreg list  = 0|<fcreg>(/<fcreg>(/<fcreg>)?)?
//      freg        = fpu register; check <reg>
//      freg list   = 0|<freg>(-<freg>)?(/<freg>(-<freg>)?)*
//      imm         = [+-]?(0((x[0-9a-f]{1,8})|[0-9]{1,9})|[1-9][0-9]{1,9})
//      immpd       = 0x[0-9a-f]{1,24}
//      immq        = 0x[0-9a-f]{1,16}
//      mem param   = <addr> | <disp> | <base reg> | <index reg>\*[1248] | \[ <space>*<mem param><space>*(,<space>*<mem param><space>*)+ \]
//      mmu cc      = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-61
//      reg         = [a-z][0-9a-z]*
//      reg list    = 0|(<areg range>|<dreg range>)(/(<areg range>|<dreg range>))*
//      single      = [+-]?(Inf|NaN|0(x[01]\.[0-9a-f]{1,6}p[+-]?[0-9]{1,3})?)
//
// note: the parser is not case-sensitive; for example 0X1234 and 0xABCD are not valid hexadecimal numbers
//

// use this macro to optimize the structures in terms of space (might generate slower code)
//#define M68K_OPTIMIZE_SPACE

// use this macro to compile the code assuming that the target machine is big-endian
//#define M68K_TARGET_IS_BIG_ENDIAN

// use this macro to compile the code assuming that the target machine is 64-bit
//#define M68K_TARGET_IS_64_BIT
#include <stdint.h>

#if UINTPTR_MAX == 0xffffffffffffffffULL && !defined(M68K_TARGET_IS_64_BIT)
#define M68K_TARGET_IS_64_BIT
#endif

// use this macro to include internal functions
#define M68K_INTERNAL_FUNCTIONS

#define M68K_CONST                              const
#define M68K_DEFAULT_FORMAT                     "%8a%-%m%s%-%o"                             // default format for the disassembled instruction
#define M68K_FALSE                              0
#define M68K_MAXIMUM_NUMBER_OPERANDS            4                                           // maximum number of operands for an instruction
#define M68K_MAXIMUM_NUMBER_VALUES_OPERAND      2                                           // maximum number of values for an operand
#define M68K_MAXIMUM_SIZE_CUSTOM_OUTPUT_BUFFER  128                                         // maximum size for the output buffer that is used by a custom function
#define M68K_MAXIMUM_SIZE_INSTRUCTION_BYTES     (2 * M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS)   // maximum size for an instruction in bytes
#define M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS     11                                          // maximum size for an instruction in words (opcode 1 + full extension 1 + displacement 2 + displacement 2 + full extension 1 + displacement 2 + displacement 2)
#define M68K_NULL                               ((void *)0UL)
#define M68K_TRUE                               1

typedef uint8_t                     M68K_BOOL, *PM68K_BOOL;
typedef uint8_t                     M68K_BYTE, *PM68K_BYTE;
typedef uint8_t M68K_CONST          M68KC_BYTE, *PM68KC_BYTE;
typedef char                        M68K_CHAR, *PM68K_CHAR, *PM68K_STR;
typedef char M68K_CONST             M68KC_CHAR, *PM68KC_CHAR, *PM68KC_STR;
typedef double                      M68K_DOUBLE, *PM68K_DOUBLE;
typedef uint32_t                    M68K_DWORD, *PM68K_DWORD;
typedef M68K_DWORD M68K_CONST       M68KC_DWORD, *PM68KC_DWORD;
typedef int32_t                     M68K_INT, *PM68K_INT;
typedef int8_t                      M68K_SBYTE, *PM68K_SBYTE;
typedef float                       M68K_SINGLE, *PM68K_SINGLE;
typedef int16_t                     M68K_SWORD, *PM68K_SWORD;
typedef int32_t                     M68K_SDWORD, *PM68K_SDWORD;
typedef uint32_t                    M68K_UINT, *PM68K_UINT;
typedef uint16_t                    M68K_WORD, *PM68K_WORD;
typedef M68K_WORD M68K_CONST        M68KC_WORD, *PM68KC_WORD;
typedef void                        *PM68K_VOID;

#ifdef M68K_TARGET_IS_64_BIT
#define M68K_LUINT_MAX 0xffffffffffffffffULL

typedef unsigned long long int  M68K_LUINT, *PM68K_LUINT;
typedef unsigned long long int  M68K_LWORD, *PM68K_LWORD;
typedef long long int           M68K_SLWORD, *PM68K_SLWORD;
#else
#define M68K_LUINT_MAX 0xffffffffUL

typedef unsigned long int       M68K_LUINT, *PM68K_LUINT;
typedef unsigned long int       M68K_LWORD, *PM68K_LWORD;
typedef long int                M68K_SLWORD, *PM68K_SLWORD;
#endif

typedef enum M68K_ARCHITECTURE
{
    M68K_ARCH_68000         = 0x00000001,
    M68K_ARCH_68008         = 0x00000002,
    M68K_ARCH_68010         = 0x00000004,
    M68K_ARCH_68020         = 0x00000008,
    M68K_ARCH_68030         = 0x00000010,
    M68K_ARCH_68EC030       = 0x00000020,       // no mmu
    M68K_ARCH_68040         = 0x00000040,
    M68K_ARCH_68EC040       = 0x00000080,       // no mmu, no fpu
    M68K_ARCH_68LC040       = 0x00000100,       // mmu, no fpu
    M68K_ARCH_68060         = 0x00000200,
    M68K_ARCH_68EC060       = 0x00000400,       // no mmu, no fpu
    M68K_ARCH_68LC060       = 0x00000800,       // mmu, no fpu
    M68K_ARCH_68851         = 0x00001000,       // mmu
    M68K_ARCH_68881         = 0x00002000,       // fpu
    M68K_ARCH_68882         = 0x00004000,       // fpu
    M68K_ARCH_CPU32         = 0x00008000,
    M68K_ARCH_AC68080       = 0x00010000,       // apollo core 68080, no mmu, fpu, vex
    M68K_ARCH_68EC000       = M68K_ARCH_68000,
    M68K_ARCH_68HC000       = M68K_ARCH_68000,
    M68K_ARCH_68HC020       = M68K_ARCH_68020,
    M68K_ARCH_68040V        = M68K_ARCH_68040,
    M68K_ARCH_68EC040V      = M68K_ARCH_68EC040,
    M68K_ARCH_68LC040V      = M68K_ARCH_68LC040,
    M68K_ARCH__ALL__        = 0x0001ffff,
    M68K_ARCH__FPU__        = 0x00016240,       // 68881, 68882, 68040, 68060 or ac68080 with fpu support
    M68K_ARCH__FPU4060__    = 0x00010240,       // 68040 or 68060 with fpu support (or ac68080)
    M68K_ARCH__MMU__        = 0x00001b50,       // 68030, 68040, 68060 with mmu support or 68851
    M68K_ARCH__MMU30__      = 0x00000010,       // 68030 with mmu support
    M68K_ARCH__MMU3051__    = 0x00001010,       // 68030 with mmu support or 68851
    M68K_ARCH__MMU30EC__    = 0x00000020,       // 68EC030 without mmu support
    M68K_ARCH__MMU40__      = 0x00000140,       // 68040 with mmu support
    M68K_ARCH__MMU4040EC__  = 0x000001c0,       // 68040 with or without mmu support
    M68K_ARCH__MMU4060__    = 0x00000b40,       // 68040 or 68060 with mmu support
    M68K_ARCH__MMU51__      = 0x00001000,       // 68851
    M68K_ARCH__P08H__       = 0x00018ffe,       // 68008 or higher (includes cpu32)
    M68K_ARCH__P10H__       = 0x00018ffc,       // 68010 or higher (includes cpu32)
    M68K_ARCH__P10L__       = 0x00000007,       // 68010 or lower
    M68K_ARCH__P2030__      = 0x00000038,       // 68020 or 68030
    M68K_ARCH__P203040__    = 0x000001f8,       // 68020, 68030 or 68040
    M68K_ARCH__P20304060__  = 0x00010ff8,       // 68020, 68030, 68040 or 68060 (or ac68080)
    M68K_ARCH__P20H__       = 0x00018ff8,       // 68020 or higher (includes cpu32)
    M68K_ARCH__P4060__      = 0x00010fc0,       // 68040 or 68060 (or ac68080)
    M68K_ARCH__P4060EC__    = 0x00000480,       // 68EC040 or 68EC060
    M68K_ARCH__P40H__       = 0x00018fc0,       // 68040 or higher (includes cpu32)
    M68K_ARCH__P60ECLC__    = 0x00010e00,       // 68060, 68EC060 or 68LC060 (or ac68080)
    M68K_ARCH__P60H__       = 0x00010e00,       // 68060 or higher
    M68K_ARCH__P60LC__      = 0x00000c00,       // 68060 or 68LC060
    M68K_ARCH__PFAM__       = 0x00018fff,       // family of processors (includes cpu32)
} M68K_ARCHITECTURE, *PM68K_ARCHITECTURE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_DWORD M68K_ARCHITECTURE_VALUE, *PM68K_ARCHITECTURE_VALUE;
#else
typedef M68K_ARCHITECTURE M68K_ARCHITECTURE_VALUE, *PM68K_ARCHITECTURE_VALUE;
#endif

typedef enum M68K_CACHE_TYPE
{
    M68K_CT_NONE        = 0,    // no cache = no operation
    M68K_CT_DATA        = 1,    // data cache
    M68K_CT_INSTRUCTION = 2,    // instruction cache
    M68K_CT_BOTH        = 3,    // data and instruction cache
    M68K_CT__SIZEOF__,
} M68K_CACHE_TYPE, *PM68K_CACHE_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_CACHE_TYPE_VALUE, *PM68K_CACHE_TYPE_VALUE;
#else
typedef M68K_CACHE_TYPE M68K_CACHE_TYPE_VALUE, *PM68K_CACHE_TYPE_VALUE;
#endif

typedef enum M68K_COPROCESSOR_ID
{
    M68K_CID_MMU        = 0,    // 68851
    M68K_CID_FPU        = 1,    // 68881/68882
    M68K_CID_RESERVED_2 = 2,
    M68K_CID_RESERVED_3 = 3,
    M68K_CID_RESERVED_4 = 4,
    M68K_CID_RESERVED_5 = 5,
    M68K_CID_USER_6     = 6,
    M68K_CID_AC80       = 7,    // apollo core 68080
    M68K_CID__SIZEOF__,
} M68K_COPROCESSOR_ID, *PM68K_COPROCESSOR_ID;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_COPROCESSOR_ID_VALUE, *PM68K_COPROCESSOR_ID_VALUE;
#else
typedef M68K_COPROCESSOR_ID M68K_COPROCESSOR_ID_VALUE, *PM68K_COPROCESSOR_ID_VALUE;
#endif

typedef M68K_BYTE M68K_COPROCESSOR_CONDITION_CODE_VALUE, *PM68K_COPROCESSOR_CONDITION_CODE_VALUE;

typedef enum M68K_CONDITION_CODE
{
    M68K_CC_T   = 0,            // 1 = true
    M68K_CC_F   = 1,            // 0 = false
    M68K_CC_HI  = 2,            // ~C & ~Z = higher (unsigned)
    M68K_CC_LS  = 3,            // C | Z = lower or same (unsigned)
    M68K_CC_CC  = 4,            // ~C = carry clear
    M68K_CC_HS  = M68K_CC_CC,   // ~C = higher or the same (unsigned)
    M68K_CC_CS  = 5,            // C = carry set
    M68K_CC_LO  = M68K_CC_CS,   // C = lower (unsigned)
    M68K_CC_NE  = 6,            // ~Z = not equal (not zero)
    M68K_CC_EQ  = 7,            // Z = equal (zero)
    M68K_CC_VC  = 8,            // ~V = overflow clear
    M68K_CC_VS  = 9,            // V = overflow set
    M68K_CC_PL  = 10,           // ~N = plus (zero or positive)
    M68K_CC_MI  = 11,           // N = minus (negative)
    M68K_CC_GE  = 12,           // (N & V) | (~N & ~V) = greater or equal (signed)
    M68K_CC_LT  = 13,           // (N & ~V) | (~N & V) = less than (signed)
    M68K_CC_GT  = 14,           // ((N & V) | (~N & ~V)) & ~Z = greater than (signed)
    M68K_CC_LE  = 15,           // Z | (N & ~V) | (~N & V) = less than or equal (signed)
    M68K_CC__SIZEOF__,
} M68K_CONDITION_CODE, *PM68K_CONDITION_CODE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_CONDITION_CODE_VALUE, *PM68K_CONDITION_CODE_VALUE;
#else
typedef M68K_CONDITION_CODE M68K_CONDITION_CODE_VALUE, *PM68K_CONDITION_CODE_VALUE;
#endif

typedef enum M68K_CONDITION_CODE_FLAG
{
    M68K_CCF_C = 0, // carry
    M68K_CCF_V = 1, // overflow
    M68K_CCF_Z = 2, // zero
    M68K_CCF_N = 3, // negative
    M68K_CCF_X = 4, // extend
    M68K_CCF__SIZEOF__,
} M68K_CONDITION_CODE_FLAG, *PM68K_CONDITION_CODE_FLAG;

typedef enum M68K_CONDITION_CODE_FLAG_ACTION
{
    M68K_CCA_NONE       = 0x00, // N (-)
    M68K_CCA_CLEAR      = 0x01, // C (0)
    M68K_CCA_OPERATION  = 0x02, // O (* or ?)
    M68K_CCA_SET        = 0x04, // S (1)
    M68K_CCA_TEST       = 0x08, // T
    M68K_CCA_UNDEFINED  = 0x10, // U
} M68K_CONDITION_CODE_FLAG_ACTION, *PM68K_CONDITION_CODE_FLAG_ACTION;

typedef M68K_BYTE M68K_CONDITION_CODE_FLAG_ACTION_VALUE, *PM68K_CONDITION_CODE_FLAG_ACTION_VALUE;

typedef struct M68K_CONDITION_CODE_FLAG_ACTIONS
{
    M68K_CONDITION_CODE_FLAG_ACTION_VALUE Actions[M68K_CCF__SIZEOF__];
} M68K_CONDITION_CODE_FLAG_ACTIONS, *PM68K_CONDITION_CODE_FLAG_ACTIONS;

typedef M68K_CONDITION_CODE_FLAG_ACTIONS M68KC_CONDITION_CODE_FLAG_ACTIONS, *PM68KC_CONDITION_CODE_FLAG_ACTIONS;

typedef enum M68K_FPU_CONDITION_CODE
{
    M68K_FPCC_F     = 0,
    M68K_FPCC_EQ    = 1,
    M68K_FPCC_OGT   = 2,
    M68K_FPCC_OGE   = 3,
    M68K_FPCC_OLT   = 4,
    M68K_FPCC_OLE   = 5,
    M68K_FPCC_OLG   = 6,
    M68K_FPCC_OR    = 7,
    M68K_FPCC_UN    = 8,
    M68K_FPCC_UEQ   = 9,
    M68K_FPCC_UGT   = 10,
    M68K_FPCC_UGE   = 11,
    M68K_FPCC_ULT   = 12,
    M68K_FPCC_ULE   = 13,
    M68K_FPCC_NE    = 14,
    M68K_FPCC_T     = 15,
    M68K_FPCC_SF    = 16,
    M68K_FPCC_SEQ   = 17,
    M68K_FPCC_GT    = 18,
    M68K_FPCC_GE    = 19,
    M68K_FPCC_LT    = 20,
    M68K_FPCC_LE    = 21,
    M68K_FPCC_GL    = 22,
    M68K_FPCC_GLE   = 23,
    M68K_FPCC_NGLE  = 24,
    M68K_FPCC_NGL   = 25,
    M68K_FPCC_NLE   = 26,
    M68K_FPCC_NLT   = 27,
    M68K_FPCC_NGE   = 28,
    M68K_FPCC_NGT   = 29,
    M68K_FPCC_SNE   = 30,
    M68K_FPCC_ST    = 31,
    M68K_FPCC__SIZEOF__,
} M68K_FPU_CONDITION_CODE, *PM68K_FPU_CONDITION_CODE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_FPU_CONDITION_CODE_VALUE, *PM68K_FPU_CONDITION_CODE_VALUE;
#else
typedef M68K_FPU_CONDITION_CODE M68K_FPU_CONDITION_CODE_VALUE, *PM68K_FPU_CONDITION_CODE_VALUE;
#endif

typedef enum M68K_MMU_CONDITION_CODE
{
    M68K_MMUCC_BS   = 0,
    M68K_MMUCC_BC   = 1,
    M68K_MMUCC_LS   = 2,
    M68K_MMUCC_LC   = 3,
    M68K_MMUCC_SS   = 4,
    M68K_MMUCC_SC   = 5,
    M68K_MMUCC_AS   = 6,
    M68K_MMUCC_AC   = 7,
    M68K_MMUCC_WS   = 8,
    M68K_MMUCC_WC   = 9,
    M68K_MMUCC_IS   = 10,
    M68K_MMUCC_IC   = 11,
    M68K_MMUCC_GS   = 12,
    M68K_MMUCC_GC   = 13,
    M68K_MMUCC_CS   = 14,
    M68K_MMUCC_CC   = 15,
    M68K_MMUCC__SIZEOF__,
} M68K_MMU_CONDITION_CODE, *PM68K_MMU_CONDITION_CODE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_MMU_CONDITION_CODE_VALUE, *PM68K_MMU_CONDITION_CODE_VALUE;
#else
typedef M68K_MMU_CONDITION_CODE M68K_MMU_CONDITION_CODE_VALUE, *PM68K_MMU_CONDITION_CODE_VALUE;
#endif

typedef enum M68K_IEEE_VALUE_TYPE
{
    M68K_IEEE_VT_IND,       // extended
    M68K_IEEE_VT_INF,       // single, double, extended
    M68K_IEEE_VT_PINF,      // extended
    M68K_IEEE_VT_PNAN,      // extended
    M68K_IEEE_VT_QNAN,      // extended
    M68K_IEEE_VT_NAN,       // single, double, extended
    M68K_IEEE_VT_ZERO,      // single, double, extended
    M68K_IEEE_VT__SIZEOF__,
} M68K_IEEE_VALUE_TYPE, *PM68K_IEEE_VALUE_TYPE;

typedef enum M68K_INCREMENT
{
    M68K_INC_PRE_DECREMENT  = -1,   // -(...)
    M68K_INC_NONE           = 0,    // (...)
    M68K_INC_POST_INCREMENT = 1,    // (...)+
} M68K_INCREMENT, *PM68K_INCREMENT;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_SBYTE M68K_INCREMENT_VALUE, *PM68K_INCREMENT_VALUE;
#else
typedef M68K_INCREMENT M68K_INCREMENT_VALUE, *PM68K_INCREMENT_VALUE;
#endif

typedef enum M68K_INSTRUCTION_TYPE
{
    // note: must be sorted in ascending order according to the mnemonic
    M68K_IT_INVALID = 0,
    M68K_IT_ABCD,
    M68K_IT_ADD,
    M68K_IT_ADDA,
    M68K_IT_ADDI,
    M68K_IT_ADDIW,
    M68K_IT_ADDQ,
    M68K_IT_ADDX,
    M68K_IT_AND,
    M68K_IT_ANDI,
    M68K_IT_ASL,
    M68K_IT_ASR,
    M68K_IT_BANK,
    M68K_IT_BCC,        // dest = pc + 2 (bytes) + disp
    M68K_IT_BCHG,
    M68K_IT_BCLR,
    M68K_IT_BFCHG,
    M68K_IT_BFCLR,
    M68K_IT_BFEXTS,
    M68K_IT_BFEXTU,
    M68K_IT_BFFFO,
    M68K_IT_BFINS,
    M68K_IT_BFLYB,
    M68K_IT_BFLYW,
    M68K_IT_BFSET,
    M68K_IT_BFTST,
    M68K_IT_BGND,
    M68K_IT_BKPT,
    M68K_IT_BRA,        // dest = pc + 2 (bytes) + disp
    M68K_IT_BSEL,
    M68K_IT_BSET,
    M68K_IT_BSR,        // dest = pc + 2 (bytes) + disp
    M68K_IT_BTST,
    M68K_IT_C2P,
    M68K_IT_CALLM,
    M68K_IT_CAS,
    M68K_IT_CAS2,
    M68K_IT_CHK,
    M68K_IT_CHK2,
    M68K_IT_CINVA,
    M68K_IT_CINVL,
    M68K_IT_CINVP,
    M68K_IT_CLR,
    M68K_IT_CMP,
    M68K_IT_CMP2,
    M68K_IT_CMPA,
    M68K_IT_CMPI,
    M68K_IT_CMPIW,
    M68K_IT_CMPM,
    M68K_IT_CPBCC,      // dest = pc + 2 (bytes) + disp
    M68K_IT_CPDBCC,     // dest = pc + 4 (bytes) + disp
    M68K_IT_CPGEN,
    M68K_IT_CPRESTORE,
    M68K_IT_CPSAVE,
    M68K_IT_CPSCC,
    M68K_IT_CPTRAPCC,
    M68K_IT_CPUSHA,
    M68K_IT_CPUSHL,
    M68K_IT_CPUSHP,
    M68K_IT_DBCC,       // dest = pc + 2 (bytes) + disp
    M68K_IT_DIVS,
    M68K_IT_DIVSL,
    M68K_IT_DIVU,
    M68K_IT_DIVUL,
    M68K_IT_EOR,
    M68K_IT_EORI,
    M68K_IT_EXG,
    M68K_IT_EXT,
    M68K_IT_EXTB,
    M68K_IT_FABS,
    M68K_IT_FACOS,
    M68K_IT_FADD,
    M68K_IT_FASIN,
    M68K_IT_FATAN,
    M68K_IT_FATANH,
    M68K_IT_FBCC,       // dest = pc + 2 (bytes) + disp
    M68K_IT_FCMP,
    M68K_IT_FCOS,
    M68K_IT_FCOSH,
    M68K_IT_FDABS,
    M68K_IT_FDADD,
    M68K_IT_FDBCC,      // dest = pc + 4 (bytes) + disp
    M68K_IT_FDDIV,
    M68K_IT_FDIV,
    M68K_IT_FDMOVE,
    M68K_IT_FDMUL,
    M68K_IT_FDNEG,
    M68K_IT_FDSQRT,
    M68K_IT_FDSUB,
    M68K_IT_FETOX,
    M68K_IT_FETOXM1,
    M68K_IT_FGETEXP,
    M68K_IT_FGETMAN,
    M68K_IT_FINT,
    M68K_IT_FINTRZ,
    M68K_IT_FLOG10,
    M68K_IT_FLOG2,
    M68K_IT_FLOGN,
    M68K_IT_FLOGNP1,
    M68K_IT_FMOD,
    M68K_IT_FMOVE,
    M68K_IT_FMOVECR,
    M68K_IT_FMOVEM,
    M68K_IT_FMUL,
    M68K_IT_FNEG,
    M68K_IT_FNOP,
    M68K_IT_FREM,
    M68K_IT_FRESTORE,
    M68K_IT_FSABS,
    M68K_IT_FSADD,
    M68K_IT_FSAVE,
    M68K_IT_FSCALE,
    M68K_IT_FSCC,
    M68K_IT_FSDIV,
    M68K_IT_FSGLDIV,
    M68K_IT_FSGLMUL,
    M68K_IT_FSIN,
    M68K_IT_FSINCOS,
    M68K_IT_FSINH,
    M68K_IT_FSMOVE,
    M68K_IT_FSMUL,
    M68K_IT_FSNEG,
    M68K_IT_FSQRT,
    M68K_IT_FSSQRT,
    M68K_IT_FSSUB,
    M68K_IT_FSUB,
    M68K_IT_FTAN,
    M68K_IT_FTANH,
    M68K_IT_FTENTOX,
    M68K_IT_FTRAPCC,
    M68K_IT_FTST,
    M68K_IT_FTWOTOX,
    M68K_IT_ILLEGAL,
    M68K_IT_JMP,
    M68K_IT_JSR,
    M68K_IT_LEA,
    M68K_IT_LINK,
    M68K_IT_LOAD,
    M68K_IT_LOADI,
    M68K_IT_LPSTOP,
    M68K_IT_LSL,
    M68K_IT_LSLQ,
    M68K_IT_LSR,
    M68K_IT_LSRQ,
    M68K_IT_MINTERM,
    M68K_IT_MOVE,
    M68K_IT_MOVE16,
    M68K_IT_MOVEA,
    M68K_IT_MOVEC,
    M68K_IT_MOVEM,
    M68K_IT_MOVEP,
    M68K_IT_MOVEQ,
    M68K_IT_MOVES,
    M68K_IT_MOVEX,
    M68K_IT_MULS,
    M68K_IT_MULU,
    M68K_IT_NBCD,
    M68K_IT_NEG,
    M68K_IT_NEGX,
    M68K_IT_NOP,
    M68K_IT_NOT,
    M68K_IT_OR,
    M68K_IT_ORI,
    M68K_IT_PABSB,
    M68K_IT_PABSW,
    M68K_IT_PACK,
    M68K_IT_PACK3216,
    M68K_IT_PACKUSWB,
    M68K_IT_PADDB,
    M68K_IT_PADDUSB,
    M68K_IT_PADDUSW,
    M68K_IT_PADDW,
    M68K_IT_PAND,
    M68K_IT_PANDN,
    M68K_IT_PAVG,
    M68K_IT_PBCC,       // dest = pc + 2 (bytes) + disp
    M68K_IT_PCMPEQB,
    M68K_IT_PCMPEQW,
    M68K_IT_PCMPGEB,
    M68K_IT_PCMPGEW,
    M68K_IT_PCMPGTB,
    M68K_IT_PCMPGTW,
    M68K_IT_PCMPHIB,
    M68K_IT_PCMPHIW,
    M68K_IT_PDBCC,      // dest = pc + 4 (bytes) + disp
    M68K_IT_PEA,
    M68K_IT_PEOR,
    M68K_IT_PERM,
    M68K_IT_PFLUSH,
    M68K_IT_PFLUSHA,
    M68K_IT_PFLUSHAN,
    M68K_IT_PFLUSHN,
    M68K_IT_PFLUSHR,
    M68K_IT_PFLUSHS,
    M68K_IT_PLOADR,
    M68K_IT_PLOADW,
    M68K_IT_PLPAR,
    M68K_IT_PLPAW,
    M68K_IT_PMAXSB,
    M68K_IT_PMAXSW,
    M68K_IT_PMAXUB,
    M68K_IT_PMAXUW,
    M68K_IT_PMINSB,
    M68K_IT_PMINSW,
    M68K_IT_PMINUB,
    M68K_IT_PMINUW,
    M68K_IT_PMOVE,
    M68K_IT_PMOVEFD,
    M68K_IT_PMUL88,
    M68K_IT_PMULA,
    M68K_IT_PMULH,
    M68K_IT_PMULL,
    M68K_IT_POR,
    M68K_IT_PRESTORE,
    M68K_IT_PSAVE,
    M68K_IT_PSCC,
    M68K_IT_PSUBB,
    M68K_IT_PSUBUSB,
    M68K_IT_PSUBUSW,
    M68K_IT_PSUBW,
    M68K_IT_PTESTR,
    M68K_IT_PTESTW,
    M68K_IT_PTRAPCC,
    M68K_IT_PVALID,
    M68K_IT_RESET,
    M68K_IT_ROL,
    M68K_IT_ROR,
    M68K_IT_ROXL,
    M68K_IT_ROXR,
    M68K_IT_RPC,
    M68K_IT_RTD,
    M68K_IT_RTE,
    M68K_IT_RTM,
    M68K_IT_RTR,
    M68K_IT_RTS,
    M68K_IT_SCC,
    M68K_IT_STOP,
    M68K_IT_STORE,
    M68K_IT_STOREC,
    M68K_IT_STOREI,
    M68K_IT_STOREILM,
    M68K_IT_STOREM,
    M68K_IT_STOREM3,
    M68K_IT_SUB,
    M68K_IT_SUBA,
    M68K_IT_SUBI,
    M68K_IT_SUBQ,
    M68K_IT_SUBX,
    M68K_IT_SWAP,
    M68K_IT_TAS,
    M68K_IT_TBLS,
    M68K_IT_TBLSN,
    M68K_IT_TBLU,
    M68K_IT_TBLUN,
    M68K_IT_TOUCH,
    M68K_IT_TRANSHI,
    M68K_IT_TRANSILO,
    M68K_IT_TRANSLO,
    M68K_IT_TRAP,
    M68K_IT_TRAPCC,
    M68K_IT_TRAPV,
    M68K_IT_TST,
    M68K_IT_UNLK,
    M68K_IT_UNPACK1632,
    M68K_IT_UNPK,
    M68K_IT_VPERM,
    M68K_IT__SIZEOF__,
    M68K_IT__XL_ALIGN__,    // used by XLangLib
    M68K_IT__XL_DC__,       // used by XLangLib
} M68K_INSTRUCTION_TYPE, *PM68K_INSTRUCTION_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_WORD M68K_INSTRUCTION_TYPE_VALUE, *PM68K_INSTRUCTION_TYPE_VALUE;
#else
typedef M68K_INSTRUCTION_TYPE M68K_INSTRUCTION_TYPE_VALUE, *PM68K_INSTRUCTION_TYPE_VALUE;
#endif

typedef M68K_WORD M68K_INTERNAL_INSTRUCTION_TYPE, *PM68K_INTERNAL_INSTRUCTION_TYPE;

typedef enum M68K_SIZE
{
    M68K_SIZE_NONE = 0, // no size
    M68K_SIZE_B,        // Byte (8-bits)
    M68K_SIZE_D,        // Double (64-bits)
    M68K_SIZE_L,        // Long (32-bits)
    M68K_SIZE_P,        // Packed-Decimal (96-bits)
    M68K_SIZE_Q,        // Quad (64-bits)
    M68K_SIZE_S,        // Single (32-bits)
    M68K_SIZE_W,        // Word (16-bits)
    M68K_SIZE_X,        // eXtended (96-bits)
    M68K_SIZE__SIZEOF__,
} M68K_SIZE, *PM68K_SIZE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_SIZE_VALUE, *PM68K_SIZE_VALUE;
#else
typedef M68K_SIZE M68K_SIZE_VALUE, *PM68K_SIZE_VALUE;
#endif

typedef M68K_SIZE_VALUE M68K_CONST M68KC_SIZE_VALUE, *PM68KC_SIZE_VALUE;

typedef struct M68K_MEMORY_DISPLACEMENT
{
    M68K_SIZE_VALUE BaseSize;   // size of base/inner displacement; must reflect the effective size even if the base register is M68K_RT__RPC
    M68K_SIZE_VALUE OuterSize;  // size of outer displacement
    M68K_SDWORD     BaseValue;  // 32-bit value (with sign-extension) for the base/inner displacement; can also be the destination address when the base register is M68K_RT__RPC
    M68K_SDWORD     OuterValue; // 32-bit value (with sign-extension) for the outer displacement
} M68K_MEMORY_DISPLACEMENT, *PM68K_MEMORY_DISPLACEMENT;

typedef enum M68K_OPERAND_TYPE
{
    M68K_OT_NONE = 0,
    M68K_OT_ADDRESS_B,                      // address using a byte displacement (Long/Displacement)
    M68K_OT_ADDRESS_L,                      // address using a long displacement (Long/Displacement)
    M68K_OT_ADDRESS_W,                      // address using a word displacement (Long/Displacement)
    M68K_OT_CACHE_TYPE,                     // cache type (CacheType)
    M68K_OT_CONDITION_CODE,                 // condition code (ConditionCode)
    M68K_OT_COPROCESSOR_ID,                 // coprocessor id (CoprocessorId)
    M68K_OT_COPROCESSOR_ID_CONDITION_CODE,  // coprocessor id and condition code (CoprocessorIdCC)
    M68K_OT_DISPLACEMENT_B,                 // [+-]$hh (Displacement)
    M68K_OT_DISPLACEMENT_L,                 // [+-]$hhhhhhhh (Displacement)
    M68K_OT_DISPLACEMENT_W,                 // [+-]$hhhh (Displacement)
    M68K_OT_DYNAMIC_KFACTOR,                // Dn (Register)
    M68K_OT_FPU_CONDITION_CODE,             // fpu condition code (FpuConditionCode)
    M68K_OT_IMMEDIATE_B,                    // #$hh (Byte)
    M68K_OT_IMMEDIATE_D,                    // #$hhphh (Double)
    M68K_OT_IMMEDIATE_L,                    // #$hhhhhhhh (Long)
    M68K_OT_IMMEDIATE_P,                    // #$hhphh (PackedDecimal)
    M68K_OT_IMMEDIATE_Q,                    // #$hhhhhhhhhhhhhhhh (Quad)
    M68K_OT_IMMEDIATE_S,                    // #$hhphh (Single)
    M68K_OT_IMMEDIATE_W,                    // #$hhhh (Word)
    M68K_OT_IMMEDIATE_X,                    // #$hhphh (Extended)
    M68K_OT_MEM_ABSOLUTE_L,                 // (xxx).l (Long)
    M68K_OT_MEM_ABSOLUTE_W,                 // (xxx).w (Long)
    M68K_OT_MEM_INDIRECT,                   // (An) (Memory)
    M68K_OT_MEM_INDIRECT_DISP_W,            // (d16, An) (Memory)
    M68K_OT_MEM_INDIRECT_INDEX_DISP_B,      // (d8, An, Xn.SIZE * SCALE) (Memory)
    M68K_OT_MEM_INDIRECT_INDEX_DISP_L,      // (d32, An, Xn.SIZE * SCALE) (Memory)
    M68K_OT_MEM_INDIRECT_POST_INCREMENT,    // (An)+ (Memory)
    M68K_OT_MEM_INDIRECT_POST_INDEXED,      // ([d32i, An], Xn.SIZE * SCALE, d32o) (Memory)
    M68K_OT_MEM_INDIRECT_PRE_DECREMENT,     // -(An) (Memory)
    M68K_OT_MEM_INDIRECT_PRE_INDEXED,       // ([d32i, An, Xn.SIZE * SCALE], d32o) (Memory)
    M68K_OT_MEM_PC_INDIRECT_DISP_W,         // (d16, PC) (Memory)
    M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B,   // (d8, PC, Xn.SIZE * SCALE) (Memory)
    M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L,   // (d32, PC, Xn.SIZE * SCALE) (Memory)
    M68K_OT_MEM_PC_INDIRECT_POST_INDEXED,   // ([d32i, PC], Xn.SIZE * SCALE, d32o) (Memory)
    M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED,    // ([d32i, PC, Xn.SIZE * SCALE], d32o) (Memory)
    M68K_OT_MEM_REGISTER_PAIR,              // (Dn|An):(Dm|Am) (RegisterPair)
    M68K_OT_MMU_CONDITION_CODE,             // mmu condition code (MMUConditionCode)
    M68K_OT_OFFSET_WIDTH,                   // offset and width (OffsetWidth)
    M68K_OT_REGISTER,                       // Dn/An/FPn/... (Register)
    M68K_OT_REGISTER_FP_LIST,               // FP0[0],FP1[1],...,FP7[7] (RegisterList)
    M68K_OT_REGISTER_FPCR_LIST,             // FPCR[0],FPSR[1],FPIAR[2] (RegisterList)
    M68K_OT_REGISTER_LIST,                  // D0[0],D1[1],...,D7[7],A0[8],A1[9],...,A7[15] (RegisterList)
    M68K_OT_REGISTER_PAIR,                  // Dn:Dm (RegisterPair)
    M68K_OT_VREGISTER_PAIR_M2,              // Dn:Dm or En:Em where (n % 2) == 0 (RegisterPair)
    M68K_OT_VREGISTER_PAIR_M4,              // Dn:Dm or En:Em where (n % 4) == 0 (RegisterPair)
    M68K_OT_STATIC_KFACTOR,                 // {nnn} (SByte)
    M68K_OT__SIZEOF__,
    M68K_OT__XL_LABEL__,                    // used by XLangLib
    M68K_OT__XL_LANG_ELEMENT__,             // used by XLangLib
} M68K_OPERAND_TYPE, *PM68K_OPERAND_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_OPERAND_TYPE_VALUE, *PM68K_OPERAND_TYPE_VALUE;
typedef M68K_BYTE M68K_OPERAND_INDEX_VALUE, *PM68K_OPERAND_INDEX_VALUE;
#else
typedef M68K_OPERAND_TYPE M68K_OPERAND_TYPE_VALUE, *PM68K_OPERAND_TYPE_VALUE;
typedef M68K_UINT M68K_OPERAND_INDEX_VALUE, *PM68K_OPERAND_INDEX_VALUE;
#endif

typedef M68K_WORD M68K_REGISTER_LIST, *PM68K_REGISTER_LIST;

typedef enum M68K_REGISTER_TYPE
{
    M68K_RT_NONE = 0,
    M68K_RT_A0,         // bank 0 address register 0
    M68K_RT_A1,         // bank 0 address register 1
    M68K_RT_A2,         // bank 0 address register 2
    M68K_RT_A3,         // bank 0 address register 3
    M68K_RT_A4,         // bank 0 address register 4
    M68K_RT_A5,         // bank 0 address register 5
    M68K_RT_A6,         // bank 0 address register 6
    M68K_RT_A7,         // bank 0 address register 7
    M68K_RT_AC,         // access control register
    M68K_RT_AC0,        // access control register 0
    M68K_RT_AC1,        // access control register 1
    M68K_RT_ACUSR,      // access control unit status register
    M68K_RT_B0,         // bank 1 address register 0
    M68K_RT_B1,         // bank 1 address register 1
    M68K_RT_B2,         // bank 1 address register 2
    M68K_RT_B3,         // bank 1 address register 3
    M68K_RT_B4,         // bank 1 address register 4
    M68K_RT_B5,         // bank 1 address register 5
    M68K_RT_B6,         // bank 1 address register 6
    M68K_RT_B7,         // bank 1 address register 7
    M68K_RT_BAC0,       // breakpoint acknowledge control register 0
    M68K_RT_BAC1,       // breakpoint acknowledge control register 1
    M68K_RT_BAC2,       // breakpoint acknowledge control register 2
    M68K_RT_BAC3,       // breakpoint acknowledge control register 3
    M68K_RT_BAC4,       // breakpoint acknowledge control register 4
    M68K_RT_BAC5,       // breakpoint acknowledge control register 5
    M68K_RT_BAC6,       // breakpoint acknowledge control register 6
    M68K_RT_BAC7,       // breakpoint acknowledge control register 7
    M68K_RT_BAD0,       // breakpoint acknowledge data register 0
    M68K_RT_BAD1,       // breakpoint acknowledge data register 1
    M68K_RT_BAD2,       // breakpoint acknowledge data register 2
    M68K_RT_BAD3,       // breakpoint acknowledge data register 3
    M68K_RT_BAD4,       // breakpoint acknowledge data register 4
    M68K_RT_BAD5,       // breakpoint acknowledge data register 5
    M68K_RT_BAD6,       // breakpoint acknowledge data register 6
    M68K_RT_BAD7,       // breakpoint acknowledge data register 7
    M68K_RT_BPC,        // apollo core 68080: branches predicted correct
    M68K_RT_BPW,        // apollo core 68080: branches predicted wrong
    M68K_RT_BUSCR,      // bus control register (check M68060 User’s Manual D-22)
    M68K_RT_CAAR,       // cache address register
    M68K_RT_CACR,       // cache control register
    M68K_RT_CAL,        // current access level
    M68K_RT_CCC,        // apollo core 68080: clock cycle counter
    M68K_RT_CCR,        // condition code register
    M68K_RT_CMW,        // apollo core 68080: counter memory writes
    M68K_RT_CRP,        // cpu root pointer
    M68K_RT_D0,         // bank 0 data register 0
    M68K_RT_D1,         // bank 0 data register 1
    M68K_RT_D2,         // bank 0 data register 2
    M68K_RT_D3,         // bank 0 data register 3
    M68K_RT_D4,         // bank 0 data register 4
    M68K_RT_D5,         // bank 0 data register 5
    M68K_RT_D6,         // bank 0 data register 6
    M68K_RT_D7,         // bank 0 data register 7
    M68K_RT_DACR0,      // data access control register 0
    M68K_RT_DACR1,      // data access control register 1
    M68K_RT_DCH,        // apollo core 68080: data cache hits
    M68K_RT_DCM,        // apollo core 68080: data cache misses
    M68K_RT_DFC,        // destination function code register
    M68K_RT_DRP,        // dma root pointer register
    M68K_RT_DTT0,       // data transparent translation register 0
    M68K_RT_DTT1,       // data transparent translation register 1
    M68K_RT_E0,         // bank 1 data register 0
    M68K_RT_E1,         // bank 1 data register 1
    M68K_RT_E2,         // bank 1 data register 2
    M68K_RT_E3,         // bank 1 data register 3
    M68K_RT_E4,         // bank 1 data register 4
    M68K_RT_E5,         // bank 1 data register 5
    M68K_RT_E6,         // bank 1 data register 6
    M68K_RT_E7,         // bank 1 data register 7
    M68K_RT_E8,         // bank 2 data register 8
    M68K_RT_E9,         // bank 2 data register 9
    M68K_RT_E10,        // bank 2 data register 10
    M68K_RT_E11,        // bank 2 data register 11
    M68K_RT_E12,        // bank 2 data register 12
    M68K_RT_E13,        // bank 2 data register 13
    M68K_RT_E14,        // bank 2 data register 14
    M68K_RT_E15,        // bank 2 data register 15
    M68K_RT_E16,        // bank 3 data register 16
    M68K_RT_E17,        // bank 3 data register 17
    M68K_RT_E18,        // bank 3 data register 18
    M68K_RT_E19,        // bank 3 data register 19
    M68K_RT_E20,        // bank 3 data register 20
    M68K_RT_E21,        // bank 3 data register 21
    M68K_RT_E22,        // bank 3 data register 22
    M68K_RT_E23,        // bank 3 data register 23
    M68K_RT_FP0,        // floating point register 0
    M68K_RT_FP1,        // floating point register 1
    M68K_RT_FP2,        // floating point register 2
    M68K_RT_FP3,        // floating point register 3
    M68K_RT_FP4,        // floating point register 4
    M68K_RT_FP5,        // floating point register 5
    M68K_RT_FP6,        // floating point register 6
    M68K_RT_FP7,        // floating point register 7
    M68K_RT_FPCR,       // floating point control register
    M68K_RT_FPIAR,      // floating point instruction addess register
    M68K_RT_FPSR,       // floating point status register
    M68K_RT_IACR0,      // instruction access control register 0
    M68K_RT_IACR1,      // instruction access control register 1
    M68K_RT_IEP1,       // apollo core 68080: instructions executed pipe 1
    M68K_RT_IEP2,       // apollo core 68080: instructions executed pipe 2
    M68K_RT_ISP,        // interrupt stack pointer register
    M68K_RT_ITT0,       // instruction transparent translation register 0
    M68K_RT_ITT1,       // instruction transparent translation register 1
    M68K_RT_MMUSR,      // mmu status register
    M68K_RT_MSP,        // master stack pointer register
    M68K_RT_PC,         // program counter register
    M68K_RT_PCR,        // processor configuration register (check M68060 User’s Manual D-22)
    M68K_RT_PCSR,       // pmmu cache status register (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-56)
    M68K_RT_PSR,        // pmmu status register (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-49)
    M68K_RT_SCC,        // stack change control register
    M68K_RT_SFC,        // source function code register
    M68K_RT_SR,         // status register
    M68K_RT_SRP,        // supervisor root pointer
    M68K_RT_STB,        // apollo core 68080: stall counter, caused by write buffer full
    M68K_RT_STC,        // apollo core 68080: stall counter, caused by dcache misses
    M68K_RT_STH,        // apollo core 68080: stall counter, caused by hazards
    M68K_RT_STR,        // apollo core 68080: stall counter, caused by register dependencies
    M68K_RT_TC,         // mmu translation control register
    M68K_RT_TT0,        // transparent translation register 0
    M68K_RT_TT1,        // transparent translation register 1
    M68K_RT_URP,        // user root pointer
    M68K_RT_USP,        // user stack pointer register
    M68K_RT_VAL,        // valid access level register
    M68K_RT_VBR,        // vector base register
    M68K_RT_ZPC,        // zero program counter register
    M68K_RT__SIZEOF__,
    M68K_RT__RPC,       // relative program counter (used to specify displacements using addresses; see M68K_DFLG_ALLOW_RPC_REGISTER)
} M68K_REGISTER_TYPE, *PM68K_REGISTER_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_REGISTER_TYPE_VALUE, *PM68K_REGISTER_TYPE_VALUE;
#else
typedef M68K_REGISTER_TYPE M68K_REGISTER_TYPE_VALUE, *PM68K_REGISTER_TYPE_VALUE;
#endif

typedef M68K_REGISTER_TYPE_VALUE M68K_CONST M68KC_REGISTER_TYPE_VALUE, *PM68KC_REGISTER_TYPE_VALUE;

typedef struct M68K_KFACTOR
{
    M68K_REGISTER_TYPE_VALUE    Register;   // data register (for dynamic k-factor) or M68K_RT_NONE (for static k-factor)
    M68K_SBYTE                  Value;      // value between -64 and 63 (check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 5-79; only used when Register == M68K_RT_NONE)
} M68K_KFACTOR, *PM68K_KFACTOR;

typedef struct M68K_OFFSET_WIDTH
{
    M68K_INT    Offset; // 0: error; >0: data register; <0: -value-1, value in 0..31
    M68K_INT    Width;  // 0: error; >0: data register; <0: -value-1, value in 1..32
} M68K_OFFSET_WIDTH, *PM68K_OFFSET_WIDTH;

typedef struct M68K_REGISTER_PAIR
{
    M68K_REGISTER_TYPE_VALUE    Register1;  // register 1
    M68K_REGISTER_TYPE_VALUE    Register2;  // register 2
} M68K_REGISTER_PAIR, *PM68K_REGISTER_PAIR;

typedef enum M68K_SCALE
{
    M68K_SCALE_1    = 0, // * 1
    M68K_SCALE_2    = 1, // * 2
    M68K_SCALE_4    = 2, // * 4
    M68K_SCALE_8    = 3, // * 8
    M68K_SCALE__SIZEOF__,
} M68K_SCALE, *PM68K_SCALE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_SCALE_VALUE, *PM68K_SCALE_VALUE;
#else
typedef M68K_SCALE M68K_SCALE_VALUE, *PM68K_SCALE_VALUE;
#endif

typedef struct M68K_REGISTER_SIZE
{
    M68K_REGISTER_TYPE_VALUE    Register;   // type of register
    M68K_SIZE_VALUE             Size;       // size of the register (only when applicable)
} M68K_REGISTER_SIZE, *PM68K_REGISTER_SIZE;

typedef struct M68K_MEMORY
{
    M68K_INCREMENT              Increment;      // memory increment (pre or post)
    M68K_REGISTER_TYPE_VALUE    Base;           // base register (address/program counter; can be M68K_RT_NONE)
    M68K_REGISTER_SIZE          Index;          // index register (data/address; can be M68K_RT_NONE)
    M68K_SCALE_VALUE            Scale;          // scale applied to index data register (ignored if Index.Register == M68K_RT_NONE)
    M68K_MEMORY_DISPLACEMENT    Displacement;   // displacement (inner and regular/outer)
} M68K_MEMORY, *PM68K_MEMORY;

typedef M68K_DWORD M68K_EXTENDED[3];            // [0] 95..64, [1] 63..32, [2] 31..0
typedef M68K_DWORD M68K_PACKED_DECIMAL[3];      // [0] 95..64, [1] 63..32, [2] 31..0
typedef M68K_DWORD M68K_QUAD[2];                // [0] 63..32, [1] 31..0

typedef M68K_EXTENDED *PM68K_EXTENDED;
typedef M68K_PACKED_DECIMAL *PM68K_PACKED_DECIMAL;
typedef M68K_QUAD *PM68K_QUAD;

typedef union M68K_OPERAND_INFO
{
    M68K_BYTE                       Byte;
    M68K_CACHE_TYPE_VALUE           CacheType;
    M68K_CONDITION_CODE_VALUE       ConditionCode;
    M68K_COPROCESSOR_ID_VALUE       CoprocessorId;
    M68K_SDWORD                     Displacement;
    M68K_DOUBLE                     Double;
    M68K_EXTENDED                   Extended; 
    M68K_FPU_CONDITION_CODE_VALUE   FpuConditionCode;
    PM68K_STR                       Label;
    M68K_DWORD                      Long;
    M68K_MEMORY                     Memory;
    M68K_MMU_CONDITION_CODE_VALUE   MMUConditionCode;
    M68K_OFFSET_WIDTH               OffsetWidth;
    M68K_PACKED_DECIMAL             PackedDecimal;   
    M68K_QUAD                       Quad;
    M68K_REGISTER_TYPE_VALUE        Register;
    M68K_REGISTER_LIST              RegisterList;
    M68K_REGISTER_PAIR              RegisterPair;
    M68K_SBYTE                      SByte;
    M68K_SINGLE                     Single;
    M68K_WORD                       Word;

    struct
    {
        M68K_COPROCESSOR_ID_VALUE               Id;
        M68K_COPROCESSOR_CONDITION_CODE_VALUE   CC;
    } CoprocessorIdCC;
} M68K_OPERAND_INFO, *PM68K_OPERAND_INFO;

typedef struct M68K_OPERAND
{
    M68K_OPERAND_TYPE_VALUE Type;           // type of operand
    M68K_OPERAND_INFO       Info;           // information for the operand
} M68K_OPERAND, *PM68K_OPERAND;

typedef struct M68K_INSTRUCTION
{
    PM68K_WORD                      Start;                                  // start of the instruction (pointer to first word that is part of the instruction)
    PM68K_WORD                      Stop;                                   // last word used to disassemble the instruction; note: limited by the "End" parameter in M68KDisassemble
    PM68K_WORD                      End;                                    // end of the instruction (pointer to first word that is not part of the instruction)
    M68K_INTERNAL_INSTRUCTION_TYPE  InternalType;                           // internal type for the instruction
    M68K_INSTRUCTION_TYPE_VALUE     Type;                                   // type of instruction
    M68K_SIZE_VALUE                 Size;                                   // instruction size
    M68K_BYTE                       PCOffset;                               // offset in bytes (from the start of the instruction) that is used to calculate final addresses using relative displacements
    M68K_OPERAND                    Operands[M68K_MAXIMUM_NUMBER_OPERANDS]; // information for each operand
} M68K_INSTRUCTION, *PM68K_INSTRUCTION;

// assemble
typedef enum M68K_ASM_ERROR_TYPE
{
    M68K_AET_NONE                                   = 0,
    M68K_AET_UNEXPECTED_CHAR                        = 1,
    M68K_AET_INVALID_MNEMONIC                       = 2,
    M68K_AET_INVALID_SIZE                           = 3,
    M68K_AET_TOO_MANY_OPERANDS                      = 4,
    M68K_AET_INVALID_OPERAND_TYPE                   = 5,
    M68K_AET_OPERAND_TYPE_NOT_SUPPORTED             = 6,
    M68K_AET_DECIMAL_OVERFLOW                       = 7,
    M68K_AET_HEXADECIMAL_OVERFLOW                   = 8,
    M68K_AET_VALUE_OUT_OF_RANGE                     = 9,
    M68K_AET_INVALID_CONDITION_CODE                 = 10,
    M68K_AET_INVALID_REGISTER                       = 11,
    M68K_AET_REGISTER_NOT_SUPPORTED                 = 12,
    M68K_AET_INVALID_CACHE_TYPE                     = 13,
    M68K_AET_REGISTERS_ALREADY_IN_LIST              = 14,
    M68K_AET_INVALID_RANGE_REGISTERS                = 15,
    M68K_AET_INVALID_IEEE_VALUE                     = 16,
    M68K_AET_IEEE_VALUE_NOT_SUPPORTED               = 17,
    M68K_AET_INVALID_EXPONENT                       = 18,
    M68K_AET_MEMORY_ADDRESS_ALREADY_SPECIFIED       = 19,
    M68K_AET_MEMORY_BASE_ALREADY_SPECIFIED          = 20,
    M68K_AET_MEMORY_INDEX_ALREADY_SPECIFIED         = 21,
    M68K_AET_INVALID_REGISTER_MEMORY_BASE           = 22,
    M68K_AET_INVALID_REGISTER_MEMORY_INDEX          = 23,
    M68K_AET_MEMORY_INNER_DISP_ALREADY_SPECIFIED    = 24,
    M68K_AET_MEMORY_OUTER_DISP_ALREADY_SPECIFIED    = 25,
    M68K_AET_INVALID_MEMORY_OPERAND                 = 26,
    M68K_AET_INVALID_INSTRUCTION                    = 27,
    M68K_AET__SIZEOF__,
} M68K_ASM_ERROR_TYPE, *PM68K_ASM_ERROR_TYPE;

typedef struct M68K_ASM_ERROR
{
    PM68KC_STR          Start;      // start of the text that is being analysed
    PM68KC_STR          Location;   // location in the text where the error occured
    M68K_ASM_ERROR_TYPE Type;       // type of error
} M68K_ASM_ERROR, *PM68K_ASM_ERROR;

typedef enum M68K_ASM_FLAGS
{
    M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS = 0x0001, // allow XLangLib extensions
    M68K_AFLG_ALLOW_BANK_EXTENSION_WORD = 0x0002, // allow bank extension word in apollo core 68080
} M68K_ASM_FLAGS, *PM68K_ASM_FLAGS;

typedef M68K_WORD M68K_ASM_FLAGS_VALUE, *PM68K_ASM_FLAGS_VALUE;

// disassemble
typedef enum M68K_DISASM_FLAGS
{
    M68K_DFLG_ALLOW_CPX                 = 0x0001, // allow cpX instructions
    M68K_DFLG_ALLOW_FMOVE_KFACTOR       = 0x0002, // allow fmove instructions with k-factor
    M68K_DFLG_ALLOW_RPC_REGISTER        = 0x0004, // allow the use of the special "rpc" register (for pc relative memory addresses i.e. [pc + disp])
    M68K_DFLG_ALLOW_ADDRESS_OPERAND     = 0x0008, // allow the use of the special "address" operands (for pc relative displacements i.e. bcc, fbcc, etc.)
    M68K_DFLG_ALLOW_BANK_EXTENSION_WORD = 0x0010, // allow bank extension word in apollo core 68080
} M68K_DISASM_FLAGS, *PM68K_DISASM_FLAGS;

typedef M68K_WORD M68K_DISASM_FLAGS_VALUE, *PM68K_DISASM_FLAGS_VALUE;

typedef enum M68K_DISASM_TEXT_FLAGS
{
    M68K_DTFLG_ALL_LOWERCASE                = 0x0001,   // force all texts in lowercase (information in the format is ignored; supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_ALL_UPPERCASE                = 0x0002,   // force all texts in uppercase (information in the format is ignored; ignored when M68K_DTFLG_ALL_LOWERCASE is specified; supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_IGNORE_SIGN                  = 0x0004,   // ignore the sign bit in immediates/displacements (supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_TRAILING_ZEROS               = 0x0008,   // include the trailing zeros in hexadecimal numbers (supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_C_HEXADECIMALS               = 0x0010,   // generate hexadecimal numbers with the "0xNNN" notation; the alternative/default notation is "$NNN" (forced by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_SCALE_1                      = 0x0020,   // write X64_SCALE_1 as "*1" (forced by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_HIDE_IMPLICIT_SIZES          = 0x0040,   // hide the size of the instructions that have an implicit size (supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_CONDITION_CODE_AS_OPERAND    = 0x0080,   // write the condition code as an operand (forced by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_HIDE_IMMEDIATE_PREFIX        = 0x0100,   // hide the immediate prefix (forced by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG_DISPLACEMENT_AS_VALUE        = 0x0200,   // displacements are converted as values (supported by M68KDisassembleTextForAssembler/XL)
    M68K_DTFLG__DEFAULT__ = 
        M68K_DTFLG_C_HEXADECIMALS | 
        M68K_DTFLG_HIDE_IMPLICIT_SIZES,// | 
        //M68K_DTFLG_HIDE_IMMEDIATE_PREFIX,
} M68K_DISASM_TEXT_FLAGS, *PM68K_DISASM_TEXT_FLAGS;

typedef M68K_WORD M68K_DISASM_TEXT_FLAGS_VALUE, *PM68K_DISASM_TEXT_FLAGS_VALUE;

typedef enum M68K_DISASM_TEXT_INFO_TYPE
{
    M68K_DTIF_UNKNOWN = 0,                  // unknown specifier (Char)
    M68K_DTIF_ADDRESS_L,                    // 32-bit memory address (Long; default = "$nnnnnnnn")
    M68K_DTIF_ADDRESS_W,                    // 16-bit memory address (Word; default = "$nnnn")
    M68K_DTIF_CONDITION_CODE,               // condition code (ConditionCode; default = "xx")
    M68K_DTIF_COPROCESSOR_CONDITION_CODE,   // coprocessor condition code (CoprocessorCC; default = "#$nn")
    M68K_DTIF_COPROCESSOR_ID,               // coprocessor id (CoprocessorId; default = "#n")
    M68K_DTIF_DISPLACEMENT_B,               // 8-bit displacement value (Long/Displacement; must include the + or -; default = "[+-]$nn")
    M68K_DTIF_DISPLACEMENT_L,               // 32-bit displacement value (Long/Displacement; must include the + or -; default = "[+-]$nnnnnnnn")
    M68K_DTIF_DISPLACEMENT_W,               // 16-bit displacement value (Long/Displacement; must include the + or -; default = "[+-]$nnnn")
    M68K_DTIF_FPU_CONDITION_CODE,           // fpu condition code (FpuConditionCode; default = "xx")
    M68K_DTIF_IMMEDIATE_B,                  // 8-bit integer value (Byte; default = "#[+-]?$nn")
    M68K_DTIF_IMMEDIATE_D,                  // 64-bit double value (Double; default = "#[+-]$nnnpmmm")
    M68K_DTIF_IMMEDIATE_L,                  // 32-bit integer value (Long; default = "#[+-]?$nnnnnnnn")
    M68K_DTIF_IMMEDIATE_P,                  // 96-bit packed-decimal value (PackedDecimal; default = "#$nnn")
    M68K_DTIF_IMMEDIATE_Q,                  // 64-bit integer value (Quad; default = "#$nnnnnnnnnnnnnnnn")
    M68K_DTIF_IMMEDIATE_S,                  // 32-bit single value (Single; default = "#[+-]$nnnpmmm")
    M68K_DTIF_IMMEDIATE_W,                  // 16-bit integer value (Word; default = "#[+-]?$nnnn")
    M68K_DTIF_IMMEDIATE_X,                  // 96-bit extended value (Extended; default = "#[+-]?$nnnnnnnn")
    M68K_DTIF_INSTRUCTION_MNEMONIC,         // instruction mnemonic (Instruction.*; default = "xxx")
    M68K_DTIF_MMU_CONDITION_CODE,           // mmu condition code (MMUConditionCode; default = "xx")
    M68K_DTIF_OFFSET_WIDTH_VALUE,           // value used to specify an offset or width (Byte; value between 0-32; nnn)
    M68K_DTIF_OPCODE_WORD,                  // word used in the opcode (Immediate16; default = nnnn)
    M68K_DTIF_SCALE,                        // text for the memory scale (Scale; must include the *; default = "*n")
    M68K_DTIF_SIZE,                         // instruction size (Size; default = ".s")
    M68K_DTIF_STATIC_KFACTOR,               // static k-factor (SByte; default = "#[+-]$nn")
    M68K_DTIF_TEXT_COPROCESSOR_END,         // char or text used to end coprocessor information (default = ">")
    M68K_DTIF_TEXT_COPROCESSOR_SEPARATOR,   // char or text used as a separator for the coprocessor id and condition code (default = ":")
    M68K_DTIF_TEXT_COPROCESSOR_START,       // char or text used to start coprocessor information (default = "<")
    M68K_DTIF_TEXT_IMMEDIATE_PREFIX,        // char or text used as a prefix for immediate values (default = "#")
    M68K_DTIF_TEXT_INNER_MEMORY_END,        // char or text used to end the inner part of a memory operand (default = ']')
    M68K_DTIF_TEXT_INNER_MEMORY_START,      // char or text used to start the inner part of a memory operand (default = '[')
    M68K_DTIF_TEXT_KFACTOR_END,             // char or text used to end a k-factor (default = '}')
    M68K_DTIF_TEXT_KFACTOR_START,           // char or text used to start a k-factor (default = '{')
    M68K_DTIF_TEXT_MEMORY_ADD,              // char or text used to add value in a memory operand (default = '+')
    M68K_DTIF_TEXT_MEMORY_END,              // char or text used to end a memory operand (default = ')')
    M68K_DTIF_TEXT_MEMORY_POST_INCREMENT,   // char or text used to (post)increment a memory operand (default = '+')
    M68K_DTIF_TEXT_MEMORY_PRE_DECREMENT,    // char or text used to (pre)decrement a memory operand (default = '-')
    M68K_DTIF_TEXT_MEMORY_START,            // char or text used to start a memory operand (default = '(')
    M68K_DTIF_TEXT_OFFSET_WIDTH_END,        // char or text used to end an offset/width (default = '}')
    M68K_DTIF_TEXT_OFFSET_WIDTH_SEPARATOR,  // char or text used as a separator in an offset/width (default = ':')
    M68K_DTIF_TEXT_OFFSET_WIDTH_START,      // char or text used to start an offset/width (default = '{')
    M68K_DTIF_TEXT_OPCODE_WORD_SEPARATOR,   // char or text used to separate opcode words (default = ' ')
    M68K_DTIF_TEXT_OPCODE_WORD_UNKNOWN,     // unknown word used in the opcode (default = "????")
    M68K_DTIF_TEXT_OPERAND_SEPARATOR,       // char or text used to separate operands (default = ", ")
    M68K_DTIF_TEXT_RELATIVE_ADDRESS_PREFIX, // char or text used as a prefix for relative addresses (default = '%')
    M68K_DTIF_TEXT_PADDING,                 // char or text used for padding (default = ' ')
    M68K_DTIF_TEXT_PAIR_SEPARATOR,          // char or text used to separate a pair (default = ':')
    M68K_DTIF_TEXT_SEPARATOR,               // char or text used to separate generic elements (default = ' ')
} M68K_DISASM_TEXT_INFO_TYPE, *PM68K_DISASM_TEXT_INFO_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_DISASM_TEXT_INFO_TYPE_VALUE, *PM68K_DISASM_TEXT_INFO_TYPE_VALUE;
#else
typedef M68K_DISASM_TEXT_INFO_TYPE M68K_DISASM_TEXT_INFO_TYPE_VALUE, *PM68K_DISASM_TEXT_INFO_TYPE_VALUE;
#endif

typedef union M68K_DISASM_TEXT_INFO_DETAILS
{
    M68K_CONDITION_CODE_VALUE               ConditionCode;
    M68K_COPROCESSOR_CONDITION_CODE_VALUE   CoprocessorCC;
    M68K_COPROCESSOR_ID_VALUE               CoprocessorId;
    M68K_BYTE                               Byte;
    M68K_CHAR                               Char;
    M68K_SDWORD                             Displacement;
    M68K_DOUBLE                             Double;
    M68K_EXTENDED                           Extended;
    M68K_FPU_CONDITION_CODE_VALUE           FpuConditionCode;
    M68K_DWORD                              Long;
    M68K_MMU_CONDITION_CODE_VALUE           MMUConditionCode;
    M68K_PACKED_DECIMAL                     PackedDecimal;
    M68K_QUAD                               Quad;
    M68K_SBYTE                              SByte;
    M68K_SINGLE                             Single;
    M68K_SCALE_VALUE                        Scale;
    M68K_SIZE_VALUE                         Size;
    M68K_WORD                               Word;
} M68K_DISASM_TEXT_INFO_DETAILS, *PM68K_DISASM_TEXT_INFO_DETAILS;

typedef struct M68K_DISASM_TEXT_INFO
{
    PM68K_INSTRUCTION                   Instruction;    // instruction being converted to text
    M68K_DISASM_TEXT_FLAGS_VALUE        TextFlags;      // flags for the text
    M68K_DISASM_TEXT_INFO_TYPE_VALUE    Type;           // type of information that needs to be converted to text
    M68K_DISASM_TEXT_INFO_DETAILS       Details;        // details for the information

    // the custom function must generate the text in the next output buffer;
    // the text must end with a \0 otherwise all chars will be used
    M68K_CHAR OutBuffer[M68K_MAXIMUM_SIZE_CUSTOM_OUTPUT_BUFFER];
} M68K_DISASM_TEXT_INFO, *PM68K_DISASM_TEXT_INFO;

typedef M68K_BOOL (*PM68K_DISASSEMBLE_TEXT_FUNCTION)(PM68K_DISASM_TEXT_INFO TextInfo, PM68K_VOID Data);

typedef enum M68K_VALUE_TYPE
{
    M68K_VT_UNKNOWN = 0,            // unknown type of value
    M68K_VT_ABSOLUTE_L,             // 32-bit absolute address
    M68K_VT_ABSOLUTE_W,             // 16-bit absolute address
    M68K_VT_BASE_DISPLACEMENT_B,    // 8-bit base displacement
    M68K_VT_BASE_DISPLACEMENT_L,    // 32-bit base displacement
    M68K_VT_BASE_DISPLACEMENT_W,    // 16-bit base displacement
    M68K_VT_DISPLACEMENT_B,         // 8-bit displacement
    M68K_VT_DISPLACEMENT_L,         // 32-bit displacement
    M68K_VT_DISPLACEMENT_W,         // 16-bit displacement
    M68K_VT_IMMEDIATE_B,            // 8-bit integer value
    M68K_VT_IMMEDIATE_D,            // 64-bit double value
    M68K_VT_IMMEDIATE_L,            // 32-bit integer value
    M68K_VT_IMMEDIATE_P,            // 96-bit packed-decimal value
    M68K_VT_IMMEDIATE_Q,            // 64-bit integer value
    M68K_VT_IMMEDIATE_S,            // 32-bit single value
    M68K_VT_IMMEDIATE_W,            // 16-bit integer value
    M68K_VT_IMMEDIATE_X,            // 96-bit extended value
    M68K_VT_OUTER_DISPLACEMENT_L,   // 32-bit outer displacement
    M68K_VT_OUTER_DISPLACEMENT_W,   // 16-bit outer displacement
    M68K_VT__SIZEOF__,
} M68K_VALUE_TYPE, *PM68K_VALUE_TYPE;

#ifdef M68K_OPTIMIZE_SPACE
typedef M68K_BYTE M68K_VALUE_TYPE_VALUE, *PM68K_VALUE_TYPE_VALUE;
#else
typedef M68K_VALUE_TYPE M68K_VALUE_TYPE_VALUE, *PM68K_VALUE_TYPE_VALUE;
#endif

typedef M68K_WORD M68K_VALUE_OPERANDS_MASK, *PM68K_VALUE_OPERANDS_MASK;

typedef struct M68K_VALUE_LOCATION
{
    PM68K_BYTE                  Location;       // pointer to the first byte that is used by the value
    M68K_VALUE_TYPE             Type;           // type of value
    M68K_OPERAND_INDEX_VALUE    OperandIndex;   // index of the operand where the value is used; 0 .. M68K_MAXIMUM_NUMBER_OPERANDS
} M68K_VALUE_LOCATION, *PM68K_VALUE_LOCATION;

extern M68K_WORD                            M68KAssemble(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_ARCHITECTURE Architectures, M68K_ASM_FLAGS Flags, PM68K_WORD OutBuffer /*M68K_NULL = return total size*/, M68K_WORD OutSize /*maximum in words*/);
extern M68K_WORD                            M68KAssembleText(PM68K_WORD Address, PM68KC_STR Text, M68K_ARCHITECTURE Architectures, M68K_ASM_FLAGS Flags, PM68K_WORD OutBuffer /*M68K_NULL = return total size*/, M68K_WORD OutSize /*maximum in words*/, PM68K_ASM_ERROR Error /*can be M68K_NULL*/);
extern M68K_BOOL                            M68KConvertOperandTo(M68K_ASM_FLAGS AsmFlags, PM68K_OPERAND Operand, M68K_OPERAND_TYPE_VALUE OperandType, PM68K_OPERAND NewOperand);
extern M68K_BOOL                            M68KDefaultDisassembleTextFunc(PM68K_DISASM_TEXT_INFO TextInfo, PM68K_VOID Data);
extern PM68K_WORD                           M68KDisassemble(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction);
extern PM68K_WORD                           M68KDisassembleEx(PM68K_WORD Address, PM68K_WORD Start, PM68K_WORD End, M68K_DISASM_FLAGS Flags, M68K_ARCHITECTURE Architectures, PM68K_INSTRUCTION Instruction, M68K_INSTRUCTION_TYPE ForcedInstructionType /*M68K_IT_INVALID = ignored*/, PM68K_VALUE_LOCATION Values /*M68K_NULL = values are ignored*/, M68K_VALUE_OPERANDS_MASK OperandsMask /*0 = values are ignored*/, M68K_UINT MaximumNumberValues /*0 = values are ignored*/, PM68K_UINT TotalNumberValues /*can be M68K_NULL*/);
extern M68K_LUINT                           M68KDisassembleText(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, PM68KC_STR Format /*M68K_NULL = use default*/, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/);
extern M68K_LUINT                           M68KDisassembleTextEx(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, PM68KC_STR Format /*M68K_NULL = use default*/, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_DISASSEMBLE_TEXT_FUNCTION DisTextFunc, PM68K_VOID DisTextFuncData, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/);
extern M68K_LUINT                           M68KDisassembleTextForAssembler(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/);
extern M68K_LUINT                           M68KDisassembleTextForXL(PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/);
extern M68K_ARCHITECTURE_VALUE              M68KGetArchitectures(M68K_INTERNAL_INSTRUCTION_TYPE InternalType);
extern PM68KC_STR                           M68KGetAsmErrorTypeText(M68K_ASM_ERROR_TYPE ErrorType);
extern PM68KC_CONDITION_CODE_FLAG_ACTIONS   M68KGetConditionCodeFlagActions(M68K_INTERNAL_INSTRUCTION_TYPE InternalType);
extern M68K_BOOL                            M68KGetIEEEValue(M68K_IEEE_VALUE_TYPE IEEEValueType, M68K_BOOL InvertSign, M68K_OPERAND_TYPE OperandType, PM68K_OPERAND_INFO OperandInfo);
extern M68K_OPERAND_TYPE                    M68KGetImplicitImmediateOperandTypeForSize(M68K_SIZE Size, PM68K_UINT SizeInBytes /*M68K_NULL = ignored*/);
extern M68K_INSTRUCTION_TYPE                M68KGetInstructionType(PM68KC_STR Text, M68K_LUINT TextSize /*0 = empty text*/);
extern PM68KC_STR                           M68KGetRegisterName(M68K_REGISTER_TYPE RegisterType);
extern M68K_REGISTER_TYPE                   M68KGetRegisterType(PM68KC_STR Text, M68K_LUINT TextSize /*0 = empty text*/);
extern M68K_SIZE                            M68KGetSizeFromChar(M68KC_CHAR Char);
extern M68K_DWORD                           M68KReadDWord(PM68K_VOID Address);
extern M68K_WORD                            M68KReadWord(PM68K_VOID Address);
extern void                                 M68KWriteDWord(PM68K_VOID Address, M68K_DWORD Value);
extern M68K_UINT                            M68KWriteImmediateOperand(PM68K_VOID Address, PM68K_OPERAND Operand);
extern void                                 M68KWriteWord(PM68K_VOID Address, M68K_WORD Value);
#ifdef M68K_INTERNAL_FUNCTIONS
extern void                                 M68KInternalGenerateAssemblerTables(PM68KC_STR FileNameIndexFirstWord, PM68KC_STR FileNameOpcodeMaps, PM68KC_STR FileNameWords);
extern M68K_UINT                            M68KInternalGenerateOpcodes(PM68K_WORD Start, M68K_UINT MaximumSize /*in words*/, M68K_DISASM_FLAGS Flags, PM68K_UINT UsedSize /*in words*/);
#endif

#endif
