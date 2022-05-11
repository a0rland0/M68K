#include "m68kinternal.h"

// try to convert an operand to an operand of a different type
M68K_BOOL M68KConvertOperandTo(M68K_ASM_FLAGS AsmFlags, PM68K_OPERAND Operand, M68K_OPERAND_TYPE_VALUE OperandType, PM68K_OPERAND NewOperand)
{
    M68K_BOOL result = M68K_FALSE;

    if (Operand != M68K_NULL && NewOperand != M68K_NULL)
    {
        // immediate values can always be converted if the fit the required type
        //
        //      B  W  L  Q
        //  B   ko ok ok ok
        //  W   ?  ko ok ok
        //  L   ?  ?  ko ok
        //  Q   ?  ?  ?  ko
        // 
        // XLangLib also allows some conversions
        //
        //  M68K_OT__XL_LANG_ELEMENT__ => M68K_OT_IMMEDIATE_L
        //
        switch (Operand->Type)
        {
        case M68K_OT_IMMEDIATE_B:
            switch (OperandType)
            {
            case M68K_OT_IMMEDIATE_B:
                NewOperand->Info.Byte = Operand->Info.Byte;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_L:
                NewOperand->Info.Long = (M68K_DWORD)Operand->Info.Byte;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_Q:
                // note: order is required to be able to support Operand == NewOperand
                NewOperand->Info.Quad[1] = (M68K_DWORD)Operand->Info.Byte;
                NewOperand->Info.Quad[0] = 0;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_W:
                NewOperand->Info.Word = (M68K_WORD)Operand->Info.Byte;
                result = M68K_TRUE;
                break;

            default:
                break;
            }
            break;

        case M68K_OT_IMMEDIATE_W:
            switch (OperandType)
            {
            case M68K_OT_IMMEDIATE_B:
                if (Operand->Info.Word <= (M68K_WORD)0xff)
                {
                    NewOperand->Info.Byte = (M68K_BYTE)Operand->Info.Word;
                    result = M68K_TRUE;
                }
                break;

            case M68K_OT_IMMEDIATE_L:
                NewOperand->Info.Long = (M68K_DWORD)Operand->Info.Word;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_Q:
                // note: order is required to be able to support Operand == NewOperand
                NewOperand->Info.Quad[1] = (M68K_DWORD)Operand->Info.Word;
                NewOperand->Info.Quad[0] = 0;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_W:
                NewOperand->Info.Word = Operand->Info.Word;
                result = M68K_TRUE;
                break;

            default:
                break;
            }
            break;

        case M68K_OT_IMMEDIATE_L:
            switch (OperandType)
            {
            case M68K_OT_IMMEDIATE_B:
                if (Operand->Info.Long <= (M68K_DWORD)0xff)
                {
                    NewOperand->Info.Byte = (M68K_BYTE)Operand->Info.Long;
                    result = M68K_TRUE;
                }
                break;

            case M68K_OT_IMMEDIATE_L:
                NewOperand->Info.Long = Operand->Info.Long;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_Q:
                // note: order is required to be able to support Operand == NewOperand
                NewOperand->Info.Quad[1] = (M68K_DWORD)Operand->Info.Word;
                NewOperand->Info.Quad[0] = 0;
                result = M68K_TRUE;
                break;

            case M68K_OT_IMMEDIATE_W:
                if (Operand->Info.Long <= (M68K_DWORD)0xffff)
                {
                    NewOperand->Info.Word = (M68K_WORD)Operand->Info.Long;
                    result = M68K_TRUE;
                }
                break;

            default:
                break;
            }
            break;

        case M68K_OT_IMMEDIATE_Q:
            if (Operand->Info.Quad[0] == 0)
            {
                switch (OperandType)
                {
                case M68K_OT_IMMEDIATE_B:
                    if (Operand->Info.Quad[1] <= (M68K_DWORD)0xff)
                    {
                        NewOperand->Info.Byte = (M68K_BYTE)Operand->Info.Quad[1];
                        result = M68K_TRUE;
                    }
                    break;

                case M68K_OT_IMMEDIATE_L:
                    NewOperand->Info.Long = Operand->Info.Quad[1];
                    result = M68K_TRUE;
                    break;

                case M68K_OT_IMMEDIATE_Q:
                    NewOperand->Info.Quad[0] = Operand->Info.Quad[0];
                    NewOperand->Info.Quad[1] = Operand->Info.Quad[1];
                    result = M68K_TRUE;
                    break;

                case M68K_OT_IMMEDIATE_W:
                    if (Operand->Info.Quad[1] <= (M68K_DWORD)0xffff)
                    {
                        NewOperand->Info.Word = (M68K_WORD)Operand->Info.Quad[1];
                        result = M68K_TRUE;
                    }
                    break;

                default:
                    break;
                }
            }
            break;

        case M68K_OT__XL_LANG_ELEMENT__:
            if ((AsmFlags & M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS) != 0 && OperandType == M68K_OT_IMMEDIATE_L)
            {
                NewOperand->Info.Long = 0x7fffffff;
                result = M68K_TRUE;
            }
            break;

        default:
            break;
        }

        if (result)
            NewOperand->Type = OperandType;
    }

    return result;
}

