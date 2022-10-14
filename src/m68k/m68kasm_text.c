#include "m68kinternal.h"

typedef enum MEMORY_PARAM_SCOPE
{
    MPS_NONE,       // no scope i.e. not used
    MPS_FINAL,      // calculating the final address
    MPS_READ_BASE,  // reading a base address i.e. inside [ ]
} MEMORY_PARAM_SCOPE, *PMEMORY_PARAM_SCOPE;

// forward declarations
static M68K_SIZE    AdjustSizeToWord(M68K_SIZE Size);
static M68K_BOOL    CharIsDecDigit(M68K_CHAR Char);
static M68K_BOOL    CharIsHexDigit(M68K_CHAR Char);
static M68K_BOOL    CharIsLetter(M68K_CHAR Char);
static M68K_BOOL    CharIsLetterOrDigit(M68K_CHAR Char);
static M68K_BOOL    CharIsNumberSign(M68K_CHAR Char);
static M68K_BOOL    CharIsNumberStart(M68K_CHAR Char);
static M68K_BOOL    CharIsSpace(M68K_CHAR Char);
static M68K_INT     CompareTexts(PM68KC_STR StaticText, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd);
static M68K_UINT    LinearSearchText(PM68KC_STR Table[], M68K_UINT Min, M68K_UINT Max, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd);
static M68K_BOOL    ParseAddr(PASM_TEXT_CTX ATCtx, PM68K_DWORD ParsedAddress);
static M68K_BOOL    ParseChar(PASM_TEXT_CTX ATCtx, M68K_CHAR Char);
static M68K_BOOL    ParseDecDigits(PASM_TEXT_CTX ATCtx, M68K_UINT MaxNumberDigits /*1..10*/, PM68K_DWORD ParsedImmediate);
static M68K_BOOL    ParseDouble(PASM_TEXT_CTX ATCtx, PM68K_DOUBLE ParsedDouble);
static M68K_BOOL    ParseExtended(PASM_TEXT_CTX ATCtx, PM68K_EXTENDED ParsedExtended);
static M68K_BOOL    ParseFPCRList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList);
static M68K_BOOL    ParseFPList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList);
static M68K_BOOL    ParseHexDigits(PASM_TEXT_CTX ATCtx, M68K_UINT MaxNumberDigits /*2..24*/, PM68K_DWORD ParsedImmediates /*[0]=0...31,[1]=32..63,...*/);
static M68K_BOOL    ParseIdentifier(PASM_TEXT_CTX ATCtx, M68K_BOOL AllowDigits);
static M68K_BOOL    ParseIEEEValue(PASM_TEXT_CTX ATCtx, PM68K_IEEE_VALUE_TYPE ParsedIEEEValue);
static M68K_BOOL    ParseImmediate(PASM_TEXT_CTX ATCtx, M68K_SDWORD MinSigned, M68K_SDWORD MaxSigned, PM68K_SDWORD ParsedImmediate);
static M68K_BOOL    ParseList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList);
static M68K_BOOL    ParseMemory(PASM_TEXT_CTX ATCtx, PM68K_OPERAND ParsedOperand);
static M68K_BOOL    ParseMnemonic(PASM_TEXT_CTX ATCtx, PM68K_INSTRUCTION_TYPE_VALUE ParsedIType, PM68K_OPERAND *Operand);
static M68K_BOOL    ParseOffsetWidth(PASM_TEXT_CTX ATCtx, PM68K_OFFSET_WIDTH ParsedOffsetWidth);
static M68K_BOOL    ParseOperand(PASM_TEXT_CTX ATCtx, PM68K_OPERAND Operand);
static M68K_BOOL    ParsePackedDecimal(PASM_TEXT_CTX ATCtx, PM68K_PACKED_DECIMAL ParsedPackedDecimal);
static M68K_BOOL    ParseQuad(PASM_TEXT_CTX ATCtx, PM68K_QUAD ParsedQuad);
static M68K_BOOL    ParseRegister(PASM_TEXT_CTX ATCtx, M68K_REGISTER_TYPE_VALUE MinRegister1, M68K_REGISTER_TYPE_VALUE MaxRegister1, M68K_REGISTER_TYPE_VALUE MinRegister2, M68K_REGISTER_TYPE_VALUE MaxRegister2, PM68K_REGISTER_TYPE_VALUE ParsedRegister);
static M68K_BOOL    ParseScale(PASM_TEXT_CTX ATCtx, PM68K_SCALE_VALUE ParsedScale);
static M68K_BOOL    ParseSingle(PASM_TEXT_CTX ATCtx, PM68K_SINGLE ParsedSingle);
static M68K_BOOL    ParseSize(PASM_TEXT_CTX ATCtx, M68K_BOOL SetErrorType, PM68K_SIZE_VALUE ParsedSize);
static M68K_BOOL    ParseValueSeparator(PASM_TEXT_CTX ATCtx);
static M68K_BOOL    RegisterIsValid(M68K_REGISTER_TYPE_VALUE Register, M68K_REGISTER_TYPE_VALUE MinRegister, M68K_REGISTER_TYPE_VALUE MaxRegister);
static M68K_BOOL    RegisterIsValid2(M68K_REGISTER_TYPE_VALUE Register, M68K_REGISTER_TYPE_VALUE MinRegister1, M68K_REGISTER_TYPE_VALUE MaxRegister1, M68K_REGISTER_TYPE_VALUE MinRegister2, M68K_REGISTER_TYPE_VALUE MaxRegister2);
static M68K_BOOL    SkipChar(PASM_TEXT_CTX ATCtx, M68K_CHAR Char);
static void         SkipNumberSign(PASM_TEXT_CTX ATCtx, PM68K_BOOL Negative);
static M68K_UINT    SkipSpaces(PASM_TEXT_CTX ATCtx);

// adjust a size to word if necessary
static M68K_SIZE AdjustSizeToWord(M68K_SIZE Size)
{
    return (Size == M68K_SIZE_B ? M68K_SIZE_W : Size);
}

// check if a char is a decimal digit
static M68K_BOOL CharIsDecDigit(M68K_CHAR Char)
{
    return (Char >= '0' && Char <= '9');
}

// check if a char is an hexadecimal digit
static M68K_BOOL CharIsHexDigit(M68K_CHAR Char)
{
    return ((Char >= '0' && Char <= '9') || (Char >= 'a' && Char <= 'f'));
}

// check if a char is a letter
static M68K_BOOL CharIsLetter(M68K_CHAR Char)
{
    return ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z'));
}

// check if a char is a letter or a digit
static M68K_BOOL CharIsLetterOrDigit(M68K_CHAR Char)
{
    return ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z') || (Char >= '0' && Char <= '9'));
}

// check if the char is a number sign
static M68K_BOOL CharIsNumberSign(M68K_CHAR Char)
{
    return (Char == '+' || Char == '-');
}

// check if the char identifies the start of a number
static M68K_BOOL CharIsNumberStart(M68K_CHAR Char)
{
    return (CharIsDecDigit(Char) || CharIsNumberSign(Char));
}

// check if a char is a space
static M68K_BOOL CharIsSpace(M68K_CHAR Char)
{
    return (Char == '\t' || Char == '\n' || Char == '\r' || Char == ' ');
}

// search (linear) for a dynamic text in a table of static texts; returns the index of the text in the table or Max + 1 (not found)
static M68K_UINT LinearSearchText(PM68KC_STR Table[], M68K_UINT Min /*must be >= 1*/, M68K_UINT Max, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd)
{
    for (;Min <= Max; Min++)
        if (CompareTexts(Table[Min], DynamicTextStart, DynamicTextEnd) == 0)
            break;

    return Min;
}

// parse an address: &<imm:0..0xffffffff>
static M68K_BOOL ParseAddr(PASM_TEXT_CTX ATCtx, PM68K_DWORD ParsedAddress)
{
    if (ParseChar(ATCtx, '&'))
    {
        M68K_SDWORD SValue;
        if (ParseImmediate(ATCtx, MIN_LONG, MAX_LONG, &SValue))
        {
            *ParsedAddress = (M68K_DWORD)SValue;
            return M68K_TRUE;
        }
    }

    return M68K_FALSE;
}

// parse a char
static M68K_BOOL ParseChar(PASM_TEXT_CTX ATCtx, M68K_CHAR Char)
{
    if (ATCtx->Error.Location[0] == Char)
    {
        // char is valid; skip it
        ATCtx->Error.Location++;
        return M68K_TRUE;
    }
    
    ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
    return M68K_FALSE;
}

// parse a sequence of decimal digits: [0-9]+
static M68K_BOOL ParseDecDigits(PASM_TEXT_CTX ATCtx, M68K_UINT MaxNumberDigits /*1..10*/, PM68K_DWORD ParsedImmediate)
{
    PM68KC_STR Next = ATCtx->Error.Location;

    // clear the immediate value
    ParsedImmediate[0] = 0;

    // start parsing the digits
    M68K_UINT NumberUsedDigits = 0;
    M68K_UINT TotalNumberDigits = 0;

    for (;NumberUsedDigits < MaxNumberDigits; TotalNumberDigits++)
    {
        M68K_DWORD Digit;
        M68K_CHAR Char = *Next;

        if (CharIsDecDigit(Char))
            Digit = (M68K_DWORD)(Char - '0');
        else
            break;

        if (NumberUsedDigits != 0)
        {
            // integer overflow? (0x0ccccccc is the largest value that can be multiplied by 10)
            if (ParsedImmediate[0] >= 0x0ccccccc)
            {
                ATCtx->Error.Type = M68K_AET_DECIMAL_OVERFLOW;
                ATCtx->Error.Location = Next;
                return M68K_FALSE;
            }

            // A * 10 = A * (8 + 2) = A * 8 + A * 2
            ParsedImmediate[0] = (ParsedImmediate[0] << 3) + (ParsedImmediate[0] << 1);
        }

        // save the new digit
        ParsedImmediate[0] += Digit;

        // move to the next char
        Next++;

        // increment the number of digits? (0's on the left are ignored)
        if (Digit != 0 || NumberUsedDigits != 0)
            NumberUsedDigits++;
    }

    // save the next char
    ATCtx->Error.Location = Next;

    // found at least one digit?
    if (TotalNumberDigits != 0)
    {
        // too many digits?
        if (!CharIsDecDigit(*Next))
            return M68K_TRUE;
        else
            ATCtx->Error.Type = M68K_AET_DECIMAL_OVERFLOW;
    }
    else
        ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
    
    return M68K_FALSE;
}

