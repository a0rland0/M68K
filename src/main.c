#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable:4702)
#endif

#include "main.h"
#include "m68k/m68kinternal.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_AFLG        (M68K_AFLG_ALLOW_BANK_EXTENSION_WORD)
#define DEFAULT_ARCH        M68K_ARCH__ALL__
#define DEFAULT_DFLG        (M68K_DFLG_ALLOW_FMOVE_KFACTOR | M68K_DFLG_ALLOW_RPC_REGISTER | M68K_DFLG_ALLOW_ADDRESS_OPERAND | M68K_DFLG_ALLOW_BANK_EXTENSION_WORD)
#define DEFAULT_DTFLG       M68K_DTFLG__DEFAULT__
#define DEFAULT_FILE_POS    0
#define DEFAULT_FILE_SIZE   INT_MAX
#define DEFAULT_FORMAT      "%40w %m%s%-%o"
#define DEFAULT_REFADDR     0xf80000UL
#define EXIT_INVALID_PARAMS 2
#define FMT_INVALID_OPCODE  "%-40.04x?"
#define FMT_REFADDR         "0x%08x"
#define MAX_BINARY_SIZE     (3 * 1024 * 1024)
#define MAX_NUMBER_BYTES    256
#define VERSION             "2.0 (" __DATE__ " " __TIME__ ")"

// forward declarations
static void         AsmTest(PM68K_WORD address, PM68K_INSTRUCTION instruction, M68K_BOOL testText);
static void         CantConvertValueForOption(PM68KC_STR argOption);
static COMMAND_TYPE CheckArguments(int argc, char **argv);
static M68K_DWORD   ConvertDWord(PM68KC_STR argOption, PM68KC_STR argValue, M68K_BOOL zeroOrGreater);
static M68K_WORD    ConvertWord(PM68KC_STR argOption, PM68KC_STR argValue);
static M68K_BOOL    DisassembleBinaryFile(PM68KC_STR fileName);
static M68K_UINT    DisassembleBuffer(PM68K_WORD address, PM68K_WORD start, PM68K_WORD end);
static void         Exit(int errorCode);
static void         NotEnoughParametersForCommand(PM68KC_STR argCommand);
static void         NotEnoughParametersForOption(PM68KC_STR argOption);
static void         PrintConfiguration(PM68KC_STR title);
static void         PrintUsageAndExit();
static PM68K_BYTE   ReadFromFile(PM68KC_STR fileName, M68K_INT position, M68K_INT size, PM68K_INT readSize);
static M68K_BOOL    StrIsEmptyOrNull(PM68KC_STR string);
static void         TooManyParametersForCommand(PM68KC_STR argCommand);
static M68K_BOOL    WriteToFile(PM68KC_STR fileName, PM68K_BYTE buffer, M68K_INT size);

// options
static M68K_ARCHITECTURE_VALUE      OptArch = DEFAULT_ARCH;
static M68K_ASM_FLAGS_VALUE         OptAsmFlags = DEFAULT_AFLG;
static M68K_BOOL                    OptAsmTests = M68K_FALSE;
static M68K_DISASM_FLAGS_VALUE      OptDisFlags = DEFAULT_DFLG;
static M68K_BOOL                    OptDisForAssembler = M68K_FALSE;
static M68K_DISASM_TEXT_FLAGS_VALUE OptDisTextFlags = DEFAULT_DTFLG;
static PM68KC_STR                   OptDisTextFormat = DEFAULT_FORMAT;
static M68K_INT                     OptFilePosition = DEFAULT_FILE_POS;
static M68K_INT                     OptFileSize = DEFAULT_FILE_SIZE;
static M68K_DWORD                   OptRefAddress = DEFAULT_REFADDR;
static M68K_BOOL                    OptShowConfiguration = M68K_FALSE;

// buffers
static M68K_WORD    AsmBuffer[M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS];
static M68K_WORD    AsmBufferTest[M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS];
static M68K_CHAR    DisText[1024];
static M68K_CHAR    DisTextTest[64];

// bytes for the disassembler
static M68K_UINT ParamNumberBytes = 0;
static M68K_BYTE ParamBytes[MAX_NUMBER_BYTES];

// information for the file(s)
static PM68KC_STR ParamFile = NULL;
static PM68KC_STR ParamFile2 = NULL;
static PM68KC_STR ParamFile3 = NULL;

// text for the assembler
static PM68KC_STR ParamText = NULL;

// test the assembler engine
static void AsmTest(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_BOOL TestText)
{
    M68K_UINT numberWords = (M68K_UINT)(Instruction->End - Instruction->Start);

    // assemble using the instruction
    M68K_UINT numberWordsTest = (M68K_UINT)M68KAssemble(Address, Instruction, OptArch, OptAsmFlags, AsmBufferTest, M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS);
    if (numberWordsTest == 0)
    {
        printf("serr: cannot assemble the instruction\n");
        return;
    }

    // same number of words?
    if (numberWords != numberWordsTest)
    {
        printf("serr: the instruction has a different size: %u != %u\n", numberWords, numberWordsTest);
        return;
    }

    // same data?
    if (memcmp(Instruction->Start, AsmBufferTest, (M68K_LWORD)numberWords << 1) != 0)
    {
        printf("serr: the instruction has different opcodes: ");

        for (M68K_UINT index = 0; index < numberWords; index++)
        {
            if (Instruction->Start[index] == AsmBufferTest[index])
                printf("%04x ", M68KReadWord(Instruction->Start + index));
            else
                printf("%04x/%04x ", M68KReadWord(Instruction->Start + index), M68KReadWord(AsmBufferTest + index));
        }

        puts("");
        return;
    }
    
    if (TestText)
    {
        // convert to text
        if (M68KDisassembleTextForAssembler(Address, Instruction, OptDisTextFlags, DisTextTest, sizeof(DisTextTest)) == 0)
        {
            printf("serr: cannot convert the instruction to text\n");
            return;
        }

        // assemble using the text
        M68K_ASM_ERROR asmError;

        numberWordsTest = (M68K_UINT)M68KAssembleText(Address, DisTextTest, OptArch, OptAsmFlags, AsmBufferTest, M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS, &asmError);
        if (numberWordsTest == 0)
        {
            printf("terr: cannot assemble the instruction using a text: %s\n", M68KGetAsmErrorTypeText(asmError.Type));
            puts(DisTextTest);
            return;
        }

        // same number of words?
        if (numberWords != numberWordsTest)
        {
            printf("terr: the instruction has a different size when using the text: %u != %u\n", numberWords, numberWordsTest);
            return;
        }

        // same data?
        if (memcmp(Instruction->Start, AsmBufferTest, (M68K_LWORD)numberWords << 1) != 0)
        {
            printf("terr: the instruction has different opcodes when using the text: ");

            for (M68K_UINT index = 0; index < numberWords; index++)
            {
                if (Instruction->Start[index] == AsmBufferTest[index])
                    printf("%04x ", M68KReadWord(Instruction->Start + index));
                else
                    printf("%04x/%04x ", M68KReadWord(Instruction->Start + index), M68KReadWord(AsmBufferTest + index));
            }

            puts("");
            return;
        }
    }
}