// get the text for an assembler error type
PM68KC_STR M68KGetAsmErrorTypeText(M68K_ASM_ERROR_TYPE ErrorType)
{
    return (ErrorType >= M68K_AET_NONE && ErrorType < M68K_AET__SIZEOF__ ? _M68KTextAsmErrorTypes[ErrorType] : (PM68KC_STR)"?");
}

// get the architectures for the supplied instruction
M68K_ARCHITECTURE_VALUE M68KGetArchitectures(M68K_INTERNAL_INSTRUCTION_TYPE InternalType)
{
    return (InternalType >= 0 && InternalType < IT__SIZEOF__ ? _M68KInstrTypeInfos[InternalType].Architectures : 0);
}

// get the condition flags for the supplied instruction
PM68KC_CONDITION_CODE_FLAG_ACTIONS M68KGetConditionCodeFlagActions(M68K_INTERNAL_INSTRUCTION_TYPE InternalType)
{
    return (InternalType >= 0 && InternalType < IT__SIZEOF__ ? _M68KConditionCodeFlagActions + InternalType : M68K_NULL);
}

// get an IEEE value
M68K_BOOL M68KGetIEEEValue(M68K_IEEE_VALUE_TYPE IEEEValueType, M68K_BOOL InvertSign, M68K_OPERAND_TYPE OperandType, PM68K_OPERAND_INFO OperandInfo)
{
    if (OperandInfo != M68K_NULL)
    {
        if (OperandType == M68K_OT_IMMEDIATE_S)
        {
            if (IEEEValueType == M68K_IEEE_VT_INF || IEEEValueType == M68K_IEEE_VT_NAN || IEEEValueType == M68K_IEEE_VT_ZERO)
            {
                OperandInfo->Single = *(PM68K_SINGLE)(_M68KIEEESingleValues + IEEEValueType);
                if (InvertSign)
                    OperandInfo->Single = -OperandInfo->Single;

                return M68K_TRUE;
            }
        }
        else if (OperandType == M68K_OT_IMMEDIATE_D)
        {
            if (IEEEValueType == M68K_IEEE_VT_INF || IEEEValueType == M68K_IEEE_VT_NAN || IEEEValueType == M68K_IEEE_VT_ZERO)
            {
#ifdef M68K_TARGET_IS_BIG_ENDIAN
                OperandInfo->Double = *(PM68K_DOUBLE)(_M68KIEEEDoubleValues + IEEEValueType);
#else
                M68K_DWORD immediates[2];

                immediates[0] = _M68KIEEEDoubleValues[IEEEValueType][1];
                immediates[1] = _M68KIEEEDoubleValues[IEEEValueType][0];
                
                OperandInfo->Double = *(PM68K_DOUBLE)immediates;
#endif
                if (InvertSign)
                    OperandInfo->Double = -OperandInfo->Double;

                return M68K_TRUE;
            }
        }
        else if (OperandType == M68K_OT_IMMEDIATE_X)
        {
            if (IEEEValueType >= 0 && IEEEValueType < M68K_IEEE_VT__SIZEOF__)
            {
                OperandInfo->Extended[0] = _M68KIEEEExtendedValues[IEEEValueType][0];
                OperandInfo->Extended[1] = _M68KIEEEExtendedValues[IEEEValueType][1];
                OperandInfo->Extended[2] = _M68KIEEEExtendedValues[IEEEValueType][2];

                if (InvertSign)
                    OperandInfo->Extended[0] ^= 0x80000000ul;

                return M68K_TRUE;
            }
        }
    }

    return M68K_FALSE;
}