// parse a double immediate value: [+-]?(inf|nan|0(x[01]\.[0-9a-f]{1,13}p[+-]?[0-9]{1,4})?)
static M68K_BOOL ParseDouble(PASM_TEXT_CTX ATCtx, PM68K_DOUBLE ParsedDouble)
{
    M68K_DWORD ParsedImmediates[
#ifdef M68K_TARGET_IS_BIG_ENDIAN
        3
#else
        2
#endif
    ];

    // start by skipping the sign
    M68K_BOOL NegativeValue;
    SkipNumberSign(ATCtx, &NegativeValue);

    // is it a number?
    PM68KC_STR Start = ATCtx->Error.Location;
    
    if (Start[0] == '0')
    {
        // hexadecimal notation?
        if (Start[1] == 'x')
        {
            // skip the "0x"
            Start = (ATCtx->Error.Location += 2);

            // the next char must be a 0 or 1
            M68K_BOOL Implicit1 = (Start[0] == '1');
            if (Implicit1 || Start[0] == '0')
            {
                // skip the implicit 0 or 1 and the .
                ATCtx->Error.Location++;
                if (ParseChar(ATCtx, '.'))
                {
                    // save the location of the first hexadecimal digit
                    Start = ATCtx->Error.Location;
                    if (ParseHexDigits(ATCtx, 13, ParsedImmediates))
                    {
                        // determine the number of digits that were parsed
                        M68K_UINT NumberDigits = (M68K_UINT)(ATCtx->Error.Location - Start);

                        if (ParseChar(ATCtx, 'p'))
                        {
                            Start = ATCtx->Error.Location;

                            // skip the sign
                            M68K_BOOL NegativeExponent;
                            SkipNumberSign(ATCtx, &NegativeExponent);

                            M68K_DWORD UExponent;
                            if (ParseDecDigits(ATCtx, 4, &UExponent))
                            {
                                M68K_SDWORD SExponent = (NegativeExponent ? -(M68K_SDWORD)UExponent : (M68K_SDWORD)UExponent);

                                // check if the exponent is valid; 
                                // normalized numbers can have an exponent between -1022 and 1023; 
                                // denormalized number always have an exponent of -1022
                                if ((Implicit1 && SExponent >= -1022 && SExponent <= 1023) || (!Implicit1 && SExponent == -1022))
                                {
                                    // shift the mantissa bits?
                                    if (NumberDigits < 13)
                                    {
                                        // how many bits do we need to shift?
                                        M68K_UINT NumberBits = (13 - NumberDigits) << 2;

                                        // shift both values
                                        ParsedImmediates[1] <<= NumberBits;
                                        ParsedImmediates[1] |= (ParsedImmediates[0] >> (32 - NumberBits));
                                        ParsedImmediates[0] <<= NumberBits;
                                    }

                                    // add the biased exponent?
                                    if (Implicit1)
                                        ParsedImmediates[1] |= (M68K_DWORD)((SExponent + 1023) & 0x7ff) << 20;

                                    // convert to double
#ifdef M68K_TARGET_IS_BIG_ENDIAN
                                    ParsedImmediates[2] = ParsedImmediates[0];
                                    *ParsedDouble = *(PM68K_DOUBLE)(ParsedImmediates + 1);
#else
                                    *ParsedDouble = *(PM68K_DOUBLE)ParsedImmediates;
#endif

check_value_sign:
                                    if (NegativeValue)
                                        *ParsedDouble = -*ParsedDouble;

                                    return M68K_TRUE;
                                }
                                else
                                {
                                    ATCtx->Error.Type = M68K_AET_INVALID_EXPONENT;
                                    ATCtx->Error.Location = Start;
                                }
                            }
                        }
                    }
                }
            }
            else
                ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
        }
        else
        {
            // parsed as zero
            ATCtx->Error.Location++;
            *ParsedDouble = 0;
            goto check_value_sign;
        }
    }
    else
    {
        // must be an ieee value
        M68K_IEEE_VALUE_TYPE IEEEValue;
        if (ParseIEEEValue(ATCtx, &IEEEValue))
        {
            // only a few ieee values are allowed: inf|nan
            if (IEEEValue == M68K_IEEE_VT_INF || IEEEValue == M68K_IEEE_VT_NAN)
            {
#ifdef M68K_TARGET_IS_BIG_ENDIAN
                *ParsedDouble = *(PM68K_DOUBLE)(_M68KIEEEDoubleValues + IEEEValue);
#else
                ParsedImmediates[0] = _M68KIEEEDoubleValues[IEEEValue][1];
                ParsedImmediates[1] = _M68KIEEEDoubleValues[IEEEValue][0];
                *ParsedDouble = *(PM68K_DOUBLE)ParsedImmediates;
#endif
                goto check_value_sign;
            }
            else
                ATCtx->Error.Type = M68K_AET_IEEE_VALUE_NOT_SUPPORTED;
        }
    }
    
    return M68K_FALSE;
}

// parse an extended immediate value: [+-]?(ind|inf|nan|pinf|pnan|qnan|qinf|0(x[01]\.[0-9a-f]{1,16}p[+-]?[0-9]{1,5})?)
static M68K_BOOL ParseExtended(PASM_TEXT_CTX ATCtx, PM68K_EXTENDED ParsedExtended)
{
    M68K_DWORD ParsedImmediates[3];

    // start by skipping the sign
    M68K_BOOL NegativeValue;
    SkipNumberSign(ATCtx, &NegativeValue);

    // is it a number?
    PM68KC_STR Start = ATCtx->Error.Location;
    
    if (Start[0] == '0')
    {
        // hexadecimal notation?
        if (Start[1] == 'x')
        {
            // skip the "0x"
            Start = (ATCtx->Error.Location += 2);

            // the next char must be a 0 or 1
            M68K_BOOL Implicit1 = (Start[0] == '1');
            if (Implicit1 || Start[0] == '0')
            {
                // skip the implicit 0 or 1 and the .
                ATCtx->Error.Location++;
                if (ParseChar(ATCtx, '.'))
                {
                    // save the location of the first hexadecimal digit
                    Start = ATCtx->Error.Location;
                    if (ParseHexDigits(ATCtx, 16, ParsedImmediates))
                    {
                        // determine the number of digits that were parsed
                        M68K_UINT NumberDigits = (M68K_UINT)(ATCtx->Error.Location - Start);

                        if (ParseChar(ATCtx, 'p'))
                        {
                            Start = ATCtx->Error.Location;

                            // skip the sign
                            M68K_BOOL NegativeExponent;
                            SkipNumberSign(ATCtx, &NegativeExponent);

                            M68K_DWORD UExponent;
                            if (ParseDecDigits(ATCtx, 5, &UExponent))
                            {
                                M68K_SDWORD SExponent = (NegativeExponent ? -(M68K_SDWORD)UExponent : (M68K_SDWORD)UExponent);

                                // check if the exponent is valid; 
                                // normalized numbers can have an exponent between -16382 and 16383;
                                // denormalized number always have an exponent of -16382
                                if ((Implicit1 && SExponent >= -16382 && SExponent <= 16383) || (!Implicit1 && SExponent == -16382))
                                {
                                    // shift the mantissa bits?
                                    if (NumberDigits < 16)
                                    {
                                        // shift more then 8 digits?
                                        if (NumberDigits < 8)
                                        {
                                            // the value of ParsedImmediates[1] is irrelevant but we need to shift ParsedImmediates[0]
                                            ParsedImmediates[1]  = (ParsedImmediates[0] << ((8 - NumberDigits) << 2));
                                            ParsedImmediates[0]  = 0;
                                        }
                                        else if (NumberDigits == 8)
                                        {
                                            // ParsedImmediates[0] is actually the correct value for ParsedImmediates[1]
                                            ParsedImmediates[1]  = ParsedImmediates[0];
                                            ParsedImmediates[0]  = 0;
                                        }
                                        else
                                        {
                                            // how many bits do we need to shift?
                                            M68K_UINT NumberBits = (16 - NumberDigits) << 2;

                                            // shift both values
                                            ParsedImmediates[1] <<= NumberBits;
                                            ParsedImmediates[1] |= (ParsedImmediates[0] >> (32 - NumberBits));
                                            ParsedImmediates[0] <<= NumberBits;
                                        }
                                    }

                                    // the mantissa only uses 63 bits so we must shift left one bit
                                    ParsedImmediates[0] = (ParsedImmediates[0] >> 1);

                                    if ((ParsedImmediates[1] & 1) != 0)
                                        ParsedImmediates[0] |= 0x80000000UL;

                                    ParsedImmediates[1] >>= 1;

                                    // the implicit 0 or 1 is saved in upper bit of the middle dword
                                    if (Implicit1)
                                        ParsedImmediates[1] |= 0x80000000UL;

                                    // convert to extended; the biased exponent is added in the highest word
                                    (*ParsedExtended)[0] = (Implicit1 ? (M68K_DWORD)((SExponent + 16383) & 0x7fff) << 16 : 0);
                                    (*ParsedExtended)[1] = ParsedImmediates[1];
                                    (*ParsedExtended)[2] = ParsedImmediates[0];

check_value_sign:
                                    if (NegativeValue)
                                        (*ParsedExtended)[0] |= 0x80000000UL;

                                    return M68K_TRUE;
                                }
                                else
                                {
                                    ATCtx->Error.Type = M68K_AET_INVALID_EXPONENT;
                                    ATCtx->Error.Location = Start;
                                }
                            }
                        }
                    }
                }
            }
            else
                ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
        }
        else
        {
            // parsed as zero
            ATCtx->Error.Location++;
            (*ParsedExtended)[0] = 0;
            (*ParsedExtended)[1] = 0;
            (*ParsedExtended)[2] = 0;
            goto check_value_sign;
        }
    }
    else
    {
        // must be an ieee value
        M68K_IEEE_VALUE_TYPE IEEEValue;
        if (ParseIEEEValue(ATCtx, &IEEEValue))
        {
            // only a few ieee values are allowed: inf|nan
            if (IEEEValue != M68K_IEEE_VT_ZERO)
            {
                (*ParsedExtended)[0] = _M68KIEEEExtendedValues[IEEEValue][0];
                (*ParsedExtended)[1] = _M68KIEEEExtendedValues[IEEEValue][1];
                (*ParsedExtended)[2] = _M68KIEEEExtendedValues[IEEEValue][2];
                goto check_value_sign;
            }
            else
                ATCtx->Error.Type = M68K_AET_IEEE_VALUE_NOT_SUPPORTED;
        }
    }

    return M68K_FALSE;
}

// parse a list of fpu control registers: 0|<fcreg>(/<fcreg>(/<fcreg>)?)?
static M68K_BOOL ParseFPCRList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList)
{
    // clear the list
    *ParsedRegisterList = 0;

    // list specified as a number?
    if (ATCtx->Error.Location[0] == '0')
    {
        ATCtx->Error.Location++;
        return M68K_TRUE;
    }

    // parse the list of registers
    do
    {
        M68K_REGISTER_TYPE_VALUE Register;
        PM68KC_STR Start = ATCtx->Error.Location;

        if (!ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register))
            return M68K_FALSE;
    
        // must be M68K_RT_FPCR, M68K_RT_FPIAR or M68K_RT_FPSR
        M68K_REGISTER_LIST Mask;

        if (Register == M68K_RT_FPCR)
            Mask = 1;
        else if (Register == M68K_RT_FPSR)
            Mask = 2;
        else if (Register == M68K_RT_FPIAR)
            Mask = 4;
        else
        {
            ATCtx->Error.Type = M68K_AET_REGISTER_NOT_SUPPORTED;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }
    
        // already used?
        if ((*ParsedRegisterList & Mask) != 0)
        {
            ATCtx->Error.Type = M68K_AET_REGISTERS_ALREADY_IN_LIST;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }

        // update the list
        *ParsedRegisterList |= Mask;
    }
    while (SkipChar(ATCtx, '/'));

    return M68K_TRUE;
}

// parse a list of fpu registers: 0|<freg>(-<freg>)?(/<freg>(-<freg>)?)*
static M68K_BOOL ParseFPList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList)
{
    // clear the list
    *ParsedRegisterList = 0;

    // list specified as a number?
    if (ATCtx->Error.Location[0] == '0')
    {
        ATCtx->Error.Location++;
        return M68K_TRUE;
    }

    // parse the list of registers
    do
    {
        M68K_REGISTER_TYPE_VALUE Register1;
        M68K_REGISTER_TYPE_VALUE Register2;
        PM68KC_STR Start = ATCtx->Error.Location;

        if (!ParseRegister(ATCtx, M68K_RT_FP0, M68K_RT_FP7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register1))
            return M68K_FALSE;
    
        // is it a range of registers?
        M68K_REGISTER_LIST Mask = ((M68K_REGISTER_LIST)1 << (Register1 - M68K_RT_FP0));

        if (SkipChar(ATCtx, '-'))
        {
            if (!ParseRegister(ATCtx, M68K_RT_FP0, M68K_RT_FP7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register2))
                return M68K_FALSE;

            // invalid range?
            if (Register1 > Register2)
            {
                ATCtx->Error.Type = M68K_AET_INVALID_RANGE_REGISTERS;
                ATCtx->Error.Location = Start;
                return M68K_FALSE;
            }

            M68K_REGISTER_LIST HMask = ((M68K_REGISTER_LIST)1 << (Register2 - M68K_RT_FP0));
            Mask = HMask | ((HMask - 1) ^ (Mask - 1));
        }
    
        // already used?
        if ((*ParsedRegisterList & Mask) != 0)
        {
            ATCtx->Error.Type = M68K_AET_REGISTERS_ALREADY_IN_LIST;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }

        // update the list
        *ParsedRegisterList |= Mask;
    }
    while (SkipChar(ATCtx, '/'));

    return M68K_TRUE;
}