// write an error message for the option and exit 
static void CantConvertValueForOption(PM68KC_STR argOption)
{
    printf("cannot convert the value for option %s\n", argOption);
    Exit(EXIT_FAILURE);
}

// check the arguments and determine the command we need to execute
static COMMAND_TYPE CheckArguments(int argc, char** argv)
{
    // parse all options
    while (argc != 0)
    {
        // get the next argument and check if it start with a -
        PM68KC_STR argOption = *argv;
        if (argOption[0] != '-')
            break;

        // ... and check it
        if (strcmp(argOption, "-arch") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptArch = (M68K_ARCHITECTURE_VALUE)ConvertDWord(argOption, *(++argv), M68K_FALSE);
        }
        else if (strcmp(argOption, "-asm-flags") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptAsmFlags = (M68K_ASM_FLAGS_VALUE)ConvertWord(argOption, *(++argv));
        }
        else if (strcmp(argOption, "-asm-tests") == 0)
            OptAsmTests = M68K_TRUE;
        else if (strcmp(argOption, "-dis-flags") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptDisFlags = (M68K_DISASM_FLAGS_VALUE)ConvertWord(argOption, *(++argv));
        }
        else if (strcmp(argOption, "-dis-for-assembler") == 0)
            OptDisForAssembler = M68K_TRUE;
        else if (strcmp(argOption, "-dis-text-format") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptDisTextFormat = *(++argv);

            if (StrIsEmptyOrNull(OptDisTextFormat))
            {
                printf("invalid text format for option %s\n", argOption);
                Exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(argOption, "-dis-text-flags") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptDisTextFlags = (M68K_DISASM_TEXT_FLAGS_VALUE)ConvertWord(argOption, *(++argv));
        }
        else if (strcmp(argOption, "-file-position") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptFilePosition = (M68K_INT)ConvertDWord(argOption, *(++argv), M68K_TRUE);
        }
        else if (strcmp(argOption, "-file-size") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptFileSize = (M68K_INT)ConvertDWord(argOption, *(++argv), M68K_TRUE);
        }
        else if (strcmp(argOption, "-ref-addr") == 0)
        {
            // we need one parameter
            if (argc < 1)
                NotEnoughParametersForOption(argOption);

            argc--;
            OptRefAddress = ConvertDWord(argOption, *(++argv), M68K_FALSE);
        }
        else if (strcmp(argOption, "-show-config") == 0)
            OptShowConfiguration = M68K_TRUE;
        else
        {
            printf("invalid option %s\n", argOption);
            Exit(EXIT_INVALID_PARAMS);
        }

        // check the next argument
        argc--;
        argv++;
    }

    // command is present?
    if (argc <= 0)
        return CT_UNKNOWN;

    PM68KC_STR argCommand = *argv;
    
    if (strcmp(argCommand, "a") == 0)
    {
        // we need one parameter
        if (argc < 1)
            NotEnoughParametersForCommand(argCommand);

        // save the text
        argc -= 2;
        ParamText = *(++argv);

        if (argc != 0)
            TooManyParametersForCommand(argCommand);

        return CT_ASSEMBLE_TEXT;
    } 
    else if (strcmp(argCommand, "d") == 0)
    {
        M68K_BYTE currentByte = 0;
        M68K_BOOL lowNibble = M68K_TRUE;

        // convert all nibbles
        for (argc--; argc != 0; argc--)
        {
            // check all chars in the text;
            // the chars that valid for a nibble are: 0-9, a-f, A-F;
            // the chars ' ', '-' or '\t' are considered spaces and will be ignored;
            // all other chars will generate an error
            char c;
            for (PM68KC_STR arg = *(++argv); (c = *arg) != 0; arg++)
            {
                M68K_BYTE nibbleValue;

                if (c >= '0' && c <= '9')
                    nibbleValue = (M68K_BYTE)(c - '0');
                else if (c >= 'a' && c <= 'f')
                    nibbleValue = (M68K_BYTE)((c - 'a') + 10);
                else if (c >= 'A' && c <= 'F')
                    nibbleValue = (M68K_BYTE)((c - 'A') + 10);
                else if (c != '\t' && c != ' ' && c != '-')
                {
                    printf("invalid nibble %c (\\x%x) for command %s\n", c, c, argCommand);
                    Exit(EXIT_FAILURE);
                    break;
                }
                else
                    continue;

                // low part of the nibble?
                if (lowNibble)
                {
                    currentByte = nibbleValue << 4;
                    lowNibble = M68K_FALSE;
                }
                else
                {
                    currentByte |= nibbleValue;

                    // too many bytes?
                    if (ParamNumberBytes >= MAX_NUMBER_BYTES)
                    {
                        printf("too many nibbles (maximum %d i.e. %d bytes) for command %s\n", MAX_NUMBER_BYTES << 1, MAX_NUMBER_BYTES, arg);
                        Exit(EXIT_FAILURE);
                        break;
                    }

                    ParamBytes[ParamNumberBytes++] = currentByte;

                    // prepare the next yte
                    currentByte = 0;
                    lowNibble = M68K_TRUE;
                }
            }
        }

        // odd number of nibbles?
        if (!lowNibble)
        {
            printf("odd number of nibbles for comand %s\n", argCommand);
            Exit(EXIT_FAILURE);
        }

        // we need an even number of bytes
        if (ParamNumberBytes == 0)
        {
            printf("you must specify at least two bytes for comand %s\n", argCommand);
            Exit(EXIT_FAILURE);
        }

        if ((ParamNumberBytes & 1) != 0)
        {
            printf("you must specify an even number of bytes for comand %s\n", argCommand);
            Exit(EXIT_FAILURE);
        }

        return CT_DISASSEMBLE_NIBBLES;
    }
    else if (strcmp(argCommand, "df") == 0)
    {
        // we need one parameter
        if (argc < 1)
            NotEnoughParametersForCommand(argCommand);

        // save the text
        argc -= 2;
        ParamFile = *(++argv);

        if (argc != 0)
            TooManyParametersForCommand(argCommand);

        return CT_DISASSEMBLE_FILE;
    }
#ifdef M68K_INTERNAL_FUNCTIONS
    else if (strcmp(argCommand, "db") == 0)
    {
        // we need one parameter
        if (argc < 1)
            NotEnoughParametersForCommand(argCommand);

        // save the text
        argc -= 2;
        ParamFile = *(++argv);

        if (argc != 0)
            TooManyParametersForCommand(argCommand);

        return CT_DISASSEMBLE_BINARY_FILE;
    }
    else if (strcmp(argCommand, "gb") == 0)
    {
        // we need one parameter
        if (argc < 1)
            NotEnoughParametersForCommand(argCommand);

        // save the text
        argc -= 2;
        ParamFile = *(++argv);

        if (argc != 0)
            TooManyParametersForCommand(argCommand);

        return CT_GENERATE_BINARY_FILE;
    }
    else if (strcmp(argCommand, "gh") == 0)
    {
        // we need three parameters
        if (argc < 3)
            NotEnoughParametersForCommand(argCommand);

        // save the texts
        argc -= 4;
        ParamFile = *(++argv);
        ParamFile2 = *(++argv);
        ParamFile3 = *(++argv);

        if (argc != 0)
            TooManyParametersForCommand(argCommand);

        return CT_GENERATE_HEADER_FILES;
    }
#endif

    printf("invalid command %s\n", argCommand);
    Exit(EXIT_FAILURE);
    return CT_UNKNOWN;
}

// try to convert an argument to a dword value
static M68K_DWORD ConvertDWord(PM68KC_STR argOption, PM68KC_STR argValue, M68K_BOOL ZeroOrGreater)
{
    char* next = NULL;
    unsigned long int value = strtoul(argValue, &next, 0);
    if (next != NULL)
    {
        if (*next == 0 && errno != ERANGE)
        {
            if (value >= 0 || !ZeroOrGreater)
                return value;
        }
    }

    CantConvertValueForOption(argOption);
    return 0;
}

// try to convert an argument to a word value
static M68K_WORD ConvertWord(PM68KC_STR argOption, PM68KC_STR argValue)
{
    M68K_DWORD value = ConvertDWord(argOption, argValue, M68K_FALSE);

    // value within the allowed limits?
    if ((value & 0xffff0000) == 0)
        return (M68K_WORD)value;

    CantConvertValueForOption(argOption);
    return 0;
}

// disassemble the binary file with all instructions
static M68K_BOOL DisassembleBinaryFile(PM68KC_STR fileName)
{
    M68K_BOOL result = M68K_FALSE;

    // read the whole file to memory
    M68K_INT size;
    PM68K_WORD buffer = (PM68K_WORD)ReadFromFile(fileName, OptFilePosition, 0x7fffffff, &size);
    if (buffer != NULL)
    {
        PM68KC_STR errFormat;
        PM68K_WORD next = buffer;
        PM68K_WORD bufferEnd = next + (size >> 1);
        PM68K_WORD address = (PM68K_WORD)(M68K_LWORD)OptRefAddress;

        while ((next + BINARY_INSTR_MINIMUM_NUMBER_WORDS) <= bufferEnd)
        {
            // read all values to the header
            BINARY_INSTR_HEADER binaryInstruction;
            
            binaryInstruction.NumberWords = M68KReadWord(next + BINARY_INSTR_WINDEX_NUMBER_WORDS);
            binaryInstruction.Type = M68KReadWord(next + BINARY_INSTR_WINDEX_TYPE);
            binaryInstruction.Architectures = M68KReadDWord(next + BINARY_INSTR_WINDEX_ARCHITECTURES);

            if (binaryInstruction.NumberWords < BINARY_INSTR_MINIMUM_NUMBER_WORDS)
            {
                errFormat = "invalid size at #0x%x\n";
            error:
                printf(errFormat, OptFilePosition + (M68K_INT)((next - buffer) << 1));
                break;
            }

            if (binaryInstruction.Type == M68K_IT_INVALID || binaryInstruction.Type >= M68K_IT__SIZEOF__)
            {
                errFormat = "invalid type at #0x%x\n";
                goto error;
            }

            if ((binaryInstruction.Architectures & ~M68K_ARCH__ALL__) != 0)
            {
                errFormat = "invalid architecture at #0x%x\n";
                goto error;
            }

            M68K_UINT totalNumberValues;
            M68K_INSTRUCTION instruction;
            M68K_VALUE_LOCATION values[M68K_MAXIMUM_NUMBER_OPERANDS * M68K_MAXIMUM_NUMBER_VALUES_OPERAND];

            PM68K_WORD firstWord = next + BINARY_INSTR_WINDEX_OPCODES;
            PM68K_WORD lastWord = next + binaryInstruction.NumberWords;
            PM68K_WORD endInstr = M68KDisassembleEx(address, firstWord, lastWord, OptDisFlags, OptArch, &instruction, M68K_IT_INVALID, values, ((M68K_WORD)1 << M68K_MAXIMUM_NUMBER_OPERANDS) - 1, M68K_MAXIMUM_NUMBER_OPERANDS * M68K_MAXIMUM_NUMBER_VALUES_OPERAND, &totalNumberValues);
            if (endInstr == NULL)
            {
                errFormat = "invalid instruction at #0x%x\n";
                goto error;
            }

            if (endInstr != lastWord)
                printf("derr: inconsistent size at #0x%x\n", OptFilePosition + (M68K_INT)((next - buffer) << 1));

            size = (M68K_INT)(endInstr - firstWord);

            // filter by architecture
            if ((binaryInstruction.Architectures & OptArch) != 0)
            {
                // convert to text
                M68K_LUINT textSize;

                if (OptDisForAssembler)
                    textSize = M68KDisassembleTextForAssembler(address, &instruction, OptDisTextFlags, DisText, sizeof(DisText));
                else
                    textSize = M68KDisassembleText(address, &instruction, OptDisTextFormat, OptDisTextFlags, DisText, sizeof(DisText));

                if (textSize == 0)
                {
                    errFormat = "cannot convert to text at #0x%x\n";
                    goto error;
                }

                printf("#0x%08x: %s\n", (M68K_UINT)(next - buffer) << 1, DisText);

                M68K_BOOL checkInvalidOpcode = M68K_TRUE;
                for (PM68K_WORD nextWord = instruction.Start; nextWord < lastWord; nextWord++)
                {
                    if (checkInvalidOpcode && nextWord >= instruction.Stop)
                    {
                        checkInvalidOpcode = M68K_FALSE;
                        printf("? ");
                    }

                    printf("%02x %02x ", ((PM68K_BYTE)nextWord)[0], ((PM68K_BYTE)nextWord)[1]);
                }

                printf("\n");

                PM68K_VALUE_LOCATION nextValue = values;
                for (M68K_UINT valueIndex = 0; valueIndex < totalNumberValues; valueIndex++, nextValue++)
                {
                    static PM68KC_STR valueTypesText[M68K_VT__SIZEOF__] =
                    {
                        "u",    // M68K_VT_UNKNOWN
                        "al",   // M68K_VT_ABSOLUTE_L
                        "aw",   // M68K_VT_ABSOLUTE_W
                        "bdb",  // M68K_VT_BASE_DISPLACEMENT_B
                        "bdl",  // M68K_VT_BASE_DISPLACEMENT_L
                        "bdw",  // M68K_VT_BASE_DISPLACEMENT_W
                        "db",   // M68K_VT_DISPLACEMENT_B
                        "dl",   // M68K_VT_DISPLACEMENT_L
                        "dw",   // M68K_VT_DISPLACEMENT_W
                        "ib",   // M68K_VT_IMMEDIATE_B
                        "id",   // M68K_VT_IMMEDIATE_D
                        "il",   // M68K_VT_IMMEDIATE_L
                        "ip",   // M68K_VT_IMMEDIATE_P
                        "iq",   // M68K_VT_IMMEDIATE_Q
                        "is",   // M68K_VT_IMMEDIATE_S
                        "iw",   // M68K_VT_IMMEDIATE_W
                        "ix",   // M68K_VT_IMMEDIATE_X
                        "odl",  // M68K_VT_OUTER_DISPLACEMENT_L
                        "odw",  // M68K_VT_OUTER_DISPLACEMENT_W
                    };

                    M68K_UINT valueOffset = (M68K_UINT)(nextValue->Location - (PM68K_BYTE)instruction.Start);
                    for (M68K_UINT newPosition = 3 * valueOffset; newPosition != 0; newPosition--)
                        printf(" ");

                    printf("%u:%s\n", (M68K_UINT)nextValue->OperandIndex, valueTypesText[nextValue->Type]);
                }

                printf("\n");

                if (OptAsmTests)
                    AsmTest(address, &instruction, M68K_TRUE);
            }

            next = lastWord;
        }

        // free the memory block
        free(buffer);
    }

    return result;
}

// disassemble all instruction in the buffer
static M68K_UINT DisassembleBuffer(PM68K_WORD address, PM68K_WORD start, PM68K_WORD end)
{
    M68K_UINT count;
    M68K_INSTRUCTION instruction;

    for (count = 0; start < end; )
    {
        // write the address
        printf(FMT_REFADDR " ", (M68K_DWORD)(M68K_LWORD)address);

        if (M68KDisassemble(address, start, end, OptDisFlags, OptArch, &instruction) != M68K_NULL)
        {
            // convert to text
            M68K_LUINT Size;

            if (OptDisForAssembler)
                Size = M68KDisassembleTextForAssembler(address, &instruction, OptDisTextFlags, DisText, sizeof(DisText));
            else
                Size = M68KDisassembleText(address, &instruction, OptDisTextFormat, OptDisTextFlags, DisText, sizeof(DisText));

            if (Size != 0)
            {
                puts(DisText);

                if (OptAsmTests)
                    AsmTest(address, &instruction, M68K_TRUE);
            }
            else
                puts("cannot convert the instruction to text");

            // move forward to the next instruction
            address += (instruction.End - start);
            start = instruction.End;
            
            // increment the number of valid disassembled instructions
            count++;
        }
        else
        {
            // invalid instruction
            if (OptDisForAssembler)
                puts("?");
            else
                printf(FMT_INVALID_OPCODE "\n", M68KReadWord(start));

            // move forward one word
            start++;
            address++;
        }
    }

    return count;
}

// exit the application
static void Exit(int errorCode)
{
    exit(errorCode);
}

// write an error message for the command and exit 
static void NotEnoughParametersForCommand(PM68KC_STR argCommand)
{
    printf("not enough parameters for command %s\n", argCommand);
    Exit(EXIT_FAILURE);
}

// write an error message for the option and exit 
static void NotEnoughParametersForOption(PM68KC_STR argOption)
{
    printf("not enough parameters for option %s\n", argOption);
    Exit(EXIT_FAILURE);
}

// print the current configuration
static void PrintConfiguration(PM68KC_STR title)
{
    printf(
        "%s:\n"
        " arch            = 0x%08x\n"
        " asm-flags       = 0x%04x\n"
        " dis-flags       = 0x%04x\n"
        " dis-text-format = %s\n"
        " dis-text-flags  = 0x%04x\n"
        " file-position   = %d\n"
        " file-size       = 0x%08x\n"
        " ref-addr        = 0x%08x\n",
        title,
        (M68K_UINT)OptArch,
        OptAsmFlags,
        OptDisFlags,
        OptDisTextFormat,
        OptDisTextFlags,
        OptFilePosition,
        OptFileSize,
        OptRefAddress
    );
}

// print the usage message and exit
static void PrintUsageAndExit()
{
    printf(
        "M68K <option>* <command>\n"
        "v" VERSION
#ifdef M68K_TARGET_IS_64_BIT
        " (64-bit)"
#else
        " (32-bit)"
#endif
        "\n"
        "\n"
        "options:\n"
        " -arch <uint32>            define the allowed architectures (mask):\n"
        "\n"
        "    M68K_ARCH_68000   = 0x00000001\n"
        "    M68K_ARCH_68008   = 0x00000002\n"
        "    M68K_ARCH_68010   = 0x00000004\n"
        "    M68K_ARCH_68020   = 0x00000008\n"
        "    M68K_ARCH_68030   = 0x00000010 (mmu)\n"
        "    M68K_ARCH_68EC030 = 0x00000020\n"
        "    M68K_ARCH_68040   = 0x00000040 (mmu+fpu)\n"
        "    M68K_ARCH_68EC040 = 0x00000080\n"
        "    M68K_ARCH_68LC040 = 0x00000100 (mmu)\n"
        "    M68K_ARCH_68060   = 0x00000200 (mmu+fpu)\n"
        "    M68K_ARCH_68EC060 = 0x00000400\n"
        "    M68K_ARCH_68LC060 = 0x00000800 (mmu)\n"
        "    M68K_ARCH_68851   = 0x00001000 (mmu)\n"
        "    M68K_ARCH_68881   = 0x00002000 (fpu)\n"
        "    M68K_ARCH_68882   = 0x00004000 (fpu)\n"
        "    M68K_ARCH_CPU32   = 0x00008000\n"
        "    M68K_ARCH_AC68080 = 0x00010000 (fpu+vex)\n"
        "\n"
        " -asm-flags <uint16>       define the flags for the assembler (mask):\n"
        "\n"
        "    M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS = 0x0001\n"
        "    M68K_AFLG_ALLOW_BANK_EXTENSION_WORD = 0x0002\n"
        "\n"
        " -asm-tests                perform internal tests by reassembling the instruction(s)\n"
        "\n"
        " -dis-flags <uint16>       define the flags for the disassembler (mask):\n"
        "\n"
        "    M68K_DFLG_ALLOW_CPX                 = 0x0001\n"
        "    M68K_DFLG_ALLOW_FMOVE_KFACTOR       = 0x0002\n"
        "    M68K_DFLG_ALLOW_RPC_REGISTER        = 0x0004\n"
        "    M68K_DFLG_ALLOW_ADDRESS_OPERAND     = 0x0008\n"
        "    M68K_DFLG_ALLOW_BANK_EXTENSION_WORD = 0x0010\n"
        "\n"
        " -dis-for-assembler        disassemble (convert to text) using the assembler format\n"
        "\n"
        " -dis-text-format <text>   define the format for the disassembled instructions.\n"
        "                           Ignored when \"-dis-for-assembler\" is used\n"
        "\n"
        " -dis-text-flags <uint16>  define the flags for the disassembler (mask):\n"
        "\n"
        "    M68K_DTFLG_ALL_LOWERCASE             = 0x0001\n"
        "    M68K_DTFLG_ALL_UPPERCASE             = 0x0002\n"
        "    M68K_DTFLG_IGNORE_SIGN               = 0x0004\n"
        "    M68K_DTFLG_TRAILING_ZEROS            = 0x0008\n"
        "    M68K_DTFLG_C_HEXADECIMALS            = 0x0010\n"
        "    M68K_DTFLG_SCALE_1                   = 0x0020\n"
        "    M68K_DTFLG_HIDE_IMPLICIT_SIZES       = 0x0040\n"
        "    M68K_DTFLG_CONDITION_CODE_AS_OPERAND = 0x0080\n"
        "    M68K_DTFLG_HIDE_IMMEDIATE_PREFIX     = 0x0100\n"
        "    M68K_DTFLG_DISPLACEMENT_AS_VALUE     = 0x0200\n"
        "\n"
        " -file-position <int32>    define the start position in the file\n"
        "\n"
        " -file-size <int32>        define the number of bytes that are read from the file.\n"
        "                           Use 0x7fffffff to read all bytes that are available\n"
        "\n"
        " -ref-addr <uint32>        define the reference address\n"
        "\n"
        " -show-config              show the configuration before executing a command\n"
        "\n"
        "commands:\n"
        " a <text>                  assemble using a text\n"
        "\n"
        " d <nibbles>               disassemble using a sequence of nibbles\n"
        "\n"
        " df <file>                 disassemble using a file (check -file-x options)\n"
        "\n"
#ifdef M68K_INTERNAL_FUNCTIONS
        " db <file>                 disassemble all instructions in the binary file generated with command \"gb\"\n"
        "\n"
        " gb <file>                 generate the binary file with all instructions\n"
        "\n"
        " gh <index_first_words.h>  generate the header files required by the M68K library.\n"
        "    <opcode_maps.h>        All files are overwritten\n"
        "    <words.h>\n"
#endif
        "\n"
        "assembler syntax:\n"
        "\n"
        "   <space>*<mnemonic><size>?<space>+(<operand>(<space>*,<space>*<operand>)*)?\n"
        "\n"
        " where\n"
        "\n"
        "   space       = [\\t\\n\\r]\n"
        "   mnemonic    = [a-z][0-9a-z]*\n"
        "   size        = \\.[bdlpqswx]\n"
        "   operand     = %%<type>\\(<space>*[value]<space>*(,<space>*[value]<space>*)*\\)\n"
        "   type        = one of\n"
        "       ab      = address using byte displacement: <addr>\n"
        "       al      = address using word displacement: <addr>\n"
        "       aw      = address using long displacement: <addr>\n"
        "       cc      = condition code: <cc>\n"
        "       cid     = coprocessor id: <imm:0-7>\n"
        "       cidcc   = coprocessor id and condition code: <imm:0..7>,<imm:0..0x3f>\n"
        "       ct      = cache type: <cache type>\n"
        "       db      = byte displacement: <imm:0..0xff>\n"
        "       dkf     = dynamic k-factor: <dreg>\n"
        "       dl      = word displacement: <imm:0..0xffffffff>\n"
        "       dw      = long displacement: <imm:0..0xffff>\n"
        "       fcc     = fpu condition code: <fpu cc>\n"
        "       fcl     = fpu control register list: <fcreg list>\n"
        "       fl      = fpu register list: <freg list>\n"
        "       ib      = immediate byte: <imm:0..0xff>\n"
        "       id      = immediate double: <double>\n"
        "       il      = immediate long: <imm:0..0xffffffff>\n"
        "       ip      = immediate packed decimal: <immpd:0..0xffffffffffffffffffffffff>\n"
        "       iq      = immediate quad: <immq:0..0xffffffffffffffff>\n"
        "       is      = immediate single: <single>\n"
        "       iw      = immediate word: <imm:0..0xffff>\n"
        "       ix      = immediate extended: <extended>\n"
        "       m       = memory: <mem param>(,<mem param>)*\n"
        "       mcc     = mmu condition code: <mmu cc>\n"
        "       md      = memory with pre-decrement: <areg>\n"
        "       mi      = memory with post-increment: <areg>\n"
        "       mp      = memory pair: <dreg>|<areg>,<dreg>|<areg>\n"
        "       ow      = offset and width: <imm:0..31>|<dreg>,<imm:1..32>|<dreg>\n"
        "       r       = register: <reg>\n"
        "       rl      = register list: <reg list>\n"
        "       rp      = register pair: <dreg>,<dreg>\n"
        "       skf     = static k-factor: <imm:-64..63>\n"
        "       vrp2    = virtual register pair multiple of 2: <dreg>,<dreg>\n"
        "       vrp4    = virtual register pair multiple of 4: <dreg>,<dreg>\n"
        "   addr        = &<imm:0..0xffffffff>\n"
        "   areg        = address register; check <reg>\n"
        "   areg range  = <areg>(-<areg>)?\n"
        "   base reg    = <areg>|pc|zpc\n"
        "   index reg   = (<areg>|<dreg>)(\\.[wl])?\n"
        "   cache type  = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-4\n"
        "   cc          = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 4-172\n"
        "   disp        = <imm:0..0xffffffff><size>?\n"
        "   double      = [+-]?(Inf|NaN|0(x[01]\\.[0-9a-f]{1,13}p[+-]?[0-9]{1,4})?)\n"
        "   dreg        = data register; check <reg>\n"
        "   dreg range  = <dreg>(-<dreg>)?\n"
        "   extended    = [+-]?(Ind|Inf|NaN|PInf|PNaN|QNaN|QInf|0(x[01]\\.[0-9a-f]{1,16}p[+-]?[0-9]{1,5})?)\n"
        "   fpu cc      = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 3-31\n"
        "   fcreg       = fpu control register; check <reg>\n"
        "   fcreg list  = 0|<fcreg>(/<fcreg>(/<fcreg>)?)?\n"
        "   freg        = fpu register; check <reg>\n"
        "   freg list   = 0|<freg>(-<freg>)?(/<freg>(-<freg>)?)*\n"
        "   imm         = [+-]?(0((x[0-9a-f]{1,8})|[0-9]{1,9})|[1-9][0-9]{1,9})\n"
        "   immpd       = 0x[0-9a-f]{1,24}\n"
        "   immq        = 0x[0-9a-f]{1,16}\n"
        "   mem param   = <addr> | <disp> | <base reg> | <index reg>\\*[1248] | \\[ <space>*<mem param><space>*(,<space>*<mem param><space>*)+ \\]\n"
        "   mmu cc      = [a-zA-Z]+; check M68000 FAMILY PROGRAMMER’S REFERENCE MANUAL 6-61\n"
        "   reg         = [a-z][0-9a-z]*\n"
        "   reg list    = 0|(<areg range>|<dreg range>)(/(<areg range>|<dreg range>))*\n"
        "   single      = [+-]?(Inf|NaN|0(x[01]\\.[0-9a-f]{1,6}p[+-]?[0-9]{1,3})?)\n"
        "\n"
        " note: the parser is not case-sensitive; for example 0X1234 and 0xABCD are not valid hexadecimal numbers\n"
        "\n"
        "examples:\n"
        " M68K a \"move.w %%r(d0), %%r(d1)\"\n"
        " M68K d \"4e71\"\n"
        " M68K d 4e 71\n"
        " M68K -ref-addr 0xf80000 df \"amiga.rom\"\n"
#ifdef M68K_INTERNAL_FUNCTIONS
        " M68K db m68k.dat\n"
        " M68K gb m68k.dat\n"
        " M68K gh /path/index_first_words.h /path/opcode_maps.h /path/words.h\n"
#endif
        "\n"
    );

    PrintConfiguration("default values");

    // invalid parameters
    Exit(EXIT_INVALID_PARAMS);
}

// read a block of data from a file
static PM68K_BYTE ReadFromFile(PM68KC_STR fileName, M68K_INT position, M68K_INT size, PM68K_INT readSize)
{
    PM68K_BYTE result = NULL;
    
    if (readSize != NULL)
        *readSize = 0;

    FILE* file = fopen(fileName, "rb");
    if (file != NULL)
    {
        // position at the end
        if (fseek(file, 0, SEEK_END) == 0)
        {
            long int fileSize = ftell(file);
            if (fileSize > 0)
            {
                // the position is valid?
                if (position < fileSize)
                {
                    if (size == 0x7fffffff)
                        size = fileSize - position;
                    else if (size > (fileSize - position))
                    {
                        printf("cannot read 0x%x bytes starting at position 0x%x (0x%lx available) from file %s\n", size, position, fileSize - position, fileName);
                        size = 0;
                    }

                    // the size must be multiple of 2 i.e. a word
                    size &= ~(sizeof(M68K_WORD) - 1);

                    if (size != 0)
                    {
                        if (fseek(file, position, SEEK_SET) == 0)
                        {
                            // allocate memory for the whole block
                            PM68K_BYTE buffer = (PM68K_BYTE)malloc((size_t)size);
                            if (buffer != NULL)
                            {
                                // read the data to memory
                                if (fread(buffer, sizeof(M68K_BYTE), (size_t)size, file) == (size_t)size)
                                {
                                    result = buffer;
                                    
                                    if (readSize != NULL)
                                        *readSize = size;
                                }
                                else
                                    printf("cannot read 0x%x bytes from file %s at 0x%x\n", size, fileName, position);

                                if (result == NULL)
                                    free(buffer);
                            }
                            else
                                printf("not enough memory to read 0x%x bytes from file %s\n", size, fileName);
                        }
                        else
                            printf("cannot move to the start of file %s\n", fileName);
                    }
                }
                else
                    printf("invalid position in file %s\n", fileName);
            }
            else
                printf("cannot determine the size of file %s\n", fileName);
        }
        else
            printf("cannot move to the end of file %s\n", fileName);

        fclose(file);
    }
    else
        printf("cannot open the file %s to read data\n", ParamFile);

    return result;
}

// check if a string is null or empty
static M68K_BOOL StrIsEmptyOrNull(PM68KC_STR String)
{
    if (String != NULL)
        return (M68K_BOOL)(String[0] == 0);
    else
        return M68K_TRUE;
}

// write an error message for the command and exit 
static void TooManyParametersForCommand(PM68KC_STR argCommand)
{
    printf("too many parameters for the command %s\n", argCommand);
    Exit(EXIT_FAILURE);
}

// write a block of data to a file
static M68K_BOOL WriteToFile(PM68KC_STR fileName, PM68K_BYTE buffer, M68K_INT size)
{
    M68K_BOOL result = M68K_FALSE;

    FILE* file = fopen(fileName, "wb");
    if (file != NULL)
    {
        if (fwrite(buffer, 1, (size_t)size, file) == (size_t)size)
            result = M68K_TRUE;
        else
            printf("cannot write 0x%x bytes to file %s\n", size, fileName);

        fclose(file);
    }
    else
        printf("cannot open the file %s to write data\n", ParamFile);

    return result;
}

int main(int argc, char* argv[])
{
#if defined(_DEBUG) && 0
    static char *args[] = 
    {
        "m68k.exe",

        // "-arch", "0x0001ffff",
        // "-asm-flags", "0x0002",
        // "-asm-tests", 
        // "-dis-flags", "0x001f",
        "-dis-for-assembler",
        // "-dis-text-format", "%40w %m%s%-%o",
        // "-dis-text-flags", "0x0050",
        // "-file-position", "0x001b571e",
        // "-file-size", "0x7fffffff",
        // "ref-addr", "0x00f80000",

        "a", 
            // "move.l %r(d5), %r(b1)",
            // "cprestore %cid(0x02), %m([&-0x0001270e],a2.w*2,0x000f4240)",
            // "touch %m(a3,a6.w*2,-0x64)",
            // "bank %ib(0x01), %ib(0x02), %ib(0x00), %ib(0x04)",
            "touch %m(a0)"
        
        /*
        "d",
            // "71 05",
            "f630 0150"
        */

        /*
        "df",
            "d:\\temp\\m68k\\all.dat"
        */

        /*
        "db",
            "d:\\temp\\m68k.dat",
        */
        /*
        "gb",
            "d:\\temp\\m68k.dat",
        */

        /*
        "gh",
            "Y:\\Projects\\VC\\M68K\\M68K\\src\\m68k\\gen\\index_first_words.h",
            "Y:\\Projects\\VC\\M68K\\M68K\\src\\m68k\\gen\\opcode_maps.h",
            "Y:\\Projects\\VC\\M68K\\M68K\\src\\m68k\\gen\\words.h",
        */
    };

    argv = args;
    argc = sizeof(args) / sizeof(char *);
#endif

    COMMAND_TYPE commandType = CheckArguments(argc - 1, argv + 1);
    switch (commandType)
    {
    case CT_ASSEMBLE_TEXT:
        if (OptShowConfiguration)
            PrintConfiguration("configuration");

        {
            M68K_ASM_ERROR asmError;

            M68K_UINT numberWords = (M68K_UINT)M68KAssembleText((PM68K_WORD)(M68K_LWORD)OptRefAddress, (PM68KC_STR)ParamText, OptArch, OptAsmFlags, AsmBuffer, M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS, &asmError);
            if (numberWords == 0)
            {
                // write the error message
                printf("cannot assemble the instruction: %s\n", M68KGetAsmErrorTypeText(asmError.Type));
                
                // try to write the position where the error occured
                M68K_UINT position = (M68K_UINT)(asmError.Location - asmError.Start);
                if (position <= strlen(ParamText))
                {
                    puts(ParamText);

                    for (M68K_UINT Index = 0; Index < position; Index++)
                        putc(' ', stdout);

                    puts("^");
                }

                Exit(EXIT_FAILURE);
            }
            
            // write all words
            printf("opcodes : ");

            for (M68K_UINT Index = 0; Index < numberWords; Index++)
                printf("%04x ", M68KReadWord(AsmBuffer + Index));

            printf("\n");

            // disassemble the instruction
            M68K_INSTRUCTION instruction;
                
            if (M68KDisassemble((PM68K_WORD)(M68K_LWORD)OptRefAddress, AsmBuffer, AsmBuffer + numberWords, OptDisFlags, OptArch, &instruction) == M68K_NULL)
            {
                printf("cannot disassemble the instruction\n");
                Exit(EXIT_FAILURE);
            }

            // same end?
            if (instruction.End != (AsmBuffer + numberWords))
            {
                printf("the disassembled instruction has a different size: %u != %u\n", (M68K_UINT)(instruction.End - AsmBuffer), (M68K_UINT)numberWords);
                Exit(EXIT_FAILURE);
            }

            // convert to text
            M68K_LUINT size;

            if (OptDisForAssembler)
                size = M68KDisassembleTextForAssembler((PM68K_WORD)(M68K_LWORD)OptRefAddress, &instruction, OptDisTextFlags, DisText, sizeof(DisText));
            else
                size = M68KDisassembleText((PM68K_WORD)(M68K_LWORD)OptRefAddress, &instruction, OptDisTextFormat, OptDisTextFlags, DisText, sizeof(DisText));

            if (size == 0)
            {
                printf("cannot convert the instruction to text\n");
                Exit(EXIT_FAILURE);
            }

            printf("disasm  : " FMT_REFADDR " %s\n", OptRefAddress, DisText);

            if (OptAsmTests)
                AsmTest((PM68K_WORD)(M68K_LWORD)OptRefAddress, &instruction, M68K_FALSE);

            puts("done");
            Exit(EXIT_SUCCESS);
        }
        break;

    case CT_DISASSEMBLE_FILE:
        if (OptShowConfiguration)
            PrintConfiguration("configuration");

        // check the options
        if (OptFilePosition >= 0)
        {
            if (OptFileSize >= 2)
            {
                PM68K_BYTE buffer = ReadFromFile(ParamFile, OptFilePosition, OptFileSize, NULL);
                if (buffer != NULL)
                {
                    printf("disassembled %u instructions\n", DisassembleBuffer((PM68K_WORD)(M68K_LWORD)OptRefAddress, (PM68K_WORD)buffer, (PM68K_WORD)((PM68K_BYTE)buffer + (OptFileSize & ~1))));

                    // free the memory block
                    free(buffer);

                    // done
                    Exit(EXIT_SUCCESS);
                }
            }
            else
                printf("the file size must be >= 2\n");
        }
        else
            printf("the file position must be >= 0\n");

        Exit(EXIT_FAILURE);
        break;

    case CT_DISASSEMBLE_NIBBLES:
        if (OptShowConfiguration)
            PrintConfiguration("configuration");

        printf("disassembled %u instruction(s)\n", DisassembleBuffer((PM68K_WORD)(M68K_LWORD)OptRefAddress, (PM68K_WORD)ParamBytes, (PM68K_WORD)(ParamBytes + ParamNumberBytes)));
        Exit(EXIT_SUCCESS);
        break;

#ifdef M68K_INTERNAL_FUNCTIONS
    case CT_DISASSEMBLE_BINARY_FILE:
        Exit(DisassembleBinaryFile(ParamFile) ? EXIT_SUCCESS : EXIT_FAILURE);
        break;

    case CT_GENERATE_BINARY_FILE:
    {
        int result = EXIT_FAILURE;

        // allocate memory for the binary file
        PM68K_BYTE binaryData = (PM68K_BYTE)malloc((size_t)MAX_BINARY_SIZE);
        if (binaryData != NULL)
        {
            M68K_UINT usedSize;
            M68K_UINT numberInstrs;

            numberInstrs = M68KInternalGenerateOpcodes((PM68K_WORD)binaryData, MAX_BINARY_SIZE >> 1, OptDisFlags, &usedSize);
            usedSize <<= 1;
            printf("Generated %u instrs, %u bytes\n", numberInstrs, usedSize);

            if (numberInstrs != 0)
                if (WriteToFile(ParamFile, binaryData, (M68K_INT)usedSize))
                    result = EXIT_SUCCESS;

            // free the binary data
            free(binaryData);
        }
        else
            printf("not enough memory to generate the binary file: 0x%x\n", MAX_BINARY_SIZE);

        Exit(result);
        break;
    }

    case CT_GENERATE_HEADER_FILES:
        M68KInternalGenerateAssemblerTables(ParamFile, ParamFile2, ParamFile3);
        Exit(EXIT_SUCCESS);
        break;
#endif

    default:
        break;
    }

    PrintUsageAndExit();
}