// get the implicit immediate operand type for the supplied size
M68K_OPERAND_TYPE M68KGetImplicitImmediateOperandTypeForSize(M68K_SIZE Size, PM68K_UINT SizeInBytes /*M68K_NULL = ignored*/)
{
    switch (Size)
    {
    case M68K_SIZE_B:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_BYTE);

        return M68K_OT_IMMEDIATE_B;

    case M68K_SIZE_D:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_DOUBLE);

        return M68K_OT_IMMEDIATE_D;

    case M68K_SIZE_L:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_DWORD);

        return M68K_OT_IMMEDIATE_L;

    case M68K_SIZE_P:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_PACKED_DECIMAL);

        return M68K_OT_IMMEDIATE_P;

    case M68K_SIZE_Q:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_QUAD);

        return M68K_OT_IMMEDIATE_Q;

    case M68K_SIZE_S:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_SINGLE);

        return M68K_OT_IMMEDIATE_S;

    case M68K_SIZE_W:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_WORD);

        return M68K_OT_IMMEDIATE_W;

    case M68K_SIZE_X:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = sizeof(M68K_EXTENDED);

        return M68K_OT_IMMEDIATE_X;

    case M68K_SIZE_NONE:
    default:
        if (SizeInBytes != M68K_NULL)
            *SizeInBytes = 0;

        return M68K_OT_NONE;
    }
}

// get the instruction type using a name
M68K_INSTRUCTION_TYPE M68KGetInstructionType(PM68KC_STR Text, M68K_LUINT TextSize /*0 = empty text*/)
{
    return (Text != M68K_NULL && TextSize != 0 ? _M68KAsmTextCheckMnemonic(Text, Text + TextSize) : M68K_IT_INVALID);
}

// get the register name using a type
PM68KC_STR M68KGetRegisterName(M68K_REGISTER_TYPE RegisterType)
{
    return (RegisterType > M68K_RT_NONE && RegisterType < M68K_RT__SIZEOF__ ? _M68KTextRegisters[RegisterType] : M68K_NULL);
}

// get the register type using a name
M68K_REGISTER_TYPE M68KGetRegisterType(PM68KC_STR Text, M68K_LUINT TextSize /*0 = empty text*/)
{
    if (Text != M68K_NULL && TextSize != 0)
        return _M68KAsmTextBinarySearchRegister(Text, Text + TextSize);
    else
        return M68K_RT_NONE;
}

// get the size using a char
M68K_SIZE M68KGetSizeFromChar(M68KC_CHAR Char)
{
    // convert to lower case
    M68K_CHAR LChar = _M68KAsmTextConvertToLowerCase(Char);
    
    // (linear) search in the table of chars
    for (M68K_SIZE_VALUE Size = M68K_SIZE_NONE + 1; Size < M68K_SIZE__SIZEOF__; Size++)
    {
        M68K_CHAR SChar = _M68KTextSizeChars[Size];

        // found?
        if (SChar == LChar)
            return (M68K_SIZE)Size;

        // since the table is sorted (i.e. the next chars are all higher), can we stop searching?
        else if (SChar > LChar)
            break;
    }

    return M68K_SIZE_NONE;
}