// parse a sequence of hexadecimal digits: [0-9a-f]+
static M68K_BOOL ParseHexDigits(PASM_TEXT_CTX ATCtx, M68K_UINT MaxNumberDigits /*2..24*/, PM68K_DWORD ParsedImmediates /*[0]=0...31,[1]=32..63,...*/)
{
    PM68KC_STR Next = ATCtx->Error.Location;

    // clear the immediate values
    ParsedImmediates[0] = 0;

    if (MaxNumberDigits > 8)
        ParsedImmediates[1] = 0;

    if (MaxNumberDigits > 16)
        ParsedImmediates[2] = 0;

    // start parsing the digits
    M68K_UINT NumberUsedDigits = 0;
    M68K_UINT TotalNumberDigits = 0;

    for (;NumberUsedDigits < MaxNumberDigits; TotalNumberDigits++)
    {
        M68K_DWORD Digit;
        M68K_CHAR Char = *Next;

        if (CharIsDecDigit(Char))
            Digit = (M68K_DWORD)(Char - '0');
        else if (Char >= 'a' && Char <= 'f')
            Digit = (M68K_DWORD)(Char - 'a' + 10);
        else
            break;

        // shift all immediates left
        if (NumberUsedDigits != 0)
        {
            if (NumberUsedDigits >= 16)
            {
                ParsedImmediates[2] <<= 4;
                ParsedImmediates[2] |= (ParsedImmediates[1] >> 28);
            }

            if (NumberUsedDigits >= 8)
            {
                ParsedImmediates[1] <<= 4;
                ParsedImmediates[1] |= (ParsedImmediates[0] >> 28);
            }

            ParsedImmediates[0] <<= 4;
        }

        // save the new digit
        ParsedImmediates[0] |= Digit;

        // move to the next char
        Next++;

        // increment the number of digits? (0's on the left are ignored)
        if (Digit != 0 || NumberUsedDigits != 0)
            NumberUsedDigits++;
    }

    // save the next char
    ATCtx->Error.Location = Next;

    // found at least one digit?
    if (TotalNumberDigits != 0)
    {
        // too many digits?
        if (!CharIsHexDigit(*Next))
            return M68K_TRUE;
        else
            ATCtx->Error.Type = M68K_AET_HEXADECIMAL_OVERFLOW;
    }
    else
        ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
    
    return M68K_FALSE;
}

// parse an identifier; [a-z][a-z0-9]*
static M68K_BOOL ParseIdentifier(PASM_TEXT_CTX ATCtx, M68K_BOOL AllowDigits)
{
    M68K_CHAR Char = ATCtx->Error.Location[0];
    PM68KC_CHAR Next = M68K_NULL;
    
    if (CharIsLetter(Char))
    {
        // check the other chars
        Next = ATCtx->Error.Location + 1;
        
        if (AllowDigits)
        {
            while ((Char = *Next) != 0)
            {
                if (!CharIsLetterOrDigit(Char))
                    break;

                Next++;
            }
        }
        else
        {
            while ((Char = *Next) != 0)
            {
                if (!CharIsLetter(Char))
                    break;

                Next++;
            }
        }

        ATCtx->Error.Location = Next;
        return M68K_TRUE;
    }

    ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
    return M68K_FALSE;
}

// parse an ieee value (except IEEE_ZERO)
static M68K_BOOL ParseIEEEValue(PASM_TEXT_CTX ATCtx, PM68K_IEEE_VALUE_TYPE ParsedIEEEValue)
{
    PM68KC_CHAR Start = ATCtx->Error.Location;
    
    if (ParseIdentifier(ATCtx, M68K_FALSE))
    {
        M68K_UINT Index = LinearSearchText(_M68KTextIEEEValues, 0, M68K_IEEE_VT__SIZEOF__ - 1, Start, ATCtx->Error.Location);
        if (Index < M68K_IEEE_VT__SIZEOF__ && Index != M68K_IEEE_VT_ZERO)
        {
            *ParsedIEEEValue = (M68K_IEEE_VALUE_TYPE)Index;
            return M68K_TRUE;
        }

        ATCtx->Error.Type = M68K_AET_INVALID_IEEE_VALUE;
    }

    return M68K_FALSE;
}

// parse an immediate value (byte, word or long); [+-]?(0((x[0-9a-f]{1,8})|[0-9]{1,9})|[1-9][0-9]{1,9})
static M68K_BOOL ParseImmediate(PASM_TEXT_CTX ATCtx, M68K_SDWORD MinSigned, M68K_SDWORD MaxSigned, PM68K_SDWORD ParsedImmediate)
{
    // start by skipping the sign
    M68K_BOOL Negative;
    SkipNumberSign(ATCtx, &Negative);

    PM68KC_STR Start = ATCtx->Error.Location;
    M68K_CHAR Char = *(ATCtx->Error.Location);

    // first digit is always decimal
    if (!CharIsDecDigit(Char))
    {
        ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
        return M68K_FALSE;
    }

    // parse as decimal or hexadecimal?
    M68K_DWORD UValue;

    if (ATCtx->Error.Location[1] == 'x')
    {
        // skip the "0x"
        ATCtx->Error.Location += 2;

        if (!ParseHexDigits(ATCtx, 8, &UValue))
            return M68K_FALSE;
    }
    else if (!ParseDecDigits(ATCtx, 10, &UValue))
        return M68K_FALSE;

    // get the final signed number?
    M68K_SDWORD SValue = (Negative ? -(M68K_SDWORD)UValue : (M68K_SDWORD)UValue);

    if (SValue < MinSigned || SValue > MaxSigned)
    {
        // we allow an overflow if the value is valid in the supplied range using a different sign
        M68K_BOOL AllowOverflow;

        M68K_SDWORD SMask = MaxSigned + 1;
        M68K_SDWORD EMask = ~(MaxSigned | SMask);
        M68K_SDWORD EValue = (SValue & EMask);

        if (~MinSigned == MaxSigned)
            // only the allowed bits can be set
            AllowOverflow = (EValue == 0 || EValue == EMask);
        else
            AllowOverflow = M68K_FALSE;

        if (!AllowOverflow)
        {
            ATCtx->Error.Type = M68K_AET_VALUE_OUT_OF_RANGE;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }
        // force the sign bit?
        else if ((SValue & SMask) != 0)
            SValue |= MinSigned;
        // clear the sign bit
        else
            SValue &= MaxSigned;
    }

    // success
    *ParsedImmediate = SValue;
    return M68K_TRUE;
}

// parse a list of registers: 0|(<areg range>|<dreg range>)(/(<areg range>|<dreg range>))*, areg range = <areg>(-<areg>)?, dreg range = <dreg>(-<dreg>)?
static M68K_BOOL ParseList(PASM_TEXT_CTX ATCtx, PM68K_REGISTER_LIST ParsedRegisterList)
{
    // clear the list
    *ParsedRegisterList = 0;

    // list specified as a number?
    if (ATCtx->Error.Location[0] == '0')
    {
        ATCtx->Error.Location++;
        return M68K_TRUE;
    }

    // parse the list of registers
    do
    {
        M68K_UINT MaskBase;
        M68K_REGISTER_TYPE_VALUE Register1;
        M68K_REGISTER_TYPE_VALUE Register2;
        M68K_REGISTER_TYPE_VALUE MinRegister;
        PM68KC_STR Start = ATCtx->Error.Location;

        // parse the first register
        if (!ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register1))
            return M68K_FALSE;

        // must be D0-D7 or A0-A7
        if (RegisterIsValid(Register1, M68K_RT_D0, M68K_RT_D7))
        {
            MinRegister = M68K_RT_D0;
            MaskBase = 0;
        }
        else if (RegisterIsValid(Register1, M68K_RT_A0, M68K_RT_A7))
        {
            MinRegister = M68K_RT_A0;
            MaskBase = 8;
        }
        else
        {
            ATCtx->Error.Type = M68K_AET_REGISTER_NOT_SUPPORTED;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }

        M68K_REGISTER_LIST Mask = ((M68K_REGISTER_LIST)1 << (MaskBase + (Register1 - MinRegister)));

        // is it a range of registers?
        if (SkipChar(ATCtx, '-'))
        {
            if (!ParseRegister(ATCtx, MinRegister, MinRegister + 7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register2))
                return M68K_FALSE;

            // invalid range?
            if (Register1 > Register2)
            {
                ATCtx->Error.Type = M68K_AET_INVALID_RANGE_REGISTERS;
                ATCtx->Error.Location = Start;
                return M68K_FALSE;
            }

            M68K_REGISTER_LIST HMask = ((M68K_REGISTER_LIST)1 << (MaskBase + (Register2 - MinRegister)));
            Mask = HMask | ((HMask - 1) ^ (Mask - 1));
        }

        // already used?
        if ((*ParsedRegisterList & Mask) != 0)
        {
            ATCtx->Error.Type = M68K_AET_REGISTERS_ALREADY_IN_LIST;
            ATCtx->Error.Location = Start;
            return M68K_FALSE;
        }

        // update the list
        *ParsedRegisterList |= Mask;
    }
    while (SkipChar(ATCtx, '/'));

    return M68K_TRUE;
}

// parse a memory operand: <mem param>(,<mem param>)*, mem param = <addr> | <disp> | <base reg> | <index reg>\*[1248] | \[ <space>*<mem param><space>*(,<space>*<mem param><space>*)+ \]
static M68K_BOOL ParseMemory(PASM_TEXT_CTX ATCtx, PM68K_OPERAND ParsedOperand)
{
    // memory operands are specified using one or more parameters;
    // we'll parse all parameters and then decide which memory type is more adequate
    struct
    {
        MEMORY_PARAM_SCOPE  Scope;
        M68K_DWORD          Value;
    } Address =
    {
        MPS_NONE,
        0,
    };
    
    struct
    {
        MEMORY_PARAM_SCOPE          Scope;
        M68K_REGISTER_TYPE_VALUE    Register;
    } Base = 
    {
        MPS_NONE,
        M68K_RT_NONE,
    };

    struct
    {
        MEMORY_PARAM_SCOPE          Scope;
        M68K_REGISTER_TYPE_VALUE    Register;
        M68K_SIZE_VALUE             Size;
        M68K_SCALE_VALUE            Scale;
    } Index = {
        MPS_NONE,
        M68K_RT_NONE,
        M68K_SIZE_NONE,
        M68K_SCALE_1
    };

    struct
    {
        struct
        {
            MEMORY_PARAM_SCOPE  Scope;
            M68K_SDWORD         Value;
            M68K_SIZE_VALUE     Size;
        } ReadBase;                         // displacement used when reading the base address with []
        
        struct
        {
            MEMORY_PARAM_SCOPE  Scope;
            M68K_SDWORD         Value;
            M68K_SIZE_VALUE     Size;
            M68K_BOOL           ForcedSize;
        } Final;                            // displacement used when calculating the final memory address
    } Displacement =
    {
        {
            MPS_NONE,
            0,
            M68K_SIZE_NONE,
        },
        {
            MPS_NONE,
            0,
            M68K_SIZE_NONE,
            M68K_FALSE,
        },
    };

    M68K_BOOL AlreadyReadBase = M68K_FALSE;
    MEMORY_PARAM_SCOPE CurrentScope = MPS_FINAL;
    PM68KC_STR MemoryStart = ATCtx->Error.Location;
    M68K_CHAR Char = MemoryStart[0];

    while (Char != ')')
    {
        // get the next char and check it
        // &        address
        // [        read the base address
        // +,-,0-9  displacement
        // letter   register
        if (Char == '&')
        {
            // already specified?
            if (Address.Scope != MPS_NONE)
            {
                ATCtx->Error.Type = M68K_AET_MEMORY_ADDRESS_ALREADY_SPECIFIED;
                return M68K_FALSE;
            }

            if (!ParseAddr(ATCtx, &(Address.Value)))
                return M68K_FALSE;

            Address.Scope = CurrentScope;
        }
        else if (Char == '[' && !AlreadyReadBase)
        {
            // already reading the base address?
            if (CurrentScope != MPS_FINAL)
                goto unexpected_char;

            CurrentScope = MPS_READ_BASE;
            AlreadyReadBase = M68K_TRUE;
            goto check_next_operand_after_char;
        }
        else if (CharIsNumberStart(Char))
        {
            M68K_SDWORD SValue;
            M68K_BOOL ForcedSize;
            M68K_SIZE_VALUE Size;
            PM68KC_STR Start = ATCtx->Error.Location;

            if (!ParseImmediate(ATCtx, MIN_LONG, MAX_LONG, &SValue))
                return M68K_FALSE;

            // parse the size
            if (!ParseSize(ATCtx, M68K_FALSE, &Size))
            {
                ForcedSize = M68K_FALSE;
                Size = M68K_SIZE_NONE;
            }
            else
                ForcedSize = M68K_TRUE;

            if (!_M68KAsmTextCheckFixImmediateSize(SValue, &Size))
            {
                ATCtx->Error.Type = M68K_AET_VALUE_OUT_OF_RANGE;
                ATCtx->Error.Location = Start;
                return M68K_FALSE;
            }

            if (CurrentScope == MPS_READ_BASE)
            {
                // was it already defined?
                if (Displacement.ReadBase.Scope != MPS_NONE)
                {
                    ATCtx->Error.Type = M68K_AET_MEMORY_INNER_DISP_ALREADY_SPECIFIED;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                Displacement.ReadBase.Scope = CurrentScope;
                Displacement.ReadBase.Value = SValue;
                Displacement.ReadBase.Size = Size;
            }
            else
            {
                // was it already defined?
                if (Displacement.Final.Scope != MPS_NONE)
                {
                    ATCtx->Error.Type = M68K_AET_MEMORY_OUTER_DISP_ALREADY_SPECIFIED;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                Displacement.Final.Scope = CurrentScope;
                Displacement.Final.Value = SValue;
                Displacement.Final.Size = Size;
                Displacement.Final.ForcedSize = ForcedSize;
            }
        }
        else if (CharIsLetter(Char))
        {
            M68K_SIZE_VALUE Size;
            M68K_SCALE_VALUE Scale;
            M68K_REGISTER_TYPE_VALUE Register;
            PM68KC_STR Start = ATCtx->Error.Location;
            
            if (!ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &Register))
                return M68K_FALSE;
        
            // register might be a base register or an index register;
            // some chars can be used to force an index register: . (size) and * (scale)
            if (ParseSize(ATCtx, M68K_FALSE, &Size))
            {
                if (!ParseScale(ATCtx, &Scale))
                    return M68K_FALSE;
                
check_index_register:
                // index register; use the default scale?
                if (Scale == M68K_SCALE__SIZEOF__)
                    Scale = M68K_SCALE_1;

                // the index register was already specified?
                if (Index.Scope != MPS_NONE)
                {
                    ATCtx->Error.Type = M68K_AET_MEMORY_INDEX_ALREADY_SPECIFIED;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                if (!RegisterIsValid2(Register, M68K_RT_D0, M68K_RT_D7, M68K_RT_A0, M68K_RT_A7))
                {
                    ATCtx->Error.Type = M68K_AET_INVALID_REGISTER_MEMORY_INDEX;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                Index.Scope = CurrentScope;
                Index.Register = Register;
                Index.Size = Size;
                Index.Scale = Scale;
            }
            else 
            {
                if (ParseScale(ATCtx, &Scale))
                {
                    // we'll assume it's an index register if
                    // 1) scale is present OR
                    // 2) base register was already specified OR
                    // 3) base register is undefined but we found a data register
                    if (Scale != M68K_SCALE__SIZEOF__ || 
                        Base.Scope != MPS_NONE ||
                        RegisterIsValid(Register, M68K_RT_D0, M68K_RT_D7))
                    {
                        // the implicit size is long
                        Size = M68K_SIZE_L;
                        goto check_index_register;
                    }
                }

                // the base register was already specified?
                if (Base.Scope != MPS_NONE)
                {
                    ATCtx->Error.Type = M68K_AET_MEMORY_BASE_ALREADY_SPECIFIED;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                if (Register != M68K_RT_PC && 
                    Register != M68K_RT_ZPC && 
                    !RegisterIsValid2(Register, M68K_RT_A0, M68K_RT_A7, M68K_RT_B0, M68K_RT_B7))
                {
                    ATCtx->Error.Type = M68K_AET_INVALID_REGISTER_MEMORY_BASE;
                    ATCtx->Error.Location = Start;
                    return M68K_FALSE;
                }

                Base.Scope = CurrentScope;
                Base.Register = Register;
            }
        }
        else
            goto unexpected_char;

        SkipSpaces(ATCtx);

        // get the next char
        Char = ATCtx->Error.Location[0];

        // stop indexing?        
        if (Char == ']' && CurrentScope == MPS_READ_BASE)
        {
            CurrentScope = MPS_FINAL;

            // skip the next char and all spaces
            ATCtx->Error.Location++;
            SkipSpaces(ATCtx);

            // get the next char
            Char = ATCtx->Error.Location[0];
        }

        // more parameters?
        if (Char == ',')
        {
check_next_operand_after_char:
            // skip the next char and all spaces
            ATCtx->Error.Location++;
            SkipSpaces(ATCtx);
            
            // get the next char
            Char = ATCtx->Error.Location[0];
        }
        else if (Char == ')')
            break;
        else
        {
unexpected_char:
            ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
            return M68K_FALSE;
        }
    }

    // check the parameters to identity the memory type
    //                                          addr        base        index       rbdisp          fdisp(forced)       
    // M68K_OT_MEM_ABSOLUTE_L                   -           -           -           -               value.l(y)
    // M68K_OT_MEM_ABSOLUTE_W                   -           -           -           -               value.w(y)
    // M68K_OT_MEM_INDIRECT                     -           areg        -           -               -
    // M68K_OT_MEM_INDIRECT_DISP_W              -           areg        -           -               value.bw(n)
    // M68K_OT_MEM_INDIRECT_INDEX_DISP_B        -           areg        reg         -               value.b(n)?
    // M68K_OT_MEM_INDIRECT_INDEX_DISP_L        -           areg?       reg?        -               value.bwl(n)?
    // M68K_OT_MEM_INDIRECT_POST_INDEXED        -           [areg]?     reg?        [value.bwl]?    value.bwl(n)?
    // M68K_OT_MEM_INDIRECT_PRE_INDEXED         -           [areg]?     [reg]?      [value.bwl]?    value.bwl(n)?
    // M68K_OT_MEM_PC_INDIRECT_DISP_W           value.l     -           -           -               -
    //                                          -           pc          -           -               value.bw(n)
    // M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B     value.l     -           reg         -               -
    //                                          -           pc          reg         -               value.b(n)
    // M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L     value.l     -           reg?        -               -
    //                                          -           pc          reg?        -               value.wl(n)
    //                                          -           zpc         reg?        -               value.wl(n)?
    // M68K_OT_MEM_PC_INDIRECT_POST_INDEXED     [value.l]   -           reg?        -               value.wl(n)?
    //                                          -           [pc]        reg?        [value.bwl]?    value.wl(n)?
    //                                          -           [zpc]       reg?        [value.bwl]?    value.wl(n)?
    // M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED      [value.l]   -           [reg]?      -               value.wl(n)?
    //                                          -           [pc]        [reg]?      [value.bwl]?    value.wl(n)?
    //                                          -           [zpc]       [reg]?      [value.bwl]?    value.wl(n)?
    //
    ParsedOperand->Type = M68K_OT_NONE;

    if (Address.Scope == MPS_NONE)
    {
        if (Base.Scope == MPS_NONE)
        {
            if (Index.Scope == MPS_NONE)
            {
                if (Displacement.ReadBase.Scope == MPS_NONE)
                {
                    if (Displacement.Final.Scope == MPS_FINAL)
                    {
                        if (Displacement.Final.ForcedSize)
                        {
                            // addr = -, base = -, index = -, rbdisp = -, fdisp = value.wl(y) => M68K_OT_MEM_ABSOLUTE_L / M68K_OT_MEM_ABSOLUTE_W
                            ParsedOperand->Type = (Displacement.Final.Size == M68K_SIZE_W ? M68K_OT_MEM_ABSOLUTE_W : M68K_OT_MEM_ABSOLUTE_L);
                            ParsedOperand->Info.Long = Displacement.Final.Value;
                        }
                        else
                        {
                            // addr = -, base = -, index = -, rbdisp = -, fdisp = value.wl(n) => M68K_OT_MEM_INDIRECT_INDEX_DISP_L
                            ParsedOperand->Type = M68K_OT_MEM_INDIRECT_INDEX_DISP_L;
                            ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                            ParsedOperand->Info.Memory.Base = M68K_RT_NONE;
                            ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                            ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.Final.Size);
                            ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                            ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                        }
                    }
                }
                else if (Displacement.ReadBase.Scope == MPS_READ_BASE)
                {
                    // addr = -, base = -, index = -, rbdisp = [value.wl], fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_POST_INDEXED or M68K_OT_MEM_INDIRECT_PRE_INDEXED (preferred)
                    ParsedOperand->Type = M68K_OT_MEM_INDIRECT_PRE_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = M68K_RT_NONE;
                    ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
            }
            else if (Index.Scope == MPS_READ_BASE)
            {
                // addr = -, base = -, index = [reg], rbdisp = [value.bwl]?, fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_PRE_INDEXED
                ParsedOperand->Type = M68K_OT_MEM_INDIRECT_PRE_INDEXED;
                ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                ParsedOperand->Info.Memory.Base = M68K_RT_NONE;
                ParsedOperand->Info.Memory.Index.Register = Index.Register;
                ParsedOperand->Info.Memory.Index.Size = Index.Size;
                ParsedOperand->Info.Memory.Scale = Index.Scale;
                ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
            }
            else if (Index.Scope == MPS_FINAL)
            {
                if (Displacement.ReadBase.Scope == MPS_NONE)
                {
                    // addr = -, base = -, index = reg, rbdisp = -, fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_INDEX_DISP_L
                    ParsedOperand->Type = M68K_OT_MEM_INDIRECT_INDEX_DISP_L;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = M68K_RT_NONE;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = Displacement.Final.Size;
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                }
                else
                {
                    // addr = -, base = -, index = reg, rbdisp = [value.bwl], fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_POST_INDEXED
                    ParsedOperand->Type = M68K_OT_MEM_INDIRECT_POST_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = M68K_RT_NONE;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
            }
        }
        else if (Base.Scope == MPS_FINAL)
        {
            if (RegisterIsValid2(Base.Register, M68K_RT_A0, M68K_RT_A7, M68K_RT_B0, M68K_RT_B7))
            {
                if (Index.Scope == MPS_NONE && Displacement.ReadBase.Scope == MPS_NONE)
                {
                    if (Displacement.Final.Scope == MPS_NONE)
                    {
                        // addr = -, base = areg, index = -, rbdisp = -, fdisp = - => M68K_OT_MEM_INDIRECT
                        ParsedOperand->Type = M68K_OT_MEM_INDIRECT;
                        ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                        ParsedOperand->Info.Memory.Base = Base.Register;
                        ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                        ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
                        ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    }
                    else if (Displacement.Final.Size == M68K_SIZE_B || Displacement.Final.Size == M68K_SIZE_W)
                    {
                        // addr = -, base = areg, index = -, rbdisp = -, fdisp = valueb.w => M68K_OT_MEM_INDIRECT_DISP_W (preferred) or M68K_OT_MEM_INDIRECT_INDEX_DISP_L
                        ParsedOperand->Type = M68K_OT_MEM_INDIRECT_DISP_W;
                        ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                        ParsedOperand->Info.Memory.Base = Base.Register;
                        ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                        ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                        ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                        ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    }
                    else if (Displacement.Final.Size == M68K_SIZE_L)
                    {
                        // addr = -, base = areg, index = -, rbdisp = -, fdisp = value.l => M68K_OT_MEM_INDIRECT_INDEX_DISP_L
                        ParsedOperand->Type = M68K_OT_MEM_INDIRECT_INDEX_DISP_L;
                        ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                        ParsedOperand->Info.Memory.Base = Base.Register;
                        ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                        ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                        ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                        ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    }
                }
                else if (Index.Scope == MPS_FINAL && Displacement.ReadBase.Scope == MPS_NONE)
                {
                    // addr = -, base = areg, index = reg, rbdisp = -, fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_INDEX_DISP_B / M68K_OT_MEM_INDIRECT_INDEX_DISP_L
                    if (Displacement.Final.Scope == MPS_NONE)
                    {
                        // force "fdisp = 0" to use M68K_OT_MEM_INDIRECT_INDEX_DISP_B 
                        Displacement.Final.Scope = MPS_FINAL;
                        Displacement.Final.Size = M68K_SIZE_B;
                        Displacement.Final.Value = 0;
                    }

                    if (Displacement.Final.Scope == MPS_FINAL)
                    {
                        ParsedOperand->Type = (Displacement.Final.Size == M68K_SIZE_B ? M68K_OT_MEM_INDIRECT_INDEX_DISP_B : M68K_OT_MEM_INDIRECT_INDEX_DISP_L);
                        ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                        ParsedOperand->Info.Memory.Base = Base.Register;
                        ParsedOperand->Info.Memory.Index.Register = Index.Register;
                        ParsedOperand->Info.Memory.Index.Size = Index.Size;
                        ParsedOperand->Info.Memory.Scale = Index.Scale;
                        ParsedOperand->Info.Memory.Displacement.BaseSize = Displacement.Final.Size;
                        ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                        ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    }
                }
            }
            else if (Base.Register == M68K_RT_PC || Base.Register == M68K_RT_ZPC)
            {
                if (Index.Scope == MPS_NONE && Displacement.ReadBase.Scope == MPS_NONE)
                {
                    if (Displacement.Final.Scope == MPS_FINAL)
                    {
                        if (Base.Register == M68K_RT_PC && (Displacement.Final.Size == M68K_SIZE_B || Displacement.Final.Size == M68K_SIZE_W))
                        {
                            // addr = -, base = pc/zpc, index = -, rbdisp = -, fdisp = value.bw => M68K_OT_MEM_PC_INDIRECT_DISP_W
                            ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_DISP_W;
                            ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                            ParsedOperand->Info.Memory.Base = M68K_RT_PC;
                            ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                            ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                            ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                            ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                        }
                        else if (Base.Register == M68K_RT_ZPC || Displacement.Final.Size == M68K_SIZE_L)
                        {
                            // addr = -, base = pc/zpc, index = -, rbdisp = -, fdisp = value.l => M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L
                            ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L;
                            ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                            ParsedOperand->Info.Memory.Base = Base.Register;
                            ParsedOperand->Info.Memory.Index.Register = Index.Register;
                            ParsedOperand->Info.Memory.Index.Size = Index.Size;
                            ParsedOperand->Info.Memory.Scale = Index.Scale;
                            ParsedOperand->Info.Memory.Displacement.BaseSize = Displacement.Final.Size;
                            ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                            ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                        }
                    }
                }
                else if (Index.Scope == MPS_FINAL && Displacement.ReadBase.Scope == MPS_NONE)
                {
                    // addr = -, base = pc/zpc, index = reg, rbdisp = -, fdisp = value.bwl? => M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B / M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L
                    if (Displacement.Final.Scope == MPS_NONE)
                    {
                        // force "fdisp = 0" to use M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B 
                        Displacement.Final.Scope = MPS_FINAL;
                        Displacement.Final.Size = M68K_SIZE_B;
                        Displacement.Final.Value = 0;
                    }

                    if (Displacement.Final.Scope == MPS_FINAL)
                    {
                        ParsedOperand->Type = (Displacement.Final.Size == M68K_SIZE_B ? M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B : M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L);
                        ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                        ParsedOperand->Info.Memory.Base = Base.Register;
                        ParsedOperand->Info.Memory.Index.Register = Index.Register;
                        ParsedOperand->Info.Memory.Index.Size = Index.Size;
                        ParsedOperand->Info.Memory.Scale = Index.Scale;
                        ParsedOperand->Info.Memory.Displacement.BaseSize = Displacement.Final.Size;
                        ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.Final.Value;
                        ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                    }
                }
            }
        }
        else if (Base.Scope == MPS_READ_BASE)
        {
            if (RegisterIsValid2(Base.Register, M68K_RT_A0, M68K_RT_A7, M68K_RT_B0, M68K_RT_B7))
            {
                if (Index.Scope == MPS_NONE || Index.Scope == MPS_READ_BASE)
                {
                    // addr = -, base = [areg], index = [reg]?, rbdisp = [value.bwl]?, fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_PRE_INDEXED
                    ParsedOperand->Type = M68K_OT_MEM_INDIRECT_PRE_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = Base.Register;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
                else
                {
                    // addr = -, base = [areg], index = reg, rbdisp = [value.bwl]?, fdisp = value.bwl? => M68K_OT_MEM_INDIRECT_POST_INDEXED
                    ParsedOperand->Type = M68K_OT_MEM_INDIRECT_POST_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = Base.Register;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
            }
            else if (Base.Register == M68K_RT_PC || Base.Register == M68K_RT_ZPC)
            {
                if (Index.Scope == MPS_NONE || Index.Scope == MPS_READ_BASE)
                {
                    // addr = -, base = [pc/zpc], index = [reg], rbdisp = [value.bwl]?, fdisp = value.bwl? => M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED
                    ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = Base.Register;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
                else
                {
                    // addr = -, base = [pc/zpc], index = reg?, rbdisp = [value.bwl]?, fdisp = value.bwl? => M68K_OT_MEM_PC_INDIRECT_POST_INDEXED
                    ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_POST_INDEXED;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = Base.Register;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = AdjustSizeToWord(Displacement.ReadBase.Size);
                    ParsedOperand->Info.Memory.Displacement.BaseValue = Displacement.ReadBase.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                    ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
                }
            }
        }
    }
    // using a memory address i.e. pc relative which means the base register and displacement can't be used
    else if (Base.Scope == MPS_NONE && Displacement.ReadBase.Scope == MPS_NONE)
    {
        if (Address.Scope == MPS_FINAL)
        {
            if (Index.Scope == MPS_NONE)
            {
                if (Displacement.Final.Scope == MPS_NONE)
                {
                    // addr = value.l, base = -, index = -, rbdisp = -, fdisp = - => M68K_OT_MEM_PC_INDIRECT_DISP_W or M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L;
                    // defining the size as M68K_SIZE_L (in theory an invalid value for M68K_OT_MEM_PC_INDIRECT_DISP_W) will force function "M68KAssemble" to choose between the two
                    ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_DISP_W;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = M68K_RT__RPC;
                    ParsedOperand->Info.Memory.Index.Register = M68K_RT_NONE;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                    ParsedOperand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)Address.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                }
            }
            else if (Index.Scope == MPS_FINAL)
            {
                if (Displacement.Final.Scope == MPS_NONE)
                {
                    // addr = value.l, base = -, index = reg, rbdisp = -, fdisp = - => M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B or M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L;
                    // defining the size as M68K_SIZE_L (in theory an invalid value for M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B) will force function "M68KAssemble" to choose between the two
                    ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B;
                    ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                    ParsedOperand->Info.Memory.Base = M68K_RT__RPC;
                    ParsedOperand->Info.Memory.Index.Register = Index.Register;
                    ParsedOperand->Info.Memory.Index.Size = Index.Size;
                    ParsedOperand->Info.Memory.Scale = Index.Scale;
                    ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                    ParsedOperand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)Address.Value;
                    ParsedOperand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                }
            }
        }
        else
        {
            if (Index.Scope == MPS_NONE || Index.Scope == MPS_READ_BASE)
            {
                // addr = [value.l], base = -, index = [reg]?, rbdisp = -, fdisp = value.bwl? => M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED;
                // we can define the size as M68K_SIZE_L because function "M68KAssemble" will choose the best size
                ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED;
                ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                ParsedOperand->Info.Memory.Base = M68K_RT__RPC;
                ParsedOperand->Info.Memory.Index.Register = Index.Register;
                ParsedOperand->Info.Memory.Index.Size = Index.Size;
                ParsedOperand->Info.Memory.Scale = Index.Scale;
                ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                ParsedOperand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)Address.Value;
                ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
            }
            else
            {
                // addr = [value.l], base = -, index = reg, rbdisp = -, fdisp = value.bwl? => M68K_OT_MEM_PC_INDIRECT_POST_INDEXED;
                // we can define the size as M68K_SIZE_L because function "M68KAssemble" will choose the best size
                ParsedOperand->Type = M68K_OT_MEM_PC_INDIRECT_POST_INDEXED;
                ParsedOperand->Info.Memory.Increment = M68K_INC_NONE;
                ParsedOperand->Info.Memory.Base = M68K_RT__RPC;
                ParsedOperand->Info.Memory.Index.Register = Index.Register;
                ParsedOperand->Info.Memory.Index.Size = Index.Size;
                ParsedOperand->Info.Memory.Scale = Index.Scale;
                ParsedOperand->Info.Memory.Displacement.BaseSize = M68K_SIZE_L;
                ParsedOperand->Info.Memory.Displacement.BaseValue = (M68K_SDWORD)Address.Value;
                ParsedOperand->Info.Memory.Displacement.OuterSize = AdjustSizeToWord(Displacement.Final.Size);
                ParsedOperand->Info.Memory.Displacement.OuterValue = Displacement.Final.Value;
            }
        }
    }
    
    // valid?
    if (ParsedOperand->Type == M68K_OT_NONE)
    {
        ATCtx->Error.Type = M68K_AET_INVALID_MEMORY_OPERAND;
        ATCtx->Error.Location = MemoryStart;
        return M68K_FALSE;
    }

    return M68K_TRUE;
}

// parse a mnemonic; [a-z][0-9a-z]*
static M68K_BOOL ParseMnemonic(PASM_TEXT_CTX ATCtx, PM68K_INSTRUCTION_TYPE_VALUE ParsedIType, PM68K_OPERAND *Operand)
{
    PM68KC_CHAR Start = ATCtx->Error.Location;

    if (ParseIdentifier(ATCtx, M68K_TRUE))
    {
        PM68K_OPERAND FirstOperand = (Operand != NULL ? *Operand : M68K_NULL);

        // search in the table of mnemonics
        M68K_INSTRUCTION_TYPE_VALUE iType = _M68KAsmTextCheckMnemonic(Start, ATCtx->Error.Location, FirstOperand);
        if (iType != M68K_IT_INVALID)
        {
            // requires an implicit operand?
            if (FirstOperand != NULL)
            {
                // was it used?
                if (FirstOperand->Type != M68K_OT_NONE)
                    // skip it
                    (*Operand)++;
            }

            // the mnemonic is valid
            *ParsedIType = iType;
            return M68K_TRUE;
        }

        ATCtx->Error.Type = M68K_AET_INVALID_MNEMONIC;
        ATCtx->Error.Location = Start;
    }

    return M68K_FALSE;
}

// parse an offset and width: <imm:0..31>|<dreg>,<imm:1..32>|<dreg>
static M68K_BOOL ParseOffsetWidth(PASM_TEXT_CTX ATCtx, PM68K_OFFSET_WIDTH ParsedOffsetWidth)
{
    // offset can be an immediate value (0-31) or a data register
    M68K_BOOL Result;
    M68K_SDWORD OffsetSValue;
    M68K_REGISTER_TYPE_VALUE OffsetRegister;
    M68K_CHAR Char = ATCtx->Error.Location[0];

    if (CharIsNumberStart(Char))
    {
        // parse as an immediate
        Result = ParseImmediate(ATCtx, 0L, 31L, &OffsetSValue);
        OffsetRegister = M68K_RT_NONE;
    }
    else
    {
        // parse as a data register
        OffsetSValue = 0;
        Result = ParseRegister(ATCtx, M68K_RT_D0, M68K_RT_D7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &OffsetRegister);
    }

    if (Result)
    {
        // parse the value separator
        if (ParseValueSeparator(ATCtx))
        {
            // width can be an immediate value (1-32) or a data register
            M68K_SDWORD WidthSValue;
            M68K_REGISTER_TYPE_VALUE WidthRegister;

            Char = ATCtx->Error.Location[0];

            if (CharIsNumberStart(Char))
            {
                // parse as an immediate
                Result = ParseImmediate(ATCtx, 1L, 32L, &WidthSValue);
                WidthRegister = M68K_RT_NONE;
            }
            else
            {
                // parse as a data register
                WidthSValue = 0;
                Result = ParseRegister(ATCtx, M68K_RT_D0, M68K_RT_D7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &WidthRegister);
            }
        
            if (Result)
            {
                if (OffsetRegister != M68K_RT_NONE)
                    ParsedOffsetWidth->Offset = OffsetRegister;
                else
                    ParsedOffsetWidth->Offset = (M68K_REGISTER_TYPE_VALUE)(-OffsetSValue - 1);

                if (WidthRegister != M68K_RT_NONE)
                    ParsedOffsetWidth->Width = WidthRegister;
                else
                    ParsedOffsetWidth->Width = (M68K_REGISTER_TYPE_VALUE)(-WidthSValue - 1);

                return M68K_TRUE;
            }        
        }
    }
    
    return M68K_FALSE;
}

// parse an operand: %<type>\(<space>*[value]<space>*(,<space>*[value]<space>*)*\)
static M68K_BOOL ParseOperand(PASM_TEXT_CTX ATCtx, PM68K_OPERAND Operand)
{
    if (ParseChar(ATCtx, '%'))
    {
        PM68KC_CHAR Start = ATCtx->Error.Location;

        if (ParseIdentifier(ATCtx, M68K_TRUE))
        {
            PM68KC_CHAR End = ATCtx->Error.Location;

            // start the operand value?
            if (ParseChar(ATCtx, '('))
            {
                // skip all spaces
                SkipSpaces(ATCtx);
                    
                // set the default error type
                ATCtx->Error.Type = M68K_AET_INVALID_OPERAND_TYPE;

                M68K_UINT Index = _M68KAsmTextBinarySearchText(_M68KTextAsmOperandTypes, 1, ATOT__SIZEOF__ - 1, Start, End);
                if (Index < ATOT__SIZEOF__)
                {
                    M68K_SDWORD SValue;
                    M68K_SDWORD MinSValue;
                    M68K_SDWORD MaxSValue;
                    M68K_OPERAND_TYPE OType;
                    M68K_BOOL ValidOperand = M68K_FALSE;

                    switch ((ASM_TEXT_OPERAND_TYPE)Index)
                    {
                    case ATOT_ADDRESS_B:
                        Start = ATCtx->Error.Location;
                        if (ParseAddr(ATCtx, &(Operand->Info.Long)))
                        {
                            Operand->Type = M68K_OT_ADDRESS_B;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_ADDRESS_L:
                        Start = ATCtx->Error.Location;
                        if (ParseAddr(ATCtx, &(Operand->Info.Long)))
                        {
                            Operand->Type = M68K_OT_ADDRESS_L;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_ADDRESS_W:
                        Start = ATCtx->Error.Location;
                        if (ParseAddr(ATCtx, &(Operand->Info.Long)))
                        {
                            Operand->Type = M68K_OT_ADDRESS_W;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_CONDITION_CODE:
                        Start = ATCtx->Error.Location;
                        if (ParseIdentifier(ATCtx, M68K_FALSE))
                        {
                            Index = LinearSearchText(_M68KTextConditionCodes, 0, M68K_CC__SIZEOF__ - 1, Start, ATCtx->Error.Location);
                            if (Index < M68K_CC__SIZEOF__)
                            {
                                Operand->Type = M68K_OT_CONDITION_CODE;
                                Operand->Info.ConditionCode = (M68K_CONDITION_CODE_VALUE)Index;
                                ValidOperand = M68K_TRUE;
                            }
                            else
                            {
unknown_condition_code:
                                ATCtx->Error.Type = M68K_AET_INVALID_CONDITION_CODE;
                                ATCtx->Error.Location = Start;
                            }
                        }
                        break;

                    case ATOT_COPROCESSOR_ID:
                        if (ParseImmediate(ATCtx, 0L, 7L, &SValue))
                        {
                            Operand->Type = M68K_OT_COPROCESSOR_ID;
                            Operand->Info.CoprocessorId = (M68K_COPROCESSOR_ID_VALUE)SValue;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_COPROCESSOR_ID_CONDITION_CODE:
                        if (ParseImmediate(ATCtx, 0L, 7L, &SValue))
                        {
                            Operand->Info.CoprocessorIdCC.Id = (M68K_COPROCESSOR_ID_VALUE)SValue;

                            if (ParseValueSeparator(ATCtx))
                            {
                                if (ParseImmediate(ATCtx, 0L, 63L, &SValue))
                                {
                                    Operand->Type = M68K_OT_COPROCESSOR_ID_CONDITION_CODE;
                                    Operand->Info.CoprocessorIdCC.CC = (M68K_COPROCESSOR_CONDITION_CODE_VALUE)SValue;
                                    ValidOperand = M68K_TRUE;
                                }
                            }
                        }
                        break;

                    case ATOT_CACHE_TYPE:
                        Start = ATCtx->Error.Location;
                        if (ParseIdentifier(ATCtx, M68K_FALSE))
                        {
                            Index = LinearSearchText(_M68KTextCacheTypes, 0, M68K_CT__SIZEOF__ - 1, Start, ATCtx->Error.Location);
                            if (Index < M68K_CT__SIZEOF__)
                            {
                                Operand->Type = M68K_OT_CACHE_TYPE;
                                Operand->Info.CacheType = (M68K_CACHE_TYPE_VALUE)Index;
                                ValidOperand = M68K_TRUE;
                            }
                            else
                            {
                                ATCtx->Error.Type = M68K_AET_INVALID_CACHE_TYPE;
                                ATCtx->Error.Location = Start;
                            }
                        }
                        break;

                    case ATOT_DISPLACEMENT_B:
                        MinSValue = MIN_BYTE;
                        MaxSValue = MAX_BYTE;
                        OType = M68K_OT_DISPLACEMENT_B;

check_disp:
                        if (ParseImmediate(ATCtx, MinSValue, MaxSValue, &(Operand->Info.Displacement)))
                        {
                            Operand->Type = OType;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_DYNAMIC_KFACTOR:
                        if (ParseRegister(ATCtx, M68K_RT_D0, M68K_RT_D7, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &(Operand->Info.Register)))
                        {
                            Operand->Type = M68K_OT_DYNAMIC_KFACTOR;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_DISPLACEMENT_L:
                        MinSValue = MIN_LONG;
                        MaxSValue = MAX_LONG;
                        OType = M68K_OT_DISPLACEMENT_L;
                        goto check_disp;

                    case ATOT_DISPLACEMENT_W:
                        MinSValue = MIN_WORD;
                        MaxSValue = MAX_WORD;
                        OType = M68K_OT_DISPLACEMENT_W;
                        goto check_disp;

                    case ATOT_FPU_CONDITION_CODE:
                        Start = ATCtx->Error.Location;
                        if (ParseIdentifier(ATCtx, M68K_FALSE))
                        {
                            Index = LinearSearchText(_M68KTextFpuConditionCodes, 0, M68K_FPCC__SIZEOF__ - 1, Start, ATCtx->Error.Location);
                            if (Index < M68K_FPCC__SIZEOF__)
                            {
                                Operand->Type = M68K_OT_FPU_CONDITION_CODE;
                                Operand->Info.FpuConditionCode = (M68K_FPU_CONDITION_CODE_VALUE)Index;
                                ValidOperand = M68K_TRUE;
                            }
                            else
                                goto unknown_condition_code;
                        }
                        break;

                    case ATOT_FPU_CONTROL_REGISTER_LIST:
                        if (ParseFPCRList(ATCtx, &(Operand->Info.RegisterList)))
                        {
                            Operand->Type = M68K_OT_REGISTER_FPCR_LIST;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_FPU_REGISTER_LIST:
                        if (ParseFPList(ATCtx, &(Operand->Info.RegisterList)))
                        {
                            Operand->Type = M68K_OT_REGISTER_FP_LIST;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_B:
                        if (ParseImmediate(ATCtx, MIN_BYTE, MAX_BYTE, &SValue))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_B;
                            Operand->Info.Byte = (M68K_BYTE)(M68K_SBYTE)SValue;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_D:
                        if (ParseDouble(ATCtx, &(Operand->Info.Double)))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_D;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_L:
                        if (ParseImmediate(ATCtx, MIN_LONG, MAX_LONG, &SValue))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_L;
                            Operand->Info.Long = (M68K_DWORD)SValue;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_P:
                        if (ParsePackedDecimal(ATCtx, &(Operand->Info.PackedDecimal)))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_P;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_Q:
                        if (ParseQuad(ATCtx, &(Operand->Info.Quad)))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_Q;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_S:
                        if (ParseSingle(ATCtx, &(Operand->Info.Single)))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_S;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_W:
                        if (ParseImmediate(ATCtx, MIN_WORD, MAX_WORD, &SValue))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_W;
                            Operand->Info.Word = (M68K_WORD)(M68K_SWORD)SValue;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_IMMEDIATE_X:
                        if (ParseExtended(ATCtx, &(Operand->Info.Extended)))
                        {
                            Operand->Type = M68K_OT_IMMEDIATE_X;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_MEMORY:
                        ValidOperand = ParseMemory(ATCtx, Operand);
                        break;

                    case ATOT_MMU_CONDITION_CODE:
                        Start = ATCtx->Error.Location;
                        if (ParseIdentifier(ATCtx, M68K_FALSE))
                        {
                            Index = LinearSearchText(_M68KTextMMUConditionCodes, 0, M68K_MMUCC__SIZEOF__ - 1, Start, ATCtx->Error.Location);
                            if (Index <= 15)
                            {
                                Operand->Type = M68K_OT_MMU_CONDITION_CODE;
                                Operand->Info.MMUConditionCode = (M68K_MMU_CONDITION_CODE_VALUE)Index;
                                ValidOperand = M68K_TRUE;
                            }
                            else
                                goto unknown_condition_code;
                        }
                        break;

                    case ATOT_MEMORY_PRE_DECREMENT:
                        if (ParseRegister(ATCtx, M68K_RT_A0, M68K_RT_A7, M68K_RT_B0, M68K_RT_B7, &(Operand->Info.Memory.Base)))
                        {
                            Operand->Type = M68K_OT_MEM_INDIRECT_PRE_DECREMENT;
                            Operand->Info.Memory.Increment = M68K_INC_PRE_DECREMENT;
                            Operand->Info.Memory.Index.Register = M68K_RT_NONE;
                            Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
                            Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;

                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_MEMORY_POST_INCREMENT:
                        if (ParseRegister(ATCtx, M68K_RT_A0, M68K_RT_A7, M68K_RT_B0, M68K_RT_B7, &(Operand->Info.Memory.Base)))
                        {
                            Operand->Type = M68K_OT_MEM_INDIRECT_POST_INCREMENT;
                            Operand->Info.Memory.Increment = M68K_INC_POST_INCREMENT;
                            Operand->Info.Memory.Index.Register = M68K_RT_NONE;
                            Operand->Info.Memory.Displacement.BaseSize = M68K_SIZE_NONE;
                            Operand->Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;

                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_MEMORY_PAIR:
                        OType = M68K_OT_MEM_REGISTER_PAIR;

register_pair:
                        Start = ATCtx->Error.Location;
                        if (ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &(Operand->Info.RegisterPair.Register1)))
                        {
                            // vregister?
                            if (OType == M68K_OT_VREGISTER_PAIR_M2 || OType == M68K_OT_VREGISTER_PAIR_M4)
                            {
                                // allowed between d0-d7 and e0-e23
                                M68K_UINT RegIndex;

                                if (RegisterIsValid(Operand->Info.RegisterPair.Register1, M68K_RT_D0, M68K_RT_D7))
                                    RegIndex = Operand->Info.RegisterPair.Register1 - M68K_RT_D0;
                                else if (RegisterIsValid(Operand->Info.RegisterPair.Register1, M68K_RT_E0, M68K_RT_E23))
                                    RegIndex = Operand->Info.RegisterPair.Register1 - M68K_RT_E0;
                                else
                                    // use an odd value
                                    RegIndex = 1;

                                ValidOperand = (
                                    // multiple of 2?
                                    (OType == M68K_OT_VREGISTER_PAIR_M2 && (RegIndex & 1) == 0) ||
                                    // multiple of 4?
                                    (OType == M68K_OT_VREGISTER_PAIR_M4 && (RegIndex & 3) == 0)
                                );
                            }
                            else
                            {
                                // is it a data register?
                                if (RegisterIsValid(Operand->Info.RegisterPair.Register1, M68K_RT_D0, M68K_RT_D7))
                                    ValidOperand = M68K_TRUE;

                                // address register is allowed?
                                else if (OType == M68K_OT_MEM_REGISTER_PAIR)
                                    ValidOperand = RegisterIsValid(Operand->Info.RegisterPair.Register1, M68K_RT_A0, M68K_RT_A7);
                            }

                            if (ValidOperand)
                            {
                                // check the next register
                                ValidOperand = M68K_FALSE;
                                
                                if (ParseValueSeparator(ATCtx))
                                {
                                    Start = ATCtx->Error.Location;
                                    if (ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &(Operand->Info.RegisterPair.Register2)))
                                    {
                                        // vregister?
                                        if (OType == M68K_OT_VREGISTER_PAIR_M2 || OType == M68K_OT_VREGISTER_PAIR_M4)
                                        {
                                            ValidOperand = (
                                                // 2 registers?
                                                (OType == M68K_OT_VREGISTER_PAIR_M2 && (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 1) == Operand->Info.RegisterPair.Register2) ||
                                                // 4 registers?
                                                (OType == M68K_OT_VREGISTER_PAIR_M4 && (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 3) == Operand->Info.RegisterPair.Register2)
                                            );
                                        }
                                        else
                                        {
                                            // is it a data register?
                                            if (RegisterIsValid(Operand->Info.RegisterPair.Register2, M68K_RT_D0, M68K_RT_D7))
                                                ValidOperand = M68K_TRUE;

                                            // address register is allowed?
                                            else if (OType == M68K_OT_MEM_REGISTER_PAIR)
                                                ValidOperand = RegisterIsValid(Operand->Info.RegisterPair.Register2, M68K_RT_A0, M68K_RT_A7);
                                        }

                                        if (ValidOperand)
                                        {
                                            Operand->Type = OType;
                                            break;
                                        }

                                        goto unsupported_register;
                                    }
                                }
                            }
                            else
                            {
unsupported_register:
                                ATCtx->Error.Type = M68K_AET_REGISTER_NOT_SUPPORTED;
                                ATCtx->Error.Location = Start;
                            }
                        }
                        break;

                    case ATOT_OFFSET_WIDTH:
                        if (ParseOffsetWidth(ATCtx, &(Operand->Info.OffsetWidth)))
                        {
                            Operand->Type = M68K_OT_OFFSET_WIDTH;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_REGISTER:
                        if (ParseRegister(ATCtx, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, M68K_RT__SIZEOF__, &(Operand->Info.Register)))
                        {
                            Operand->Type = M68K_OT_REGISTER;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_REGISTER_LIST:
                        if (ParseList(ATCtx, &(Operand->Info.RegisterList)))
                        {
                            Operand->Type = M68K_OT_REGISTER_LIST;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_REGISTER_PAIR:
                        OType = M68K_OT_REGISTER_PAIR;
                        goto register_pair;

                    case ATOT_VREGISTER_PAIR_M2:
                        OType = M68K_OT_VREGISTER_PAIR_M2;
                        goto register_pair;

                    case ATOT_VREGISTER_PAIR_M4:
                        OType = M68K_OT_VREGISTER_PAIR_M4;
                        goto register_pair;

                    case ATOT_STATIC_KFACTOR:
                        if (ParseImmediate(ATCtx, -63L, 64L, &SValue))
                        {
                            Operand->Type = M68K_OT_STATIC_KFACTOR;
                            Operand->Info.SByte = (M68K_SBYTE)SValue;
                            ValidOperand = M68K_TRUE;
                        }
                        break;

                    case ATOT_UNKNOWN:
                    default:
                        ATCtx->Error.Type = M68K_AET_OPERAND_TYPE_NOT_SUPPORTED;
                        break;
                    }

                    // can we continue?
                    if (ValidOperand)
                    {
                        // skip all spaces
                        SkipSpaces(ATCtx);

                        // end the operand?
                        if (ParseChar(ATCtx, ')'))
                            return M68K_TRUE;
                    }
                }
            }
        }
    }
    
    return M68K_FALSE;
}

// parse a packed decimal immediate value: 0x[0-9a-f]{1,24}
static M68K_BOOL ParsePackedDecimal(PASM_TEXT_CTX ATCtx, PM68K_PACKED_DECIMAL ParsedPackedDecimal)
{
    if (ParseChar(ATCtx, '0'))
    {
        if (ParseChar(ATCtx, 'x'))
        {
            M68K_DWORD ParsedImmediates[3];

            if (ParseHexDigits(ATCtx, 24, ParsedImmediates))
            {
                (*ParsedPackedDecimal)[0] = ParsedImmediates[2];
                (*ParsedPackedDecimal)[1] = ParsedImmediates[1];
                (*ParsedPackedDecimal)[2] = ParsedImmediates[0];
                return M68K_TRUE;
            }
        }
    }

    return M68K_FALSE;
}

// parse a quad immediate value: 0x[0-9a-f]{1,16}
static M68K_BOOL ParseQuad(PASM_TEXT_CTX ATCtx, PM68K_QUAD ParsedQuad)
{
    if (ParseChar(ATCtx, '0'))
    {
        if (ParseChar(ATCtx, 'x'))
        {
            M68K_DWORD ParsedImmediates[2];

            if (ParseHexDigits(ATCtx, 16, ParsedImmediates))
            {
                (*ParsedQuad)[0] = ParsedImmediates[1];
                (*ParsedQuad)[1] = ParsedImmediates[0];
                return M68K_TRUE;
            }
        }
    }

    return M68K_FALSE;
}

// parse a register: [a-z][0-9a-z]*
static M68K_BOOL ParseRegister(PASM_TEXT_CTX ATCtx, M68K_REGISTER_TYPE_VALUE MinRegister1, M68K_REGISTER_TYPE_VALUE MaxRegister1, M68K_REGISTER_TYPE_VALUE MinRegister2, M68K_REGISTER_TYPE_VALUE MaxRegister2, PM68K_REGISTER_TYPE_VALUE ParsedRegister)
{
    PM68KC_STR Start = ATCtx->Error.Location;
    
    if (ParseIdentifier(ATCtx, M68K_TRUE))
    {
        // search in the table of registers
        M68K_REGISTER_TYPE_VALUE Register = _M68KAsmTextBinarySearchRegister(Start, ATCtx->Error.Location);
        if (Register < M68K_RT__SIZEOF__)
        {
            if (MinRegister1 == M68K_RT__SIZEOF__)
            {
                MinRegister1 = Register;
                MaxRegister1 = Register;
            }

            if (MinRegister2 == M68K_RT__SIZEOF__)
            {
                MinRegister2 = Register;
                MaxRegister2 = Register;
            }

            // check the ranges
            if (RegisterIsValid2(Register, MinRegister1, MaxRegister1, MinRegister2, MaxRegister2))
            {
                // the register is valid
                *ParsedRegister = Register;
                return M68K_TRUE;
            }
            else
                ATCtx->Error.Type = M68K_AET_REGISTER_NOT_SUPPORTED;
        }
        else
            ATCtx->Error.Type = M68K_AET_INVALID_REGISTER;

        ATCtx->Error.Location = Start;
    }

    return M68K_FALSE;
}

// parse a scale: \*[1248]
static M68K_BOOL ParseScale(PASM_TEXT_CTX ATCtx, PM68K_SCALE_VALUE ParsedScale)
{
    if (ATCtx->Error.Location[0] == '*')
    {
        M68K_CHAR Char = (++ATCtx->Error.Location)[0];

        if (Char == '1')
            *ParsedScale = M68K_SCALE_1;
        else if (Char == '2')
            *ParsedScale = M68K_SCALE_2;
        else if (Char == '4')
            *ParsedScale = M68K_SCALE_4;
        else if (Char == '8')
            *ParsedScale = M68K_SCALE_8;
        else
        {
            ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
            return M68K_FALSE;
        }

        // skip the multiplication factor
        ATCtx->Error.Location++;
    }
    else
        *ParsedScale = M68K_SCALE__SIZEOF__;
    
    return M68K_TRUE;
}

// parse a single immediate value: [+-]?(inf|nan|0(x[01]\.[0-9a-f]{1,6}p[+-]?[0-9]{1,3})?)
static M68K_BOOL ParseSingle(PASM_TEXT_CTX ATCtx, PM68K_SINGLE ParsedSingle)
{
    M68K_DWORD ParsedImmediates[1];

    // start by skipping the sign
    M68K_BOOL NegativeValue;
    SkipNumberSign(ATCtx, &NegativeValue);

    // is it a number?
    PM68KC_STR Start = ATCtx->Error.Location;
    
    if (Start[0] == '0')
    {
        // hexadecimal notation?
        if (Start[1] == 'x')
        {
            // skip the "0x"
            Start = (ATCtx->Error.Location += 2);

            // the next char must be a 0 or 1
            M68K_BOOL Implicit1 = (Start[0] == '1');
            if (Implicit1 || Start[0] == '0')
            {
                // skip the implicit 0 or 1 and the .
                ATCtx->Error.Location++;
                if (ParseChar(ATCtx, '.'))
                {
                    // save the location of the first hexadecimal digit
                    Start = ATCtx->Error.Location;
                    if (ParseHexDigits(ATCtx, 6, ParsedImmediates))
                    {
                        // determine the number of digits that were parsed
                        M68K_UINT NumberDigits = (M68K_UINT)(ATCtx->Error.Location - Start);

                        if (ParseChar(ATCtx, 'p'))
                        {
                            Start = ATCtx->Error.Location;

                            // skip the sign
                            M68K_BOOL NegativeExponent;
                            SkipNumberSign(ATCtx, &NegativeExponent);

                            M68K_DWORD UExponent;
                            if (ParseDecDigits(ATCtx, 3, &UExponent))
                            {
                                M68K_SDWORD SExponent = (NegativeExponent ? -(M68K_SDWORD)UExponent : (M68K_SDWORD)UExponent);

                                // check if the exponent is valid; 
                                // normalized numbers can have an exponent between -126 and 127;
                                // denormalized number always have an exponent of -126
                                if ((Implicit1 && SExponent >= -126 && SExponent <= 127) || (!Implicit1 && SExponent == -126))
                                {
                                    // shift the mantissa bits?
                                    if (NumberDigits < 6)
                                        ParsedImmediates[0] <<= ((6 - NumberDigits) << 2);
                                    
                                    // the mantissa only uses 23 bits so we must shift left one bit
                                    ParsedImmediates[0] >>= 1;

                                    // add the biased exponent?
                                    if (Implicit1)
                                        ParsedImmediates[0] |= (M68K_DWORD)((SExponent + 127) & 0xff) << 23;
                                
                                    // convert to single
                                    *ParsedSingle = *(PM68K_SINGLE)ParsedImmediates;

check_value_sign:
                                    if (NegativeValue)
                                        *ParsedSingle = -*ParsedSingle;

                                    return M68K_TRUE;
                                }
                                else
                                {
                                    ATCtx->Error.Type = M68K_AET_INVALID_EXPONENT;
                                    ATCtx->Error.Location = Start;
                                }
                            }
                        } 
                    }
                }
            }
            else
                ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;
        }
        else
        {
            // parsed as zero
            ATCtx->Error.Location++;
            *ParsedSingle = 0;
            goto check_value_sign;
        }
    }
    else
    {
        // must be an ieee value
        M68K_IEEE_VALUE_TYPE IEEEValue;
        if (ParseIEEEValue(ATCtx, &IEEEValue))
        {
            // only a few ieee values are allowed: inf|nan
            if (IEEEValue == M68K_IEEE_VT_INF || IEEEValue == M68K_IEEE_VT_NAN)
            {
                *ParsedSingle = *(PM68K_SINGLE)(_M68KIEEESingleValues + IEEEValue);
                goto check_value_sign;
            }
            else
                ATCtx->Error.Type = M68K_AET_IEEE_VALUE_NOT_SUPPORTED;
        }
    }
    
    return M68K_FALSE;
}

// parse a size: \.[bdlpqswx]
static M68K_BOOL ParseSize(PASM_TEXT_CTX ATCtx, M68K_BOOL SetErrorType, PM68K_SIZE_VALUE ParsedSize)
{
    // get the first char
    PM68KC_CHAR Start = ATCtx->Error.Location;
    M68K_CHAR Char = *Start;

    if (Char == '.')
    {
        // check the next char
        PM68KC_CHAR Next = Start + 1;
        M68K_SIZE Size = M68KGetSizeFromChar(*Next);

        if (Size != M68K_SIZE_NONE)
        {
            // the size is valid
            *ParsedSize = (M68K_SIZE_VALUE)Size;
            
            // save the next location
            ATCtx->Error.Location = Next + 1;
            return M68K_TRUE;
        }

        if (SetErrorType)
            ATCtx->Error.Type = M68K_AET_INVALID_SIZE;
    }
    else if (SetErrorType)
        ATCtx->Error.Type = M68K_AET_UNEXPECTED_CHAR;

    return M68K_FALSE;
}

// parse a value separator: <space>*(,<space>*
static M68K_BOOL ParseValueSeparator(PASM_TEXT_CTX ATCtx)
{
    SkipSpaces(ATCtx);

    if (!ParseChar(ATCtx, ','))
        return M68K_FALSE;

    SkipSpaces(ATCtx);
    return M68K_TRUE;
}

// check if a register is valid i.e. if it belongs to an interval of registers
static M68K_BOOL RegisterIsValid(M68K_REGISTER_TYPE_VALUE Register, M68K_REGISTER_TYPE_VALUE MinRegister, M68K_REGISTER_TYPE_VALUE MaxRegister)
{
    return (Register >= MinRegister && Register <= MaxRegister);
}

// check if a register is valid i.e. if it belongs to one of two interval of registers
static M68K_BOOL RegisterIsValid2(M68K_REGISTER_TYPE_VALUE Register, M68K_REGISTER_TYPE_VALUE MinRegister1, M68K_REGISTER_TYPE_VALUE MaxRegister1, M68K_REGISTER_TYPE_VALUE MinRegister2, M68K_REGISTER_TYPE_VALUE MaxRegister2)
{
    return
        (Register >= MinRegister1 && Register <= MaxRegister1) ||
        (Register >= MinRegister2 && Register <= MaxRegister2);
}

// skip a char if possible
static M68K_BOOL SkipChar(PASM_TEXT_CTX ATCtx, M68K_CHAR Char)
{
    // next char is the same?
    if (ATCtx->Error.Location[0] != Char)
        return M68K_FALSE;

    // skip it
    ATCtx->Error.Location++;
    return M68K_TRUE;
}

// skip a number sign: [+-]?
static void SkipNumberSign(PASM_TEXT_CTX ATCtx, PM68K_BOOL Negative)
{
    M68K_CHAR Char = *(ATCtx->Error.Location);

    if (Char == '+')
    {
        // skip the "+"
        ATCtx->Error.Location++;
        *Negative = M68K_FALSE;
    }
    else if (Char == '-')
    {
        // skip the "-"
        ATCtx->Error.Location++;
        *Negative = M68K_TRUE;
    }
    else
        *Negative = M68K_FALSE;
}

// skip the space(s) in the input text: [\t\n\r ]
static M68K_UINT SkipSpaces(PASM_TEXT_CTX ATCtx)
{
    M68K_UINT Count = 0;
    
    while (CharIsSpace(ATCtx->Error.Location[0]))
    {
        ATCtx->Error.Location++;
        Count++;
    }

    return Count;
}

// search for register using a dynamic text
M68K_REGISTER_TYPE_VALUE _M68KAsmTextBinarySearchRegister(PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd)
{
    M68K_UINT Min = 1;
    M68K_UINT Max = M68K_RT__SIZEOF__ - 1;

    while (Max > Min)
    {
        // get middle index
        M68K_UINT Mid = (Min + Max) / 2;

        if (CompareTexts(_M68KTextRegisters[_M68KTextSortedRegisters[Mid]], DynamicTextStart, DynamicTextEnd) < 0)
            // if the element at [Mid] is lower than the key then we can search in [Mid+1,Max]
            Min = Mid + 1;
        else
            // if the element at [Mid] is greater or equal than the key then we can search in [Min,Mid]
            Max = Mid;
    }

    if (Min == Max)
        if (CompareTexts(_M68KTextRegisters[_M68KTextSortedRegisters[Min]], DynamicTextStart, DynamicTextEnd) == 0)
            return _M68KTextSortedRegisters[Min];

    return (M68K_REGISTER_TYPE_VALUE)M68K_RT__SIZEOF__;
}

// search (binary) for a dynamic text in a table of static texts; returns the index of the text in the table or Max + 1 (not found)
M68K_UINT _M68KAsmTextBinarySearchText(PM68KC_STR Table[], M68K_UINT Min, M68K_UINT Max, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd)
{
    M68K_UINT Error = Max + 1;

    while (Max > Min)
    {
        // get middle index
        M68K_UINT Mid = (Min + Max) / 2;

        if (CompareTexts(Table[Mid], DynamicTextStart, DynamicTextEnd) < 0)
            // if the element at [Mid] is lower than the key then we can search in [Mid+1,Max]
            Min = Mid + 1;
        else
            // if the element at [Mid] is greater or equal than the key then we can search in [Min,Mid]
            Max = Mid;
    }

    if (Min == Max)
        if (CompareTexts(Table[Min], DynamicTextStart, DynamicTextEnd) == 0)
            return Min;

    return Error;
}

// check and fix the size for an immediate value
M68K_BOOL _M68KAsmTextCheckFixImmediateSize(M68K_SDWORD SValue, PM68K_SIZE_VALUE Size)
{
    // size exists?
    switch (*Size)
    {
    case M68K_SIZE_B:
        return (SValue >= MIN_BYTE && SValue <= MAX_BYTE);

    case M68K_SIZE_W:
        return (SValue >= MIN_WORD && SValue <= MAX_WORD);

    case M68K_SIZE_L:
        return M68K_TRUE;

    case M68K_SIZE_NONE:
        if (SValue >= MIN_BYTE && SValue <= MAX_BYTE)
            *Size = M68K_SIZE_B;
        else if (SValue >= MIN_WORD && SValue <= MAX_WORD)
            *Size = M68K_SIZE_W;
        else
            *Size = M68K_SIZE_L;

        return M68K_TRUE;

    default:
        return M68K_FALSE;
    }
}

// get an instruction type using the mnemonic
M68K_INSTRUCTION_TYPE_VALUE _M68KAsmTextCheckMnemonic(PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd, PM68K_OPERAND ImplicitOperand /*can be M68K_NULL*/)
{
    M68K_UINT Index;

    // can we check the mnemonic aliases?
    if (ImplicitOperand != M68K_NULL)
    {
        Index = _M68KAsmTextBinarySearchText(_M68KTextMnemonicAliases, 0, MAT__SIZEOF__ - 1, DynamicTextStart, DynamicTextEnd);
        if (Index < MAT__SIZEOF__)
        {
            PCMNEMONIC_ALIAS alias = _M68KAsmMnemonicAliases + Index;
            ImplicitOperand->Type = alias->OperandType;
            switch (alias->OperandType)
            {
                case M68K_OT_CONDITION_CODE:
                    ImplicitOperand->Info.ConditionCode = (M68K_CONDITION_CODE_VALUE)alias->OperandValue;
                    break;

                case M68K_OT_FPU_CONDITION_CODE:
                    ImplicitOperand->Info.FpuConditionCode = (M68K_FPU_CONDITION_CODE)alias->OperandValue;
                    break;

                case M68K_OT_MMU_CONDITION_CODE:
                    ImplicitOperand->Info.MMUConditionCode = (M68K_MMU_CONDITION_CODE)alias->OperandValue;
                    break;

                default:
                    // we don't know how to init the operand
                    ImplicitOperand->Type = M68K_OT_NONE;
                    break;
            }

            if (ImplicitOperand->Type != M68K_OT_NONE)
                return alias->MasterType;
        }
        else
            ImplicitOperand->Type = M68K_OT_NONE;
    }

    // search in the table of mnemonics
    Index = _M68KAsmTextBinarySearchText(_M68KTextMnemonics, 1, M68K_IT__SIZEOF__ - 1, DynamicTextStart, DynamicTextEnd);
    return (Index < M68K_IT__SIZEOF__ ? (M68K_INSTRUCTION_TYPE_VALUE)Index : M68K_IT_INVALID);
}

// convert a char to lower case
M68K_CHAR _M68KAsmTextConvertToLowerCase(M68K_CHAR Char)
{
    return (Char >= 'A' && Char <= 'Z' ? Char ^ 0x20 : Char);
}

// compare a static text (null-terminated) with a dynamic text
static M68K_INT CompareTexts(PM68KC_STR StaticText, PM68KC_STR DynamicTextStart, PM68KC_STR DynamicTextEnd)
{
    while (DynamicTextStart < DynamicTextEnd)
    {
        // get both chars in upper case
        M68K_CHAR SChar = _M68KAsmTextConvertToLowerCase(*(StaticText++));
        M68K_CHAR DChar = _M68KAsmTextConvertToLowerCase(*(DynamicTextStart++));

        if (SChar < DChar)
            return -1;
        else if (SChar > DChar)
            return 1;
    }

    // also reached the end of the static text?
    return (*StaticText == 0 ? 0 : 1);
}

// assemble an instruction using a textual representation
M68K_WORD M68KAssembleText(PM68K_WORD Address, PM68KC_STR Text, M68K_ARCHITECTURE Architectures, M68K_ASM_FLAGS Flags, PM68K_WORD OutBuffer /*M68K_NULL = return total size*/, M68K_WORD OutSize /*maximum in words*/, PM68K_ASM_ERROR Error /*can be M68K_NULL*/)
{
    // check the parameters
    if (Text != M68K_NULL)
    {
        ASM_TEXT_CTX ATCtx;

        // init
        ATCtx.Address = Address;
        ATCtx.Error.Start = Text;
        ATCtx.Error.Location = Text;
        ATCtx.Error.Type = M68K_AET_NONE;

        // skip all spaces at start
        SkipSpaces(&ATCtx);

        // parse the mnemonic; some instructions (cc) will use the first operand
        PM68K_OPERAND Operand = ATCtx.Instruction.Operands;

        if (ParseMnemonic(&ATCtx, &(ATCtx.Instruction.Type), &Operand))
        {
            if (!ParseSize(&ATCtx, M68K_FALSE, &(ATCtx.Instruction.Size)))
                // implicit size
                ATCtx.Instruction.Size = M68K_SIZE_NONE;

            SkipSpaces(&ATCtx);

            for (M68K_UINT OperandIndex = (M68K_UINT)(Operand - ATCtx.Instruction.Operands); OperandIndex < M68K_MAXIMUM_NUMBER_OPERANDS; OperandIndex++, Operand++)
            {
                // reached the end of the text?
                if (ATCtx.Error.Location[0] == 0)
                {
                    Operand->Type = M68K_OT_NONE;
                    goto no_more_operands;
                }
                    
                // parse the operand
                if (!ParseOperand(&ATCtx, Operand))
                    break;

                SkipSpaces(&ATCtx);

                // get the next char
                M68K_CHAR Char = *ATCtx.Error.Location;
                    
                // more operands?
                if (Char == ',')
                {
                    // allowed?
                    if (OperandIndex + 1 >= M68K_MAXIMUM_NUMBER_OPERANDS)
                    {
                        ATCtx.Error.Type = M68K_AET_TOO_MANY_OPERANDS;
                        break;
                    }

                    // skip the separator and the spaces
                    ATCtx.Error.Location++;
                    SkipSpaces(&ATCtx);
                }
                // reached the end?
                else if (Char == 0)
                {
no_more_operands:
                    // yes! make sure the remaining operands have type M68K_OT_NONE
                    for (OperandIndex++, Operand++; OperandIndex < M68K_MAXIMUM_NUMBER_OPERANDS; OperandIndex++, Operand++)
                        Operand->Type = M68K_OT_NONE;

                    // now we are ready to assemble the instruction
                    M68K_WORD Size = M68KAssemble(Address, &(ATCtx.Instruction), Architectures, Flags, OutBuffer, OutSize);
                    if (Size != 0)
                        return Size;
                        
                    ATCtx.Error.Type = M68K_AET_INVALID_INSTRUCTION;
                    break;
                }
                else
                {
                    ATCtx.Error.Type = M68K_AET_UNEXPECTED_CHAR;
                    break;
                }
            }
        }

        // return the error information?
        if (Error != M68K_NULL)
            *Error = ATCtx.Error;
    }

    // error
    return 0;
}