// read a dword from memory
M68K_DWORD M68KReadDWord(PM68K_VOID Address)
{
    if (Address == M68K_NULL)
        return 0;

    return (
        ((M68K_DWORD)((PM68K_BYTE)Address)[0] << 24) |
        ((M68K_DWORD)((PM68K_BYTE)Address)[1] << 16) |
        ((M68K_DWORD)((PM68K_BYTE)Address)[2] << 8) |
        ((M68K_DWORD)((PM68K_BYTE)Address)[3])
    );
}

// read a word from memory
M68K_WORD M68KReadWord(PM68K_VOID Address)
{
    if (Address == M68K_NULL)
        return 0;

    return (
        ((M68K_WORD)((PM68K_BYTE)Address)[0] << 8) |
        ((M68K_WORD)((PM68K_BYTE)Address)[1])
    );
}

// write a dword to memory
void M68KWriteDWord(PM68K_VOID Address, M68K_DWORD Value)
{
    if (Address != M68K_NULL)
    {
        ((PM68K_BYTE)Address)[0] = (M68K_BYTE)(Value >> 24);
        ((PM68K_BYTE)Address)[1] = (M68K_BYTE)(Value >> 16);
        ((PM68K_BYTE)Address)[2] = (M68K_BYTE)(Value >> 8);
        ((PM68K_BYTE)Address)[3] = (M68K_BYTE)Value;
    }
}

// write the value of an immediate operand to memory
M68K_UINT M68KWriteImmediateOperand(PM68K_VOID Address, PM68K_OPERAND Operand)
{
    if (Address != M68K_NULL && Operand != M68K_NULL)
    {
        PM68K_DWORD start;

        switch (Operand->Type)
        {
        case M68K_OT_IMMEDIATE_B:
            *(PM68K_BYTE)Address = Operand->Info.Byte;
            return sizeof(M68K_BYTE);

        case M68K_OT_IMMEDIATE_D:
            start = (PM68K_DWORD)&(Operand->Info.Double);
            M68KWriteDWord(Address, start[0]);
            M68KWriteDWord((PM68K_DWORD)Address + 1, start[1]);
            return sizeof(M68K_DOUBLE);

        case M68K_OT_IMMEDIATE_L:
            M68KWriteDWord(Address, Operand->Info.Long);
            return sizeof(M68K_DWORD);

        case M68K_OT_IMMEDIATE_P:
            M68KWriteDWord(Address, Operand->Info.PackedDecimal[0]);
            M68KWriteDWord((PM68K_DWORD)Address + 1, Operand->Info.PackedDecimal[1]);
            M68KWriteDWord((PM68K_DWORD)Address + 2, Operand->Info.PackedDecimal[2]);
            return sizeof(M68K_PACKED_DECIMAL);

        case M68K_OT_IMMEDIATE_Q:
            M68KWriteDWord(Address, Operand->Info.Quad[0]);
            M68KWriteDWord((PM68K_DWORD)Address + 1, Operand->Info.Quad[1]);
            return sizeof(M68K_QUAD);

        case M68K_OT_IMMEDIATE_S:
            start = (PM68K_DWORD)&(Operand->Info.Double);
            M68KWriteDWord(Address, start[0]);
            return sizeof(M68K_SINGLE);

        case M68K_OT_IMMEDIATE_W:
            M68KWriteWord(Address, Operand->Info.Word);
            return sizeof(M68K_WORD);

        case M68K_OT_IMMEDIATE_X:
            M68KWriteDWord(Address, Operand->Info.Extended[0]);
            M68KWriteDWord((PM68K_DWORD)Address + 1, Operand->Info.Extended[1]);
            M68KWriteDWord((PM68K_DWORD)Address + 2, Operand->Info.Extended[2]);
            return sizeof(M68K_EXTENDED);

        default:
            break;
        }
    }

    return 0;
}

// write a word to memory
void M68KWriteWord(PM68K_VOID Address, M68K_WORD Value)
{
    if (Address != M68K_NULL)
    {
        ((PM68K_BYTE)Address)[0] = (M68K_BYTE)(Value >> 8);
        ((PM68K_BYTE)Address)[1] = (M68K_BYTE)Value;
    }
}
