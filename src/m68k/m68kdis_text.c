#include "m68kinternal.h"

typedef M68K_BOOL (*PADD_OPERAND_FUNC)(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored);

// forward declarations
static void         AddChar(PDISASM_TEXT_CONTEXT DTCtx, M68K_CHAR Char, TEXT_CASE TextCase);
static M68K_BOOL    AddDisplacement(PDISASM_TEXT_CONTEXT DTCtx, M68K_SIZE_VALUE Size, M68K_SDWORD Value, M68K_BOOL Forced, M68K_BOOL AsmText, M68K_CHAR AsmSep, TEXT_CASE TextCase);
static M68K_BOOL    AddIndexScale(PDISASM_TEXT_CONTEXT DTCtx, PM68K_MEMORY Memory, M68K_CHAR Separator, M68K_BOOL AsmText, TEXT_CASE TextCase);
static M68K_BOOL    AddMask(PDISASM_TEXT_CONTEXT DTCtx, M68K_WORD Mask, M68K_REGISTER_TYPE StartRegister, TEXT_CASE TextCase, M68K_CHAR RegSeparator, M68K_CHAR RangeSeparator);
static M68K_BOOL    AddOperand(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored);
static M68K_BOOL    AddOperandAssembler(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored);
static void         AddOperandTypeAssembler(PDISASM_TEXT_CONTEXT DTCtx, PM68KC_STR OperandTypeText, TEXT_CASE TextCase);
static M68K_BOOL    AddOperandXL(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored);
static M68K_BOOL    AddRegister(PDISASM_TEXT_CONTEXT DTCtx, M68K_REGISTER_TYPE Register, TEXT_CASE TextCase);
static M68K_BOOL    AddSize(PDISASM_TEXT_CONTEXT DTCtx, M68K_SIZE Size, M68K_BOOL AsmText, TEXT_CASE TextCase);
static void         AddText(PDISASM_TEXT_CONTEXT DTCtx, PM68KC_STR Text, M68K_LUINT MaximumSize, TEXT_CASE TextCase);
static M68K_BOOL    CallFunction(PDISASM_TEXT_CONTEXT DTCtx, TEXT_CASE TextCase);
static M68K_BOOL    CallFunctionNoDetails(PDISASM_TEXT_CONTEXT DTCtx, M68K_DISASM_TEXT_INFO_TYPE_VALUE Type, TEXT_CASE TextCase);
static PM68K_CHAR   ConvertByteHex(M68K_BYTE Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertDouble(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_DOUBLE Double, PM68K_CHAR Output);
static PM68K_CHAR   ConvertDWordHex(M68K_DWORD Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertExtended(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_EXTENDED Extended, PM68K_CHAR Output);
static PM68K_CHAR   ConvertImmBHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_BYTE Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertImmLHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_DWORD Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertImmWHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_WORD Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertPackedDecimal(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_PACKED_DECIMAL PackedDecimal, PM68K_CHAR Output);
static PM68K_CHAR   ConvertQuad(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_QUAD Quad, PM68K_CHAR Output);
static PM68K_CHAR   ConvertSingle(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_SINGLE Single, PM68K_CHAR Output, PM68K_BOOL AddSuffix);
static PM68K_CHAR   ConvertWordDec(M68K_WORD Value, PM68K_CHAR Output);
static PM68K_CHAR   ConvertWordHex(M68K_WORD Value, PM68K_CHAR Output);
static PM68K_CHAR   CopyHexPrefixes(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_BOOL NegativeValue, M68K_BOOL ValueHigher10, PM68K_CHAR Output);
static PM68K_CHAR   CopyIEEEValue(M68K_IEEE_VALUE_TYPE IEEEValue, M68K_BOOL ForXL, M68K_BOOL Negative, PM68K_CHAR Output);
static PM68K_CHAR   CopyImmPrefix(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_CHAR Output);
static PM68K_CHAR   CopyMantissaExponent(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL Normalized, PM68K_CHAR ConvertedMantissa, M68K_SWORD Exponent, PM68K_CHAR Output);
static PM68K_CHAR   CopyText(PM68KC_STR Text, PM68K_CHAR Output);
static M68K_LUINT   DisassembleFor(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/, PADD_OPERAND_FUNC AddOperandFunc);
static PM68K_CHAR   SkipEndZeros(PM68K_CHAR Start, PM68K_CHAR Last);
static PM68K_CHAR   SkipStartZeros(PM68K_CHAR Start, PM68K_CHAR Last);

// add a char to the output buffer
static void AddChar(PDISASM_TEXT_CONTEXT DTCtx, M68K_CHAR Char, TEXT_CASE TextCase)
{
    // update the total number of chars
    DTCtx->Output.TotalSize++;

    // can we still write the char?
    if (DTCtx->Output.AvailSize != 0)
    {
        // change the text case?
        if (TextCase == TC_LOWER)
        {
            if (Char >= 'A' && Char <= 'Z')
                Char ^= 0x20;
        }
        else if (TextCase == TC_UPPER)
        {
            if (Char >= 'a' && Char <= 'z')
                Char ^= 0x20;
        }

        // copy the char and update the number of free chars
        *(DTCtx->Output.Buffer++) = Char;
        DTCtx->Output.AvailSize--;
    }
}

// add the displacement text to the output buffer
static M68K_BOOL AddDisplacement(PDISASM_TEXT_CONTEXT DTCtx, M68K_SIZE_VALUE Size, M68K_SDWORD Value, M68K_BOOL Forced, M68K_BOOL AsmText, M68K_CHAR AsmSep, TEXT_CASE TextCase)
{
    // no displacement?
    if (Size == M68K_SIZE_NONE || (Value == 0 && !Forced))
        return M68K_TRUE;

    if (AsmText)
    {
        M68K_DISASM_TEXT_FLAGS_VALUE TextFlags = DTCtx->TextFlags;
        M68K_BOOL AsDisplacement = M68K_FALSE;
        M68K_CHAR LocalBuffer[17];

        if (AsmSep != 0)
        {
            if (AsmSep == '+')
            {
                AsDisplacement = M68K_TRUE;
                TextFlags &= ~M68K_DTFLG_IGNORE_SIGN;
            }
            else
                AddChar(DTCtx, AsmSep, TextCase);
        }

        if (Size == M68K_SIZE_B)
            ConvertImmBHex(TextFlags, AsDisplacement, (M68K_BYTE)(M68K_SBYTE)Value, LocalBuffer)[0] = 0;
        else if (Size == M68K_SIZE_W)
            ConvertImmWHex(TextFlags, AsDisplacement, (M68K_WORD)(M68K_SWORD)Value, LocalBuffer)[0] = 0;
        else if (Size == M68K_SIZE_L)
            ConvertImmLHex(TextFlags, AsDisplacement, (M68K_SDWORD)Value, LocalBuffer)[0] = 0;
        else
            return M68K_FALSE;
    
        AddText(DTCtx, LocalBuffer, 0, TextCase);
        return M68K_TRUE;
    }
    else
    {
        if (Size == M68K_SIZE_B)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_B;
            DTCtx->TextInfo.Details.Displacement = (M68K_SDWORD)(M68K_SBYTE)Value;
        }
        else if (Size == M68K_SIZE_W)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_W;
            DTCtx->TextInfo.Details.Displacement = (M68K_SDWORD)(M68K_SWORD)Value;
        }
        else if (Size == M68K_SIZE_L)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_L;
            DTCtx->TextInfo.Details.Displacement = (M68K_SDWORD)Value;
        }
        else
            return M68K_FALSE;

        return CallFunction(DTCtx, TextCase);
    }
}

// add the index and scale information to the output buffer
static M68K_BOOL AddIndexScale(PDISASM_TEXT_CONTEXT DTCtx, PM68K_MEMORY Memory, M68K_CHAR Separator, M68K_BOOL AsmText, TEXT_CASE TextCase)
{
    // index register was specified?
    if (Memory->Index.Register == M68K_RT_NONE)
        return M68K_TRUE;

    if (AsmText)
    {
        if (Separator != 0)
            AddChar(DTCtx, Separator, TextCase);
    }
    else if (Separator != 0)
    {
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_ADD, TextCase))
            return M68K_FALSE;
    }

    if (!AddRegister(DTCtx, Memory->Index.Register, TextCase))
        return M68K_FALSE;

    // write the size if it's not L
    if (Memory->Index.Size != M68K_SIZE_L)
        if (!AddSize(DTCtx, (M68K_SIZE)Memory->Index.Size, AsmText, TextCase))
            return M68K_FALSE;

    if (AsmText)
    {
        if (Memory->Scale < M68K_SCALE_1 || Memory->Scale > M68K_SCALE_8)
            return M68K_FALSE;

        AddChar(DTCtx, '*', TextCase);
        AddChar(DTCtx, _M68KTextScaleChars[Memory->Scale - M68K_SCALE_1], TextCase);
        
        return M68K_TRUE;
    }
    else
    {
        DTCtx->TextInfo.Type = M68K_DTIF_SCALE;
        DTCtx->TextInfo.Details.Scale = Memory->Scale;
    
        return CallFunction(DTCtx, TextCase);
    }
}

// add a mask/list of registers to the output buffer
static M68K_BOOL AddMask(PDISASM_TEXT_CONTEXT DTCtx, M68K_WORD Mask, M68K_REGISTER_TYPE StartRegister, TEXT_CASE TextCase, M68K_CHAR RegSeparator, M68K_CHAR RangeSeparator)
{
    M68K_UINT GroupCount = 0;
    M68K_INT LastRegister = -1;

    for (M68K_INT Index = 0; Index < 8; Index++)
    {
        // bit is set?
        if ((Mask & 1) != 0)
        {
            // yes! continue the group?
            if (LastRegister < 0)
            {
                // no! first register?
                if (GroupCount != 0)
                    // add the separator
                    AddChar(DTCtx, RegSeparator, TC_NONE);

                // write the register
                if (!AddRegister(DTCtx, (M68K_REGISTER_TYPE)(StartRegister + Index), TextCase))
                    return M68K_FALSE;
            
                // save the last register
                LastRegister = Index;
            }
        }
        // no! breaking a group?
        else if (LastRegister >= 0)
        {
            // yes! more than one register?
            if (LastRegister < Index - 1)
            {
                // add the separator
                AddChar(DTCtx, RangeSeparator, TC_NONE);

                // write the register
                if (!AddRegister(DTCtx, (M68K_REGISTER_TYPE)(StartRegister + (Index - 1)), TextCase))
                    return M68K_FALSE;
            }

            LastRegister = -1;
            GroupCount++;
        }

        // move bits ot the right
        Mask >>= 1;
    }

    // process the last group?
    if (LastRegister >= 0 && LastRegister < 7)
    {
        // add the separator
        AddChar(DTCtx, RangeSeparator, TC_NONE);

        // write the register
        if (!AddRegister(DTCtx, (M68K_REGISTER_TYPE)(StartRegister + 7), TextCase))
            return M68K_FALSE;
    }

    return M68K_TRUE;
}

// add the operand text to the output buffer
static M68K_BOOL AddOperand(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored)
{
    M68K_BOOL PreIndexed;
    M68K_BOOL PostIndexed;
    M68K_BOOL Result = M68K_FALSE;
    M68K_OPERAND_TYPE Type = (M68K_OPERAND_TYPE)Operand->Type;

    *Ignored = M68K_FALSE;

    switch (Type)
    {
    case M68K_OT_NONE:
        Result = M68K_TRUE;
        break;

    case M68K_OT_ADDRESS_B:
    case M68K_OT_ADDRESS_L:
    case M68K_OT_ADDRESS_W:
        // do we have a valid offset?
        if (DTCtx->Instruction->PCOffset == 0)
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_L;
        DTCtx->TextInfo.Details.Long = (M68K_DWORD)(M68K_LWORD)(M68K_SLWORD)Operand->Info.Displacement;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_CACHE_TYPE:
        AddText(DTCtx, _M68KTextCacheTypes[Operand->Info.CacheType & (M68K_CT__SIZEOF__ - 1)], 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_CONDITION_CODE:
    case M68K_OT_FPU_CONDITION_CODE:
    case M68K_OT_MMU_CONDITION_CODE:
        // condition code was merged with the mnemonic?
        if ((DTCtx->TextFlags & M68K_DTFLG_CONDITION_CODE_AS_OPERAND) == 0 &&
            (_M68KInstrMasterTypeFlags[DTCtx->Instruction->Type] & IMF_CONDITION_CODE) != 0 && 
            Operand == DTCtx->Instruction->Operands)
        {
            *Ignored = M68K_TRUE;
            Result = M68K_TRUE;
        }
        else
        {
            // write the condition code
            if (Type == M68K_OT_CONDITION_CODE)
            {
                DTCtx->TextInfo.Type = M68K_DTIF_CONDITION_CODE;
                DTCtx->TextInfo.Details.ConditionCode = Operand->Info.ConditionCode;
            }
            else if (Type == M68K_OT_FPU_CONDITION_CODE)
            {
                DTCtx->TextInfo.Type = M68K_DTIF_FPU_CONDITION_CODE;
                DTCtx->TextInfo.Details.FpuConditionCode = Operand->Info.FpuConditionCode;
            }
            else // if (Type == M68K_OT_MMU_CONDITION_CODE)
            {
                DTCtx->TextInfo.Type = M68K_DTIF_MMU_CONDITION_CODE;
                DTCtx->TextInfo.Details.MMUConditionCode = Operand->Info.MMUConditionCode;
            }

            Result = CallFunction(DTCtx, TextCase);
        }
        break;

    case M68K_OT_COPROCESSOR_ID:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_COPROCESSOR_START, TextCase))
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_COPROCESSOR_ID;
        DTCtx->TextInfo.Details.CoprocessorId = (Operand->Info.CoprocessorId & 7);

        if (!CallFunction(DTCtx, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_COPROCESSOR_END, TextCase);
        break;

    case M68K_OT_COPROCESSOR_ID_CONDITION_CODE:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_COPROCESSOR_START, TextCase))
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_COPROCESSOR_ID;
        DTCtx->TextInfo.Details.CoprocessorId = (Operand->Info.CoprocessorIdCC.Id & 7);

        if (!CallFunction(DTCtx, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_COPROCESSOR_SEPARATOR, TextCase))
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_COPROCESSOR_CONDITION_CODE;
        DTCtx->TextInfo.Details.CoprocessorCC = (Operand->Info.CoprocessorIdCC.CC & 0x3f);

        if (!CallFunction(DTCtx, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_COPROCESSOR_END, TextCase);
        break;

    case M68K_OT_DISPLACEMENT_B:
        DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_B;

displacement:
        DTCtx->TextInfo.Details.Displacement = Operand->Info.Displacement;

        // displacement as an address?
        if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
        {
            // do we have a valid offset?
            if (DTCtx->Instruction->PCOffset == 0)
                break;

            DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_L;
            DTCtx->TextInfo.Details.Long = (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset + (M68K_SLWORD)Operand->Info.Displacement);
        }

        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_DISPLACEMENT_L:
        DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_L;
        goto displacement;

    case M68K_OT_DISPLACEMENT_W:
        DTCtx->TextInfo.Type = M68K_DTIF_DISPLACEMENT_W;
        goto displacement;

    case M68K_OT_DYNAMIC_KFACTOR:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_KFACTOR_START, TextCase))
            return M68K_FALSE;

        if (!AddRegister(DTCtx, Operand->Info.Register, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_KFACTOR_END, TextCase);
        break;

    case M68K_OT_IMMEDIATE_B:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_B;
        DTCtx->TextInfo.Details.Byte = Operand->Info.Byte;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_D:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_D;
        DTCtx->TextInfo.Details.Double = Operand->Info.Double;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_L:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_L;
        DTCtx->TextInfo.Details.Long = Operand->Info.Long;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_P:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_P;
        DTCtx->TextInfo.Details.PackedDecimal[0] = Operand->Info.PackedDecimal[0];
        DTCtx->TextInfo.Details.PackedDecimal[1] = Operand->Info.PackedDecimal[1];
        DTCtx->TextInfo.Details.PackedDecimal[2] = Operand->Info.PackedDecimal[2];
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_Q:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_Q;
        DTCtx->TextInfo.Details.Quad[0] = Operand->Info.Quad[0];
        DTCtx->TextInfo.Details.Quad[1] = Operand->Info.Quad[1];
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_S:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_S;
        DTCtx->TextInfo.Details.Single = Operand->Info.Single;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_W:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_W;
        DTCtx->TextInfo.Details.Word = Operand->Info.Word;
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_IMMEDIATE_X:
        DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_X;
        DTCtx->TextInfo.Details.Extended[0] = Operand->Info.Extended[0];
        DTCtx->TextInfo.Details.Extended[1] = Operand->Info.Extended[1];
        DTCtx->TextInfo.Details.Extended[2] = Operand->Info.Extended[2];
        Result = CallFunction(DTCtx, TextCase);
        break;

    case M68K_OT_MEM_ABSOLUTE_L:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_START, TextCase))
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_L;
        DTCtx->TextInfo.Details.Long = Operand->Info.Long;
        
        if (!CallFunction(DTCtx, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_END, TextCase))
            break;

        Result = AddSize(DTCtx, M68K_SIZE_L, M68K_FALSE, TextCase);
        break;

    case M68K_OT_MEM_ABSOLUTE_W:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_START, TextCase))
            break;

        DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_W;
        DTCtx->TextInfo.Details.Word = (M68K_WORD)Operand->Info.Long;
        
        if (!CallFunction(DTCtx, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_END, TextCase))
            break;

        Result = AddSize(DTCtx, M68K_SIZE_W, M68K_FALSE, TextCase);
        break;

    case M68K_OT_MEM_INDIRECT:
    case M68K_OT_MEM_INDIRECT_DISP_W:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_L:
    case M68K_OT_MEM_INDIRECT_POST_INCREMENT:
    case M68K_OT_MEM_INDIRECT_PRE_DECREMENT:
    case M68K_OT_MEM_PC_INDIRECT_DISP_W:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_FALSE;

memory:
        // pre-decrement?
        if (Operand->Info.Memory.Increment == M68K_INC_PRE_DECREMENT)
            if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_PRE_DECREMENT, TextCase))
                break;
        
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_START, TextCase))
            break;
        
        if (PreIndexed || PostIndexed)
            if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_INNER_MEMORY_START, TextCase))
                break;

        M68K_BOOL IgnoreBaseDisplacement = M68K_FALSE;
        M68K_SIZE_VALUE BaseDisplacementSize = Operand->Info.Memory.Displacement.BaseSize;
        M68K_SDWORD BaseDisplacementValue = Operand->Info.Memory.Displacement.BaseValue;

        if (Operand->Info.Memory.Base != M68K_RT_NONE)
        {
            // base register is pc? if so we can add the displacement and show the base register as an address
            if (Operand->Info.Memory.Base == M68K_RT_PC && 
                (BaseDisplacementSize == M68K_SIZE_B || BaseDisplacementSize == M68K_SIZE_W || BaseDisplacementSize == M68K_SIZE_L) &&
                (DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
            {
                // do we have a valid offset?
                if (DTCtx->Instruction->PCOffset == 0)
                    break;

                IgnoreBaseDisplacement = M68K_TRUE;
                
                if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_RELATIVE_ADDRESS_PREFIX, TextCase))
                    break;

                DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_L;
                DTCtx->TextInfo.Details.Long = (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset + (M68K_SLWORD)BaseDisplacementValue);

                if (!CallFunction(DTCtx, TextCase))
                    break;
            }
            // base register is the relative pc?
            else if (Operand->Info.Memory.Base == M68K_RT__RPC)
            {
                // base displacement must be byte, word or long
                if (BaseDisplacementSize != M68K_SIZE_B && BaseDisplacementSize != M68K_SIZE_W && BaseDisplacementSize != M68K_SIZE_L)
                    break;

                // do we have a valid offset?
                if (DTCtx->Instruction->PCOffset == 0)
                    break;

                // calculate the real displacement value
                BaseDisplacementValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)BaseDisplacementValue - ((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset));
                
                // does it fit the displacement size?
                if (!_M68KAsmTextCheckFixImmediateSize(BaseDisplacementValue, &BaseDisplacementSize))
                    break;

                // display as address?
                if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
                {
                    // yes
                    IgnoreBaseDisplacement = M68K_TRUE;
                
                    if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_RELATIVE_ADDRESS_PREFIX, TextCase))
                        break;

                    DTCtx->TextInfo.Type = M68K_DTIF_ADDRESS_L;
                    DTCtx->TextInfo.Details.Long = (M68K_DWORD)Operand->Info.Memory.Displacement.BaseValue;

                    if (!CallFunction(DTCtx, TextCase))
                        break;
                }
                // write the pc register; the displacement is written below
                else if (!AddRegister(DTCtx, M68K_RT_PC, TextCase))
                    break;
            }
            else if (!AddRegister(DTCtx, Operand->Info.Memory.Base, TextCase))
                break;
        }

        // the base displacement is forced
        // 1) when it's mandatory
        // 2) for relative addresses
        // 3) when we are using zpc
        M68K_BOOL ForcedBaseDisplacement = (
            Operand->Type == M68K_OT_MEM_INDIRECT_DISP_W ||
            Operand->Type == M68K_OT_MEM_PC_INDIRECT_DISP_W ||
            Operand->Info.Memory.Base == M68K_RT__RPC ||
            (
                (Operand->Info.Memory.Base == M68K_RT_NONE || Operand->Info.Memory.Base == M68K_RT_ZPC) &&
                Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                BaseDisplacementSize == M68K_SIZE_W && /* check "case 6:" in InitAModeOperandEx */
                BaseDisplacementValue == 0 &&
                Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE
            )
        );

        // post-indexed?
        if (PostIndexed)
        {
            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_FALSE, 0, TextCase))
                    break;

            if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_INNER_MEMORY_END, TextCase))
                break;

            if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), ',', M68K_FALSE, TextCase))
                break;

            if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_FALSE, 0, TextCase))
                break;
        }
        // no! pre-indexed or indirect
        else
        {
            if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), (Operand->Info.Memory.Base != M68K_RT_NONE ? ',' : 0), M68K_FALSE, TextCase))
                break;

            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_FALSE, 0, TextCase))
                    break;

            if (PreIndexed)
            {
                if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_INNER_MEMORY_END, TextCase))
                    break;

                if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_FALSE, 0, TextCase))
                    break;
            }
        }

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_END, TextCase))
            break;

        // post-increment?
        if (Operand->Info.Memory.Increment == M68K_INC_POST_INCREMENT)
            if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_POST_INCREMENT, TextCase))
                break;

        Result = M68K_TRUE;
        break;

    case M68K_OT_MEM_INDIRECT_POST_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_POST_INDEXED:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_TRUE;
        goto memory;

    case M68K_OT_MEM_INDIRECT_PRE_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED:
        PreIndexed = M68K_TRUE;
        PostIndexed  = M68K_FALSE;
        goto memory;

    case M68K_OT_MEM_REGISTER_PAIR:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_START, TextCase))
            break;

        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register1, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_END, TextCase))
            break;
        
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_PAIR_SEPARATOR, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_START, TextCase))
            break;

        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register2, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_MEMORY_END, TextCase);
        break;

    case M68K_OT_OFFSET_WIDTH:
        // both fields are valid?
        if (Operand->Info.OffsetWidth.Offset < -32 || (Operand->Info.OffsetWidth.Offset >= 0 && !(Operand->Info.OffsetWidth.Offset >= M68K_RT_D0 && Operand->Info.OffsetWidth.Offset <= M68K_RT_D7)))
            break;

        if (Operand->Info.OffsetWidth.Width < -33 ||  Operand->Info.OffsetWidth.Width == -1 /*0*/ || (Operand->Info.OffsetWidth.Width >= 0 && !(Operand->Info.OffsetWidth.Width >= M68K_RT_D0 && Operand->Info.OffsetWidth.Width <= M68K_RT_D7)))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_OFFSET_WIDTH_START, TextCase))
            break;

        // offset is a value or a register?
        if (Operand->Info.OffsetWidth.Offset < 0)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_OFFSET_WIDTH_VALUE;
            DTCtx->TextInfo.Details.Byte = (M68K_BYTE)(-Operand->Info.OffsetWidth.Offset - 1);

            if (!CallFunction(DTCtx, TextCase))
                break;
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Offset, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_OFFSET_WIDTH_SEPARATOR, TextCase))
            break;

        if (Operand->Info.OffsetWidth.Width < 0)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_OFFSET_WIDTH_VALUE;
            DTCtx->TextInfo.Details.Byte = (M68K_BYTE)(-Operand->Info.OffsetWidth.Width - 1);

            if (!CallFunction(DTCtx, TextCase))
                break;
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Width, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_OFFSET_WIDTH_END, TextCase);
        break;

    case M68K_OT_REGISTER:
        Result = AddRegister(DTCtx, (M68K_REGISTER_TYPE)Operand->Info.Register, TextCase);
        break;
    
    case M68K_OT_REGISTER_FP_LIST:
        {
            M68K_WORD FPValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
            if (FPValue == 0)
            {
                DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_B;
                DTCtx->TextInfo.Details.Word = 0;
                Result = CallFunction(DTCtx, TextCase);
            }
            else
                Result = AddMask(DTCtx, FPValue, M68K_RT_FP0, TextCase, '/', '-');
        }
        break;

    case M68K_OT_REGISTER_FPCR_LIST:
        {
            M68K_WORD FPCRValue = (M68K_WORD)(Operand->Info.RegisterList & 7);
            if (FPCRValue == 0)
            {
                DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_B;
                DTCtx->TextInfo.Details.Word = 0;
                Result = CallFunction(DTCtx, TextCase);
            }
            else
            {
                // FPCR?
                if ((FPCRValue & 1) != 0)
                {
                    if (!AddRegister(DTCtx, M68K_RT_FPCR, TextCase))
                        break;

                    // add separator?
                    if ((FPCRValue & 6) != 0)
                        AddChar(DTCtx, '/', TextCase);
                }

                // FPSR?
                if ((FPCRValue & 2) != 0)
                {
                    if (!AddRegister(DTCtx, M68K_RT_FPSR, TextCase))
                        break;

                    // add separator?
                    if ((FPCRValue & 4) != 0)
                        AddChar(DTCtx, '/', TextCase);
                }

                // FPIAR?
                if ((FPCRValue & 4) != 0)
                    if (!AddRegister(DTCtx, M68K_RT_FPIAR, TextCase))
                        break;

                Result = M68K_TRUE;
            }
        }
        break;

    case M68K_OT_REGISTER_LIST:
        if (Operand->Info.RegisterList == 0)
        {
            DTCtx->TextInfo.Type = M68K_DTIF_IMMEDIATE_W;
            DTCtx->TextInfo.Details.Word = 0;
            Result = CallFunction(DTCtx, TextCase);
        }
        else
        {
            M68K_WORD DValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
            M68K_WORD AValue =  (M68K_WORD)(Operand->Info.RegisterList >> 8);

            if (!AddMask(DTCtx, DValue, M68K_RT_D0, TextCase, '/', '-'))
                break;
            
            if (DValue != 0 && AValue != 0)
                AddChar(DTCtx, '/', TextCase);
            
            if (!AddMask(DTCtx, AValue, M68K_RT_A0, TextCase, '/', '-'))
                break;

            Result = M68K_TRUE;
        }
        break;

    case M68K_OT_REGISTER_PAIR:
    case M68K_OT_VREGISTER_PAIR_M2:
    case M68K_OT_VREGISTER_PAIR_M4:
        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register1, TextCase))
            break;

        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_PAIR_SEPARATOR, TextCase))
            break;

        Result = AddRegister(DTCtx, Operand->Info.RegisterPair.Register2, TextCase);
        break;

    case M68K_OT_STATIC_KFACTOR:
        if (!CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_KFACTOR_START, TextCase))
            return M68K_FALSE;

        DTCtx->TextInfo.Type = M68K_DTIF_STATIC_KFACTOR;
        DTCtx->TextInfo.Details.SByte = Operand->Info.SByte;
        if (!CallFunction(DTCtx, TextCase))
            break;

        Result = CallFunctionNoDetails(DTCtx, M68K_DTIF_TEXT_KFACTOR_END, TextCase);
        break;

    default:
        break;
    }

    return Result;
}

// add the operand text to the output buffer (for the assembler function)
static M68K_BOOL AddOperandAssembler(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored)
{
    PM68KC_STR TypeText;
    M68K_BOOL PreIndexed;
    M68K_BOOL PostIndexed;
    PM68K_CHAR NextOutput;

    M68K_BOOL Result = M68K_FALSE;
    M68K_OPERAND_TYPE Type = (M68K_OPERAND_TYPE)Operand->Type;

    // init
    NextOutput = DTCtx->TextInfo.OutBuffer;
    *Ignored = M68K_FALSE;

    switch (Type)
    {
    case M68K_OT_NONE:
        Result = M68K_TRUE;
        break;

    case M68K_OT_ADDRESS_B:
        TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_B];

address:
        // do we have a valid offset?
        if (DTCtx->Instruction->PCOffset == 0)
            break;

        AddOperandTypeAssembler(DTCtx, TypeText, TextCase);
        AddChar(DTCtx, '&', TextCase);

        ConvertImmLHex(DTCtx->TextFlags | M68K_DTFLG_TRAILING_ZEROS | M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, (M68K_DWORD)(M68K_LWORD)(M68K_SLWORD)Operand->Info.Displacement, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_ADDRESS_L:
        TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_L];
        goto address;

    case M68K_OT_ADDRESS_W:
        TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_W];
        goto address;

    case M68K_OT_CACHE_TYPE:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_CACHE_TYPE], TextCase);
        AddText(DTCtx, _M68KTextCacheTypes[Operand->Info.CacheType & (M68K_CT__SIZEOF__ - 1)], 0, TextCase);

add_end:
        AddChar(DTCtx, ')', TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_CONDITION_CODE:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_CONDITION_CODE], TextCase);
        AddText(DTCtx, _M68KTextConditionCodes[Operand->Info.ConditionCode & (M68K_CC__SIZEOF__ - 1)], 0, TextCase);
        goto add_end;

    case M68K_OT_COPROCESSOR_ID:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_COPROCESSOR_ID], TextCase);
        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorId & 7), NextOutput)[0] = 0;

add_outbuffer_end:
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        goto add_end;

    case M68K_OT_COPROCESSOR_ID_CONDITION_CODE:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_COPROCESSOR_ID_CONDITION_CODE], TextCase);
        
        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorIdCC.Id & 7), NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);

        AddChar(DTCtx, ',', TC_NONE);
        
        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorIdCC.CC & 0x3f), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_DISPLACEMENT_B:
        // displacement as an address?
        if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
        {
            TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_B];

displacement_as_address:
            // do we have a valid offset?
            if (DTCtx->Instruction->PCOffset == 0)
                break;

            AddOperandTypeAssembler(DTCtx, TypeText, TextCase);
            AddChar(DTCtx, '&', TextCase);

            ConvertImmLHex(DTCtx->TextFlags | M68K_DTFLG_TRAILING_ZEROS | M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset + (M68K_SLWORD)Operand->Info.Displacement), NextOutput)[0] = 0;
        }
        else
        {
            AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_DISPLACEMENT_B], TextCase);
            ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(M68K_SBYTE)Operand->Info.Displacement, NextOutput)[0] = 0;
        }
        goto add_outbuffer_end;

    case M68K_OT_DISPLACEMENT_L:
        // displacement as an address?
        if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
        {
            TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_L];
            goto displacement_as_address;
        }
        else
        {
            AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_DISPLACEMENT_L], TextCase);
            ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, (M68K_DWORD)Operand->Info.Displacement, NextOutput)[0] = 0;
        }
        goto add_outbuffer_end;

    case M68K_OT_DISPLACEMENT_W:
        // displacement as an address?
        if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
        {
            TypeText = _M68KTextAsmOperandTypes[ATOT_ADDRESS_W];
            goto displacement_as_address;
        }
        else
        {
            AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_DISPLACEMENT_W], TextCase);
            ConvertImmWHex(DTCtx->TextFlags, M68K_FALSE, (M68K_WORD)(M68K_SWORD)Operand->Info.Displacement, NextOutput)[0] = 0;
        }
        goto add_outbuffer_end;
    
    case M68K_OT_DYNAMIC_KFACTOR:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_DYNAMIC_KFACTOR], TextCase);
        
        if (!AddRegister(DTCtx, Operand->Info.Register, TextCase))
            break;

        goto add_end;

    case M68K_OT_FPU_CONDITION_CODE:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_FPU_CONDITION_CODE], TextCase);
        AddText(DTCtx, _M68KTextFpuConditionCodes[Operand->Info.FpuConditionCode & (M68K_FPCC__SIZEOF__ - 1)], 0, TextCase);
        goto add_end;

    case M68K_OT_IMMEDIATE_B:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_B], TextCase);
        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Byte, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_D:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_D], TextCase);
        ConvertDouble(DTCtx->TextFlags, M68K_FALSE, &(Operand->Info.Double), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_L:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_L], TextCase);
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Long, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_P:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_P], TextCase);
        ConvertPackedDecimal(DTCtx->TextFlags, &(Operand->Info.PackedDecimal), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_Q:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_Q], TextCase);
        ConvertQuad(DTCtx->TextFlags, &(Operand->Info.Quad), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_S:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_S], TextCase);
        ConvertSingle(DTCtx->TextFlags, M68K_FALSE, &(Operand->Info.Single), NextOutput, M68K_NULL)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_W:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_W], TextCase);
        ConvertImmWHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Word, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_X:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_IMMEDIATE_X], TextCase);
        ConvertExtended(DTCtx->TextFlags, M68K_FALSE, &(Operand->Info.Extended), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_MEM_ABSOLUTE_L:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY], TextCase);
        
        ConvertImmLHex(DTCtx->TextFlags | M68K_DTFLG_TRAILING_ZEROS, M68K_FALSE, Operand->Info.Long, NextOutput)[0] = 0;
        AddText(DTCtx, NextOutput, 0, TextCase);

        if (!AddSize(DTCtx, M68K_SIZE_L, M68K_TRUE, TextCase))
            break;
        
        goto add_end;

    case M68K_OT_MEM_ABSOLUTE_W:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY], TextCase);

        ConvertImmWHex(DTCtx->TextFlags | M68K_DTFLG_TRAILING_ZEROS, M68K_FALSE, (M68K_WORD)Operand->Info.Long, NextOutput)[0] = 0;
        AddText(DTCtx, NextOutput, 0, TextCase);

        if (!AddSize(DTCtx, M68K_SIZE_W, M68K_TRUE, TextCase))
            break;

        goto add_end;

    case M68K_OT_MEM_INDIRECT:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY], TextCase);

mem_areg:
        if (!AddRegister(DTCtx, Operand->Info.Memory.Base, TextCase))
            break;

        goto add_end;

    case M68K_OT_MEM_INDIRECT_DISP_W:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_L:
    case M68K_OT_MEM_PC_INDIRECT_DISP_W:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_FALSE;

memory:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY], TextCase);

        M68K_BOOL AddSeparator = M68K_FALSE;
        M68K_BOOL IgnoreBaseDisplacement = M68K_FALSE;
        M68K_SIZE_VALUE BaseDisplacementSize = Operand->Info.Memory.Displacement.BaseSize;
        M68K_SDWORD BaseDisplacementValue = Operand->Info.Memory.Displacement.BaseValue;

        if (PreIndexed || PostIndexed)
            AddChar(DTCtx, '[', TC_NONE);

        if (Operand->Info.Memory.Base != M68K_RT_NONE)
        {
            // base register is pc? if so we can add the displacement and show the base register as an address
            if (Operand->Info.Memory.Base == M68K_RT_PC && 
                (BaseDisplacementSize == M68K_SIZE_B || BaseDisplacementSize == M68K_SIZE_W || BaseDisplacementSize == M68K_SIZE_L) &&
                (DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
            {
                // do we have a valid offset?
                if (DTCtx->Instruction->PCOffset == 0)
                    break;

                IgnoreBaseDisplacement = M68K_TRUE;
                AddChar(DTCtx, '&', TC_NONE);
                
                ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset + (M68K_SLWORD)BaseDisplacementValue), NextOutput)[0] = 0;
                AddText(DTCtx, NextOutput, 0, TextCase);
            }
            // base register is the relative pc?
            else if (Operand->Info.Memory.Base == M68K_RT__RPC)
            {
                // base displacement must be byte, word or long
                if (BaseDisplacementSize != M68K_SIZE_B && BaseDisplacementSize != M68K_SIZE_W && BaseDisplacementSize != M68K_SIZE_L)
                    break;

                // do we have a valid offset?
                if (DTCtx->Instruction->PCOffset == 0)
                    break;

                // calculate the real displacement value
                BaseDisplacementValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)BaseDisplacementValue - ((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset));

                // does it fit the displacement size?
                if (!_M68KAsmTextCheckFixImmediateSize(BaseDisplacementValue, &BaseDisplacementSize))
                    break;

                // display as address?
                if ((DTCtx->TextFlags & M68K_DTFLG_DISPLACEMENT_AS_VALUE) == 0)
                {
                    // yes
                    IgnoreBaseDisplacement = M68K_TRUE;
                    AddChar(DTCtx, '&', TC_NONE);
                
                    ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, (M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)DTCtx->Address + (M68K_SLWORD)(M68K_SWORD)DTCtx->Instruction->PCOffset + (M68K_SLWORD)BaseDisplacementValue), NextOutput)[0] = 0;
                    AddText(DTCtx, NextOutput, 0, TextCase);
                }
                // write the pc register; the displacement is written below
                else if (!AddRegister(DTCtx, M68K_RT_PC, TextCase))
                    break;
            }
            else if (!AddRegister(DTCtx, Operand->Info.Memory.Base, TextCase))
                break;

            AddSeparator = M68K_TRUE;
        }

        // the base displacement is forced
        // 1) when it's mandatory
        // 2) for relative addresses
        // 3) when we are using zpc
        M68K_BOOL ForcedBaseDisplacement = (
            Operand->Type == M68K_OT_MEM_INDIRECT_DISP_W ||
            Operand->Type == M68K_OT_MEM_PC_INDIRECT_DISP_W ||
            Operand->Info.Memory.Base == M68K_RT__RPC ||
            (
                (Operand->Info.Memory.Base == M68K_RT_NONE || Operand->Info.Memory.Base == M68K_RT_ZPC) &&
                Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                (BaseDisplacementSize == M68K_SIZE_W || BaseDisplacementSize == M68K_SIZE_L) && /* check "case 6:" in InitAModeOperandEx */
                BaseDisplacementValue == 0 &&
                Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE
            )
        );

        // post-indexed?
        if (PostIndexed)
        {
            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_TRUE, (AddSeparator ? ',' : 0), TextCase))
                    break;

            AddChar(DTCtx, ']', TC_NONE);

            if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), ',', M68K_TRUE, TextCase))
                break;

            if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_TRUE, ',', TextCase))
                break;
        }
        // no! pre-indexed or indirect
        else
        {
            // index and scale are present?
            if (Operand->Info.Memory.Index.Register != M68K_RT_NONE)
            {
                if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), (AddSeparator ? ',' : 0), M68K_TRUE, TextCase))
                    break;

                AddSeparator = M68K_TRUE;
            }

            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_TRUE, (AddSeparator ? ',' : 0), TextCase))
                    break;

            if (PreIndexed)
            {
                AddChar(DTCtx, ']', TC_NONE);

                if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_TRUE, ',', TextCase))
                    break;
            }
        }

        goto add_end;

    case M68K_OT_MEM_INDIRECT_POST_INCREMENT:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY_POST_INCREMENT], TextCase);
        goto mem_areg;

    case M68K_OT_MEM_INDIRECT_POST_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_POST_INDEXED:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_TRUE;
        goto memory;

    case M68K_OT_MEM_INDIRECT_PRE_DECREMENT:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY_PRE_DECREMENT], TextCase);
        goto mem_areg;

    case M68K_OT_MEM_INDIRECT_PRE_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED:
        PreIndexed = M68K_TRUE;
        PostIndexed  = M68K_FALSE;
        goto memory;

    case M68K_OT_MEM_REGISTER_PAIR:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MEMORY_PAIR], TextCase);

reg_pair:
        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register1, TextCase))
            break;

        AddChar(DTCtx, ',', TC_NONE);

        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register2, TextCase))
            break;
        goto add_end;

    case M68K_OT_MMU_CONDITION_CODE:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_MMU_CONDITION_CODE], TextCase);
        AddText(DTCtx, _M68KTextMMUConditionCodes[Operand->Info.ConditionCode & (M68K_MMUCC__SIZEOF__ - 1)], 0, TextCase);
        goto add_end;

    case M68K_OT_OFFSET_WIDTH:
        // both fields are valid?
        if (Operand->Info.OffsetWidth.Offset < -32 || (Operand->Info.OffsetWidth.Offset >= 0 && !(Operand->Info.OffsetWidth.Offset >= M68K_RT_D0 && Operand->Info.OffsetWidth.Offset <= M68K_RT_D7)))
            break;

        if (Operand->Info.OffsetWidth.Width < -33 || Operand->Info.OffsetWidth.Width == -1 /*0*/ || (Operand->Info.OffsetWidth.Width >= 0 && !(Operand->Info.OffsetWidth.Width >= M68K_RT_D0 && Operand->Info.OffsetWidth.Width <= M68K_RT_D7)))
            break;
        
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_OFFSET_WIDTH], TextCase);

        // offset is a value or a register?
        if (Operand->Info.OffsetWidth.Offset < 0)
        {
            ConvertWordDec((M68K_WORD)(M68K_BYTE)(-Operand->Info.OffsetWidth.Offset - 1), NextOutput)[0] = 0;
            AddText(DTCtx, NextOutput, 0, TextCase);
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Offset, TextCase))
            break;

        AddChar(DTCtx, ',', TC_NONE);

        if (Operand->Info.OffsetWidth.Width < 0)
        {
            ConvertWordDec((M68K_WORD)(M68K_BYTE)(-Operand->Info.OffsetWidth.Width - 1), NextOutput)[0] = 0;
            AddText(DTCtx, NextOutput, 0, TextCase);
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Width, TextCase))
            break;

        goto add_end;

    case M68K_OT_REGISTER:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_REGISTER], TextCase);
        if (!AddRegister(DTCtx, (M68K_REGISTER_TYPE)Operand->Info.Register, TextCase))
            break;
        goto add_end;

    case M68K_OT_REGISTER_FP_LIST:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_FPU_REGISTER_LIST], TextCase);

        M68K_WORD FPValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
        if (FPValue == 0)
            AddText(DTCtx, "0", 0, TC_NONE);
        else if (!AddMask(DTCtx, (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList, M68K_RT_FP0, TextCase, '/', '-'))
            break;
        
        goto add_end;

    case M68K_OT_REGISTER_FPCR_LIST:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_FPU_CONTROL_REGISTER_LIST], TextCase);
        
        M68K_WORD FPCRValue = (M68K_WORD)(Operand->Info.RegisterList & 7);
        if (FPCRValue == 0)
            AddText(DTCtx, "0", 0, TC_NONE);
        else
        {
            // FPCR?
            if ((FPCRValue & 1) != 0)
            {
                if (!AddRegister(DTCtx, M68K_RT_FPCR, TextCase))
                    break;

                // add separator?
                if ((FPCRValue & 6) != 0)
                    AddChar(DTCtx, '/', TextCase);
            }

            // FPSR?
            if ((FPCRValue & 2) != 0)
            {
                if (!AddRegister(DTCtx, M68K_RT_FPSR, TextCase))
                    break;

                // add separator?
                if ((FPCRValue & 4) != 0)
                    AddChar(DTCtx, '/', TextCase);
            }

            // FPIAR?
            if ((FPCRValue & 4) != 0)
                if (!AddRegister(DTCtx, M68K_RT_FPIAR, TextCase))
                    break;
        }
        goto add_end;

    case M68K_OT_REGISTER_LIST:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_REGISTER_LIST], TextCase);
        
        if (Operand->Info.RegisterList == 0)
            AddText(DTCtx, "0", 0, TC_NONE);
        else
        {
            M68K_WORD DValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
            M68K_WORD AValue =  (M68K_WORD)(Operand->Info.RegisterList >> 8);

            if (!AddMask(DTCtx, DValue, M68K_RT_D0, TextCase, '/', '-'))
                break;
            
            if (DValue != 0 && AValue != 0)
                AddChar(DTCtx, '/', TextCase);
            
            if (!AddMask(DTCtx, AValue, M68K_RT_A0, TextCase, '/', '-'))
                break;
        }
        goto add_end;

    case M68K_OT_REGISTER_PAIR:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_REGISTER_PAIR], TextCase);
        goto reg_pair;

    case M68K_OT_VREGISTER_PAIR_M2:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_VREGISTER_PAIR_M2], TextCase);
        goto reg_pair;

    case M68K_OT_VREGISTER_PAIR_M4:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_VREGISTER_PAIR_M4], TextCase);
        goto reg_pair;

    case M68K_OT_STATIC_KFACTOR:
        AddOperandTypeAssembler(DTCtx, _M68KTextAsmOperandTypes[ATOT_STATIC_KFACTOR], TextCase);
        ConvertImmBHex(DTCtx->TextFlags & ~M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, (M68K_BYTE)Operand->Info.SByte, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    default:
        Result = M68K_FALSE;
        break;
    }

    return Result;
}

// add the text for the operand type to the output buffer
static void AddOperandTypeAssembler(PDISASM_TEXT_CONTEXT DTCtx, PM68KC_STR OperandTypeText, TEXT_CASE TextCase)
{
    AddChar(DTCtx, '%', TC_NONE);
    AddText(DTCtx, OperandTypeText, 0, TextCase);
    AddChar(DTCtx, '(', TC_NONE);
}

// add the operand text to the output buffer (for the XL language)
static M68K_BOOL AddOperandXL(PDISASM_TEXT_CONTEXT DTCtx, PM68K_OPERAND Operand, TEXT_CASE TextCase, PM68K_BOOL Ignored)
{
    PM68K_CHAR NextOutput;

    M68K_BOOL AddSuffix;
    M68K_BOOL PreIndexed;
    M68K_BOOL PostIndexed;
    M68K_BOOL Result = M68K_FALSE;
    M68K_OPERAND_TYPE Type = (M68K_OPERAND_TYPE)Operand->Type;

    // init
    NextOutput = DTCtx->TextInfo.OutBuffer;
    *Ignored = M68K_FALSE;

    switch (Type)
    {
    case M68K_OT_NONE:
        Result = M68K_TRUE;
        break;

    case M68K_OT_ADDRESS_B:
    case M68K_OT_ADDRESS_L:
    case M68K_OT_ADDRESS_W:
    case M68K_OT_DISPLACEMENT_B:
    case M68K_OT_DISPLACEMENT_L:
    case M68K_OT_DISPLACEMENT_W:
        AddText(DTCtx, "label L", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_CACHE_TYPE:
        AddText(DTCtx, "<cache> ", 0, TC_NONE);
        AddText(DTCtx, _M68KTextCacheTypes[Operand->Info.CacheType & (M68K_CT__SIZEOF__ - 1)], 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_CONDITION_CODE:
        AddText(DTCtx, "<cc> ", 0, TC_NONE);
        AddText(DTCtx, _M68KTextConditionCodes[Operand->Info.ConditionCode & (M68K_CC__SIZEOF__ - 1)], 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_COPROCESSOR_ID:
        AddText(DTCtx, "<cop> ", 0, TC_NONE);

        if (Operand->Info.CoprocessorId == M68K_CID_MMU)
            AddText(DTCtx, "mmu", 0, TC_NONE);
        else if (Operand->Info.CoprocessorId == M68K_CID_FPU)
            AddText(DTCtx, "fpu", 0, TC_NONE);
        else
        {
            ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorId & 7), NextOutput)[0] = 0;
            AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        }

        Result = M68K_TRUE;
        break;

    case M68K_OT_COPROCESSOR_ID_CONDITION_CODE:
        AddText(DTCtx, "<copcc> ", 0, TC_NONE);

        if (Operand->Info.CoprocessorIdCC.Id == M68K_CID_MMU)
            AddText(DTCtx, "mmu", 0, TC_NONE);
        else if (Operand->Info.CoprocessorIdCC.Id == M68K_CID_FPU)
            AddText(DTCtx, "fpu", 0, TC_NONE);
        else
        {
            ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorIdCC.Id & 7), NextOutput)[0] = 0;
            AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        }

        AddChar(DTCtx, ':', TC_NONE);

        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(Operand->Info.CoprocessorIdCC.CC & 0x3f), NextOutput)[0] = 0;
        
    add_outbuffer_end:
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_DYNAMIC_KFACTOR:
        AddText(DTCtx, "<dkf> ", 0, TC_NONE);
        Result = AddRegister(DTCtx, Operand->Info.Register, TextCase);
        break;

    case M68K_OT_FPU_CONDITION_CODE:
        AddText(DTCtx, "<fpucc> ", 0, TC_NONE);
        AddText(DTCtx, _M68KTextFpuConditionCodes[Operand->Info.FpuConditionCode & (M68K_FPCC__SIZEOF__ - 1)], 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_IMMEDIATE_B:
        ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Byte, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_D:
        ConvertDouble(DTCtx->TextFlags, M68K_TRUE, &(Operand->Info.Double), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_L:
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Long, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_P:
        AddText(DTCtx, "<packeddec> ", 0, TC_NONE);

        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.PackedDecimal[0], NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        
        AddText(DTCtx, ":", 0, TC_NONE);
        
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.PackedDecimal[1], NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        
        AddText(DTCtx, ":", 0, TC_NONE);
        
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.PackedDecimal[2], NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_Q:
        ConvertQuad(DTCtx->TextFlags, &(Operand->Info.Quad), NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_S:
        ConvertSingle(DTCtx->TextFlags, M68K_TRUE, &(Operand->Info.Single), NextOutput, &AddSuffix)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        
        if (AddSuffix)
            AddText(DTCtx, "f", 0, TC_NONE);

        Result = M68K_TRUE;
        break;

    case M68K_OT_IMMEDIATE_W:
        ConvertImmWHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Word, NextOutput)[0] = 0;
        goto add_outbuffer_end;

    case M68K_OT_IMMEDIATE_X:
        AddText(DTCtx, "<extended> ", 0, TC_NONE);

        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Extended[0], NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        
        AddText(DTCtx, ":", 0, TC_NONE);
        
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Extended[1], NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        
        AddText(DTCtx, ":", 0, TC_NONE);
        
        ConvertImmLHex(DTCtx->TextFlags, M68K_FALSE, Operand->Info.Extended[2], NextOutput)[0] = 0;
        goto add_outbuffer_end;
        
    case M68K_OT_MEM_ABSOLUTE_L:
        AddText(DTCtx, "([S]).l", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_MEM_ABSOLUTE_W:
        AddChar(DTCtx, '(', TC_NONE);

        ConvertImmWHex(DTCtx->TextFlags | M68K_DTFLG_TRAILING_ZEROS, M68K_FALSE, (M68K_WORD)Operand->Info.Long, NextOutput)[0] = 0;
        AddText(DTCtx, NextOutput, 0, TextCase);

        AddText(DTCtx, ").w", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_MEM_INDIRECT:
    add_mem_areg:
        AddChar(DTCtx, '(', TC_NONE);

        if (!AddRegister(DTCtx, Operand->Info.Memory.Base, TextCase))
            break;

    add_mem_end:
        AddChar(DTCtx, ')', TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_MEM_INDIRECT_DISP_W:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_INDIRECT_INDEX_DISP_L:
    case M68K_OT_MEM_PC_INDIRECT_DISP_W:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B:
    case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_FALSE;

    memory:
        AddChar(DTCtx, '(', TC_NONE);

        M68K_BOOL AddSeparator = M68K_FALSE;
        M68K_BOOL IgnoreBaseDisplacement = M68K_FALSE;
        M68K_SIZE_VALUE BaseDisplacementSize = Operand->Info.Memory.Displacement.BaseSize;
        M68K_SDWORD BaseDisplacementValue = Operand->Info.Memory.Displacement.BaseValue;

        if (PreIndexed || PostIndexed)
            AddChar(DTCtx, '(', TC_NONE);

        if (Operand->Info.Memory.Base != M68K_RT_NONE)
        {
            if (!AddRegister(DTCtx, (Operand->Info.Memory.Base == M68K_RT__RPC ? M68K_RT_PC : Operand->Info.Memory.Base), TextCase))
                break;

            AddSeparator = M68K_TRUE;
        }

        // the base displacement is forced
        // 1) when it's mandatory
        // 2) for relative addresses
        // 3) when we are using zpc
        M68K_BOOL ForcedBaseDisplacement = (
            Operand->Type == M68K_OT_MEM_INDIRECT_DISP_W ||
            Operand->Type == M68K_OT_MEM_PC_INDIRECT_DISP_W ||
            Operand->Info.Memory.Base == M68K_RT__RPC ||
            (
                (Operand->Info.Memory.Base == M68K_RT_NONE || Operand->Info.Memory.Base == M68K_RT_ZPC) &&
                Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                (BaseDisplacementSize == M68K_SIZE_W || BaseDisplacementSize == M68K_SIZE_L) && /* check "case 6:" in InitAModeOperandEx */
                BaseDisplacementValue == 0 &&
                Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE
            )
        );

        // post-indexed?
        if (PostIndexed)
        {
            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_TRUE, (AddSeparator ? '+' : 0), TextCase))
                    break;

            AddChar(DTCtx, ')', TC_NONE);

            if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), '+', M68K_TRUE, TextCase))
                break;

            if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_TRUE, '+', TextCase))
                break;
        }
        // no! pre-indexed or indirect
        else
        {
            // index and scale are present?
            if (Operand->Info.Memory.Index.Register != M68K_RT_NONE)
            {
                if (!AddIndexScale(DTCtx, &(Operand->Info.Memory), (AddSeparator ? '+' : 0), M68K_TRUE, TextCase))
                    break;

                AddSeparator = M68K_TRUE;
            }

            if (!IgnoreBaseDisplacement)
                if (!AddDisplacement(DTCtx, BaseDisplacementSize, BaseDisplacementValue, ForcedBaseDisplacement, M68K_TRUE, (AddSeparator ? '+' : 0), TextCase))
                    break;

            if (PreIndexed)
            {
                AddChar(DTCtx, ')', TC_NONE);

                if (!AddDisplacement(DTCtx, Operand->Info.Memory.Displacement.OuterSize, Operand->Info.Memory.Displacement.OuterValue, M68K_FALSE, M68K_TRUE, '+', TextCase))
                    break;
            }
        }
    
        goto add_mem_end;
    
    case M68K_OT_MEM_INDIRECT_POST_INCREMENT:
        AddText(DTCtx, "<postinc> ", 0, TC_NONE);
        goto add_mem_areg;

    case M68K_OT_MEM_INDIRECT_POST_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_POST_INDEXED:
        PreIndexed = M68K_FALSE;
        PostIndexed  = M68K_TRUE;
        goto memory;

    case M68K_OT_MEM_INDIRECT_PRE_DECREMENT:
        AddText(DTCtx, "<predec> ", 0, TC_NONE);
        goto add_mem_areg;

    case M68K_OT_MEM_INDIRECT_PRE_INDEXED:
    case M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED:
        PreIndexed = M68K_TRUE;
        PostIndexed  = M68K_FALSE;
        goto memory;

    case M68K_OT_MEM_REGISTER_PAIR:
        AddText(DTCtx, "<mempair> (", 0, TC_NONE);

        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register1, TextCase))
            break;

        AddText(DTCtx, "):(", 0, M68K_INC_NONE);

        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register2, TextCase))
            break;

        AddChar(DTCtx, ')', TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_MMU_CONDITION_CODE:
        AddText(DTCtx, "<mmucc> ", 0, TC_NONE);
        AddText(DTCtx, _M68KTextMMUConditionCodes[Operand->Info.ConditionCode & (M68K_MMUCC__SIZEOF__ - 1)], 0, TextCase);
        Result = M68K_TRUE;
        break;

    case M68K_OT_OFFSET_WIDTH:
        if (Operand->Info.OffsetWidth.Offset == 0 || Operand->Info.OffsetWidth.Width == 0)
            break;
        
        AddText(DTCtx, "<offsetwidth> ", 0, TC_NONE);

        if (Operand->Info.OffsetWidth.Offset < 0)
        {
            ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(-Operand->Info.OffsetWidth.Offset - 1), NextOutput)[0] = 0;
            AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Offset, TextCase))
            break;

        AddText(DTCtx, ":", 0, TC_NONE);

        if (Operand->Info.OffsetWidth.Width < 0)
        {
            ConvertImmBHex(DTCtx->TextFlags, M68K_FALSE, (M68K_BYTE)(-Operand->Info.OffsetWidth.Width - 1), NextOutput)[0] = 0;
            AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);
        }
        else if (!AddRegister(DTCtx, Operand->Info.OffsetWidth.Width, TextCase))
            break;

        Result = M68K_TRUE;
        break;

    case M68K_OT_REGISTER:
        AddText(DTCtx, "<reg> ", 0, TC_NONE);
        Result = AddRegister(DTCtx, (M68K_REGISTER_TYPE)Operand->Info.Register, TextCase);
        break;

    case M68K_OT_REGISTER_FP_LIST:
        AddText(DTCtx, "<fplist> {", 0, TC_NONE);

        M68K_WORD FPValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
        if (FPValue != 0)
        {
            if (!AddMask(DTCtx, (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList, M68K_RT_FP0, TextCase, ',', '-'))
                break;
        }

        AddText(DTCtx, "}", 0, TC_NONE);
        Result = M68K_TRUE;
        break;
        
    case M68K_OT_REGISTER_FPCR_LIST:
        AddText(DTCtx, "<fpcrlist> {", 0, TC_NONE);

        M68K_WORD FPCRValue = (M68K_WORD)(Operand->Info.RegisterList & 7);
        if (FPCRValue != 0)
        {
            // FPCR?
            if ((FPCRValue & 1) != 0)
            {
                if (!AddRegister(DTCtx, M68K_RT_FPCR, TextCase))
                    break;

                // add separator?
                if ((FPCRValue & 6) != 0)
                    AddChar(DTCtx, ',', TextCase);
            }

            // FPSR?
            if ((FPCRValue & 2) != 0)
            {
                if (!AddRegister(DTCtx, M68K_RT_FPSR, TextCase))
                    break;

                // add separator?
                if ((FPCRValue & 4) != 0)
                    AddChar(DTCtx, ',', TextCase);
            }

            // FPIAR?
            if ((FPCRValue & 4) != 0)
                if (!AddRegister(DTCtx, M68K_RT_FPIAR, TextCase))
                    break;
        }

        AddText(DTCtx, "}", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_REGISTER_LIST:
        AddText(DTCtx, "<reglist> {", 0, TC_NONE);

        if (Operand->Info.RegisterList != 0)
        {
            M68K_WORD DValue = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
            M68K_WORD AValue =  (M68K_WORD)(Operand->Info.RegisterList >> 8);

            if (!AddMask(DTCtx, DValue, M68K_RT_D0, TextCase, ',', '-'))
                break;
            
            if (DValue != 0 && AValue != 0)
                AddChar(DTCtx, ',', TextCase);
            
            if (!AddMask(DTCtx, AValue, M68K_RT_A0, TextCase, ',', '-'))
                break;
        }
        
        AddText(DTCtx, "}", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    case M68K_OT_REGISTER_PAIR:
        AddText(DTCtx, "<regpair> ", 0, TC_NONE);

    reg_pair:
        if (!AddRegister(DTCtx, Operand->Info.RegisterPair.Register1, TextCase))
            break;

        AddChar(DTCtx, ':', TC_NONE);

        Result = AddRegister(DTCtx, Operand->Info.RegisterPair.Register2, TextCase);
        break;

    case M68K_OT_VREGISTER_PAIR_M2:
        AddText(DTCtx, "<vregpair2> ", 0, TC_NONE);
        goto reg_pair;

    case M68K_OT_VREGISTER_PAIR_M4:
        AddText(DTCtx, "<vregpair4> ", 0, TC_NONE);
        goto reg_pair;

    case M68K_OT_STATIC_KFACTOR:
        AddText(DTCtx, "<skf> ", 0, TC_NONE);

        ConvertImmBHex(DTCtx->TextFlags & ~M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, (M68K_BYTE)Operand->Info.SByte, NextOutput)[0] = 0;
        AddText(DTCtx, DTCtx->TextInfo.OutBuffer, 0, TextCase);

        Result = M68K_TRUE;
        break;

    case M68K_OT__XL_LABEL__:
        AddText(DTCtx, "label L", 0, TC_NONE);
        Result = M68K_TRUE;
        break;

    default:
        Result = M68K_FALSE;
        break;
    }

    return Result;
}

// add the register text to the output buffer
static M68K_BOOL AddRegister(PDISASM_TEXT_CONTEXT DTCtx, M68K_REGISTER_TYPE Register, TEXT_CASE TextCase)
{
    // index must be valid
    if (Register <= M68K_RT_NONE || Register >= M68K_RT__SIZEOF__)
        return M68K_FALSE;

    AddText(DTCtx, _M68KTextRegisters[Register], 0, TextCase);
    return M68K_TRUE;
}

// add the size text to the output buffer
static M68K_BOOL AddSize(PDISASM_TEXT_CONTEXT DTCtx, M68K_SIZE Size, M68K_BOOL AsmText, TEXT_CASE TextCase)
{
    // invalid?
    if (Size < M68K_SIZE_NONE || Size > M68K_SIZE__SIZEOF__)
        return M68K_FALSE;

    if (Size == M68K_SIZE_NONE)
        return M68K_TRUE;

    if (AsmText)
    {
        AddChar(DTCtx, '.', TC_NONE);
        AddChar(DTCtx, _M68KTextSizeChars[Size], TextCase);
        return M68K_TRUE;
    }
    else
    {
        DTCtx->TextInfo.Type = M68K_DTIF_SIZE;
        DTCtx->TextInfo.Details.Size = Size;
        
        return CallFunction(DTCtx, TextCase);
    }
}

// add a text to the output buffer
static void AddText(PDISASM_TEXT_CONTEXT DTCtx, PM68KC_STR Text, M68K_LUINT MaximumSize, TEXT_CASE TextCase)
{
    M68K_CHAR Char;

    if (MaximumSize == 0)
        MaximumSize = M68K_LUINT_MAX;

    for (; (Char = *Text) != 0 && MaximumSize != 0; Text++, MaximumSize--)
        AddChar(DTCtx, Char, TextCase);
}

// call the custom function; the Type and Details in "TextInfo" must be filled before calling this function
static M68K_BOOL CallFunction(PDISASM_TEXT_CONTEXT DTCtx, TEXT_CASE TextCase)
{
    // init "TextInfo" because the custom function might have changed it in the last call
    DTCtx->TextInfo.Instruction = DTCtx->Instruction;
    DTCtx->TextInfo.TextFlags = DTCtx->TextFlags;
    DTCtx->TextInfo.OutBuffer[0] = 0;

    if (!DTCtx->DisTextFunc(&(DTCtx->TextInfo), DTCtx->DisTextFuncData))
        return M68K_FALSE;

    // copy the text to the output
    AddText(DTCtx, DTCtx->TextInfo.OutBuffer, sizeof(DTCtx->TextInfo.OutBuffer), TextCase);
    return M68K_TRUE;
}

// call the custom function for a type without details
static M68K_BOOL CallFunctionNoDetails(PDISASM_TEXT_CONTEXT DTCtx, M68K_DISASM_TEXT_INFO_TYPE_VALUE Type, TEXT_CASE TextCase)
{
    DTCtx->TextInfo.Type = Type;
    return CallFunction(DTCtx, TextCase);
}

// convert a byte value to hexadecimal
static PM68K_CHAR ConvertByteHex(M68K_BYTE Value, PM68K_CHAR Output)
{
    Output[0] = _M68KTextHexChars[(Value >> 4) & 15];
    Output[1] = _M68KTextHexChars[Value & 15];
    return Output + 2;
}

// convert a double value to text
static PM68K_CHAR ConvertDouble(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_DOUBLE Double, PM68K_CHAR Output)
{
    // we need the double value as two 32-bit values
    //
    //  bit     3210987654321098765432109876543210987654321098765432109876543210
    //          seeeeeeeeeeemmmmmmmmmmmmmmmmmmmm
    //                                          mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
    //
    //  s       sign
    //  e..e    exponent
    //  m..m    mantissa with an implicit 1, except when the number is denormalized
    //          
    // special values:
    //
    //  +0      s = 0, e..e = 0, m..m = 0
    //  -0      s = 1, e..e = 0, m..m = 0
    //  +denorm s = 0, e..e = 0, m..m != 0
    //  -denorm s = 1, e..e = 0, m..m != 0
    //  +inf    s = 0, e..e = 1, m..m = 0
    //  -inf    s = 1, e..e = 1, m..m = 0
    //  +nan    s = 0, e..e = 1, m..m != 0
    //  -nan    s = 1, e..e = 1, m..m != 0
    //  +norm   s = 0, e..e != 0
    //  -norm   s = 1, e..e != 0
    //
    M68K_BOOL Normalized;
    M68K_SWORD FinalExponent;

#ifdef M68K_TARGET_IS_BIG_ENDIAN
    M68K_DWORD Value0 = ((PM68K_DWORD)Double)[0];
    M68K_DWORD Value1 = ((PM68K_DWORD)Double)[1];
#else
    M68K_DWORD Value0 = ((PM68K_DWORD)Double)[1];
    M68K_DWORD Value1 = ((PM68K_DWORD)Double)[0];
#endif
    M68K_DWORD Exponent = (M68K_DWORD)((Value0 >> 20) & 0x7ff);
    M68K_DWORD MantissaHigh = (M68K_DWORD)((Value0 << 12) | (Value1 >> 20));
    M68K_DWORD MantissaLow = (M68K_DWORD)(Value1 << 12);

    // define the first chars
    if (!ForXL)
    {
        Output = CopyImmPrefix(TextFlags, Output);
        *(Output++) = ((M68K_SDWORD)Value0 < 0 ? '-' : '+');
    }
    else if (!(
        (Exponent == 0 && MantissaHigh == 0 && MantissaLow == 0) ||     // M68K_IEEE_VT_ZERO
        Exponent == 0x7ff                                               // M68K_IEEE_VT_INF / M68K_IEEE_VT_NAN
        ))
        *(Output++) = ((M68K_SDWORD)Value0 < 0 ? '-' : '+');
    else
        Output = CopyText("<double> ", Output);

    // e..e = 0?
    if (Exponent == 0)
    {
        // m..m = 0?
        if (MantissaHigh == 0 && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_ZERO, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
        else
        {
            // denormalized i.e. with an implicit 0
            Normalized = M68K_FALSE;
            FinalExponent = -1022;
            goto generate_double_mantissa_exponent;
        }
    }
    else if (Exponent == 0x7ff)
    {
        // m..m = 0?
        if (MantissaHigh == 0 && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_INF, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
        else
            Output = CopyIEEEValue(M68K_IEEE_VT_NAN, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
    }
    else
    {
        // normalized i.e. with an implicit 1
        Normalized = M68K_TRUE;

        // calculate the final exponent
        FinalExponent = (M68K_SWORD)(Exponent - 1023);

generate_double_mantissa_exponent:
        {
            M68K_CHAR LocalBuffer[17];

            // convert the mantissa to hexadecimal
            ConvertDWordHex(MantissaHigh, LocalBuffer);
            ConvertDWordHex(MantissaLow, LocalBuffer + 8);

            // the mantissa uses 52 bits which means it requires 13 hexadecimal chars (13 * 4 == 52);
            // all zeros at the end are ignored
            SkipEndZeros(LocalBuffer, LocalBuffer + (13 - 1))[1] = 0;

            Output = CopyMantissaExponent(TextFlags, Normalized, LocalBuffer, FinalExponent, Output);
        }
    }

    return Output;
}

// convert a dword value to hexadecimal
static PM68K_CHAR ConvertDWordHex(M68K_DWORD Value, PM68K_CHAR Output)
{
    ConvertWordHex((M68K_WORD)(Value >> 16), Output);
    return ConvertWordHex((M68K_WORD)Value, Output + 4);
}

// convert an extended value to text
static PM68K_CHAR ConvertExtended(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_EXTENDED Extended, PM68K_CHAR Output)
{
    // we need the extended value as three 32-bit values
    //
    //  bit     543210987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210
    //          seeeeeeeeeeeeeeezzzzzzzzzzzzzzzz
    //                                          ijmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
    //                                                                          mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
    //
    //  s       sign
    //  e..e    exponent
    //  z..z    zeros
    //  i       explicit integer bit i.e. 0 or 1
    //  j..m    mantissa (j is the upper bit of the mantissa)
    //          
    // special values:
    //
    //  +0      s = 0, e..e = 0, i = 0, j..m = 0
    //  -0      s = 1, e..e = 0, i = 0, j..m = 0
    //  +denorm s = 0, e..e = 0, i = -, j..m != 0
    //  -denorm s = 1, e..e = 0, i = -, j..m != 0
    //  +pinf   s = 0, e..e = 1, i = 0, j..m = 0
    //  -pinf   s = 1, e..e = 1, i = 0, j..m = 0
    //  +pnan   s = 0, e..e = 1, i = 0, j..m != 0
    //  -pnan   s = 1, e..e = 1, i = 0, j..m != 0
    //  +inf    s = 0, e..e = 1, i = 1, j..m = 0
    //  -inf    s = 1, e..e = 1, i = 1, j..m = 0
    //  +nan    s = 0, e..e = 1, i = 1, j = 0, m..m != 0
    //  -nan    s = 1, e..e = 1, i = 1, j = 0, m..m != 0
    //  +ind    s = 0, e..e = 1, i = 1, j = 1, m..m = 0
    //  -ind    s = 1, e..e = 1, i = 1, j = 1, m..m = 0
    //  +qnan   s = 0, e..e = 1, i = 1, j = 1, m..m != 0
    //  -qnan   s = 1, e..e = 1, i = 1, j = 1, m..m != 0
    //  +norm   s = 0, e..e != 0
    //  -norm   s = 1, e..e != 0
    //
    M68K_BOOL Normalized;
    M68K_SWORD FinalExponent;

    M68K_DWORD Value0 = (*Extended)[0];
    M68K_DWORD MantissaHigh = (*Extended)[1];
    M68K_DWORD MantissaLow = (*Extended)[2];
    M68K_DWORD Exponent = (M68K_DWORD)((Value0 >> 16) & 0x7fff);

    // define the first chars
    Output = CopyImmPrefix(TextFlags, Output);
    *(Output++) = ((M68K_SDWORD)Value0 < 0 ? '-' : '+');

    // e..e = 0?
    if (Exponent == 0)
    {
        // i = 0 and j..m = 0?
        if (MantissaHigh == 0 && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_ZERO, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
        else
        {
            FinalExponent = -16382;
            goto generate_extended_mantissa_exponent;
        }
    }
    else if (Exponent == 0x7fff)
    {
        // i = 0 and j..m = 0?
        if (MantissaHigh == 0 && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_PINF, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
        // i = 0?
        else if ((M68K_SDWORD)MantissaHigh >= 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_PNAN, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
                    
        // i = 1 && j..m == 0?
        else if (MantissaHigh == 0x80000000UL && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_INF, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
                    
        // i = 1 && j = 0 && m..m != 0?
        else if ((MantissaHigh & 0x40000000UL) == 0 && ((MantissaHigh & 0xbfffffffUL) != 0 || MantissaLow != 0))
            Output = CopyIEEEValue(M68K_IEEE_VT_NAN, ForXL, ((M68K_SDWORD)Value0 < 0), Output);

        // i = 1 && j = 1 && m..m = 0?
        else if (MantissaHigh == 0xc0000000UL && MantissaLow == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_IND, ForXL, ((M68K_SDWORD)Value0 < 0), Output);

        // i = 1 && j = 1 && m..m != 0?
        else
            Output = CopyIEEEValue(M68K_IEEE_VT_QNAN, ForXL, ((M68K_SDWORD)Value0 < 0), Output);
    }
    //  s = 0, e..e != 0
    else
    {
        // calculate the final exponent
        FinalExponent = (M68K_SWORD)(Exponent - 16383);

generate_extended_mantissa_exponent:
        {
            M68K_CHAR LocalBuffer[25];

            // the upper bit in the mantissa is the implicit bit
            Normalized = ((M68K_SDWORD)MantissaHigh < 0);
            MantissaHigh = (MantissaHigh << 1) | (MantissaLow >> 31);
            MantissaLow <<= 1;

            // convert the mantissa to hexadecimal
            ConvertDWordHex(MantissaHigh, LocalBuffer);
            ConvertDWordHex(MantissaLow, LocalBuffer + 8);

            // the mantissa uses 63 bits which means it requires 16 hexadecimal chars (16 * 4 >= 63)
            // all zeros at the end are ignored
            SkipEndZeros(LocalBuffer, LocalBuffer + (16 - 1))[1] = 0;
            
            Output = CopyMantissaExponent(TextFlags, Normalized, LocalBuffer, FinalExponent, Output);
        }
    }
    return Output;
}

// convert an 8-bit immediate value to hexadecimal; immediate values use flags to generate the text
static PM68K_CHAR ConvertImmBHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_BYTE Value, PM68K_CHAR Output)
{
    // negative value?
    M68K_BOOL Negative = ((TextFlags & M68K_DTFLG_IGNORE_SIGN) == 0 && (M68K_SBYTE)Value < 0);
    if (Negative)
        Value = (M68K_BYTE)-(M68K_SBYTE)Value;

    // copy the prefixes
    M68K_BOOL TrailingZeros = ((TextFlags & M68K_DTFLG_TRAILING_ZEROS) != 0);
    Output = CopyHexPrefixes(TextFlags, AsDisplacement, Negative, (TrailingZeros || Value >= 10), Output);

    // include the trailing zeros?
    if (TrailingZeros)
        Output = ConvertByteHex(Value, Output);
    else
    {
        // convert using a local buffer and put a \0 at the end
        M68K_CHAR LocalBuffer[3];
        
        ConvertByteHex(Value, LocalBuffer);
        LocalBuffer[2] = 0;

        // skip the zeros at start and copy to the output
        Output = CopyText(SkipStartZeros(LocalBuffer, LocalBuffer + 1), Output);
    }

    return Output;
}

// convert a 32-bit immediate value to hexadecimal; immediate values use flags to generate the text
static PM68K_CHAR ConvertImmLHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_DWORD Value, PM68K_CHAR Output)
{
    // negative value?
    M68K_BOOL Negative = ((TextFlags & M68K_DTFLG_IGNORE_SIGN) == 0 && (M68K_SDWORD)Value < 0);
    if (Negative)
        Value = (M68K_DWORD)-(M68K_SDWORD)Value;

    // copy the prefixes
    M68K_BOOL TrailingZeros = ((TextFlags & M68K_DTFLG_TRAILING_ZEROS) != 0);
    Output = CopyHexPrefixes(TextFlags, AsDisplacement, Negative, (TrailingZeros || Value >= 10), Output);

    // include the trailing zeros?
    if ((TextFlags & M68K_DTFLG_TRAILING_ZEROS) == 0)
    {
        // convert using a local buffer and put a \0 at the end
        M68K_CHAR LocalBuffer[9];
        
        ConvertDWordHex(Value, LocalBuffer);
        LocalBuffer[8] = 0;

        // skip the zeros at start and copy to the output
        Output = CopyText(SkipStartZeros(LocalBuffer, LocalBuffer + 7), Output);
    }
    else
        Output = ConvertDWordHex(Value, Output);

    return Output;
}

// convert a 16-bit immediate value to hexadecimal; immediate values use flags to generate the text
static PM68K_CHAR ConvertImmWHex(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_WORD Value, PM68K_CHAR Output)
{
    // negative value?
    M68K_BOOL Negative = ((TextFlags & M68K_DTFLG_IGNORE_SIGN) == 0 && (M68K_SWORD)Value < 0);
    if (Negative)
        Value = (M68K_WORD)-(M68K_SWORD)Value;

    // copy the prefixes
    M68K_BOOL TrailingZeros = ((TextFlags & M68K_DTFLG_TRAILING_ZEROS) != 0);
    Output = CopyHexPrefixes(TextFlags, AsDisplacement, Negative, (TrailingZeros || Value >= 10), Output);

    // include the trailing zeros?
    if ((TextFlags & M68K_DTFLG_TRAILING_ZEROS) == 0)
    {
        // convert using a local buffer and put a \0 at the end
        M68K_CHAR LocalBuffer[5];
        
        ConvertWordHex(Value, LocalBuffer);
        LocalBuffer[4] = 0;

        // skip the zeros at start and copy to the output
        Output = CopyText(SkipStartZeros(LocalBuffer, LocalBuffer + 3), Output);
    }
    else
        Output = ConvertWordHex(Value, Output);

    return Output;
}

// convert a packed decimal value to text
static PM68K_CHAR ConvertPackedDecimal(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_PACKED_DECIMAL PackedDecimal, PM68K_CHAR Output)
{
    M68K_CHAR LocalBuffer[25];

    Output = CopyImmPrefix(TextFlags, Output);

    // packed-decimal values are "not supported" and we write them as a 96-bit hexadecimal number
    ConvertDWordHex((*PackedDecimal)[0], LocalBuffer);
    ConvertDWordHex((*PackedDecimal)[1], LocalBuffer + 8);
    ConvertDWordHex((*PackedDecimal)[2], LocalBuffer + 16);
    LocalBuffer[24] = 0;
            
    Output = CopyHexPrefixes(TextFlags, M68K_FALSE, M68K_FALSE, M68K_TRUE, Output);
    Output = CopyText(LocalBuffer, Output);
    
    return Output;
}

// convert a quad value to text
static PM68K_CHAR ConvertQuad(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_QUAD Quad, PM68K_CHAR Output)
{
    M68K_CHAR LocalBuffer[17];

    Output = CopyImmPrefix(TextFlags, Output);

    // quad values are always unsigned
    ConvertDWordHex((*Quad)[0], LocalBuffer);
    ConvertDWordHex((*Quad)[1], LocalBuffer + 8);
    LocalBuffer[16] = 0;
            
    Output = CopyHexPrefixes(TextFlags, M68K_FALSE, M68K_FALSE, M68K_TRUE, Output);
    Output = CopyText(LocalBuffer, Output);

    return Output;
}

// convert a single value to text
static PM68K_CHAR ConvertSingle(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL ForXL, PM68K_SINGLE Single, PM68K_CHAR Output, PM68K_BOOL AddSuffix)
{
    // we need the single value as a 32-bit value
    //
    //  bit     10987654321098765432109876543210
    //          seeeeeeeemmmmmmmmmmmmmmmmmmmmmmm
    //
    //  s       sign
    //  e..e    exponent
    //  m..m    mantissa with an implicit 1, except when the number is denormalized
    //          
    // special values:
    //
    //  +0      s = 0, e..e = 0, m..m = 0
    //  -0      s = 1, e..e = 0, m..m = 0
    //  +denorm s = 0, e..e = 0, m..m != 0
    //  -denorm s = 1, e..e = 0, m..m != 0
    //  +inf    s = 0, e..e = 1, m..m = 0
    //  -inf    s = 1, e..e = 1, m..m = 0
    //  +nan    s = 0, e..e = 1, m..m != 0
    //  -nan    s = 1, e..e = 1, m..m != 0
    //  +norm   s = 0, e..e != 0
    //  -norm   s = 1, e..e != 0
    //
    M68K_BOOL Normalized;
    M68K_SWORD FinalExponent;

    M68K_DWORD Value = ((PM68K_DWORD)Single)[0];
    M68K_DWORD Exponent = (M68K_DWORD)(M68K_BYTE)(Value >> 23);
    M68K_DWORD Mantissa = (M68K_DWORD)(Value << 9);

    if (AddSuffix != M68K_NULL)
        *AddSuffix = M68K_FALSE;

    // define the first chars
    if (!ForXL)
    {
        Output = CopyImmPrefix(TextFlags, Output);
        *(Output++) = ((M68K_SDWORD)Value < 0 ? '-' : '+');
    }
    else if (!(
        (Exponent == 0 && Mantissa == 0) || // M68K_IEEE_VT_ZERO
        Exponent == 0xff                    // M68K_IEEE_VT_INF / M68K_IEEE_VT_NAN
        ))
    {
        *(Output++) = ((M68K_SDWORD)Value < 0 ? '-' : '+');

        if (AddSuffix != M68K_NULL)
            *AddSuffix = M68K_TRUE;
    }
    else
        Output = CopyText("<single> ", Output);

    // e..e = 0?
    if (Exponent == 0)
    {
        // m..m = 0?
        if (Mantissa == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_ZERO, ForXL, ((M68K_SDWORD)Value < 0), Output);
        else
        {
            // denormalized i.e. with an implicit 0
            Normalized = M68K_FALSE;
            FinalExponent = -126;
            goto generate_single_mantissa_exponent;
        }
    }
    else if (Exponent == 0xff)
    {
        // m..m = 0?
        if (Mantissa == 0)
            Output = CopyIEEEValue(M68K_IEEE_VT_INF, ForXL, ((M68K_SDWORD)Value < 0), Output);
        else
            Output = CopyIEEEValue(M68K_IEEE_VT_NAN, ForXL, ((M68K_SDWORD)Value < 0), Output);
    }
    else
    {
        // normalized i.e. with an implicit 1
        Normalized = M68K_TRUE;

        // calculate the final exponent
        FinalExponent = (M68K_SWORD)(M68K_SBYTE)(Exponent - 127);

generate_single_mantissa_exponent:
        {
            M68K_CHAR LocalBuffer[9];
            
            // convert the mantissa to hexadecimal
            ConvertDWordHex(Mantissa, LocalBuffer);

            // the mantissa uses 23 bits which means it requires 6 hexadecimal chars (6 * 4 = 24 >= 23);
            // all zeros at the end are ignored
            SkipEndZeros(LocalBuffer, LocalBuffer + (6 - 1))[1] = 0;
            
            Output = CopyMantissaExponent(TextFlags, Normalized, LocalBuffer, FinalExponent, Output);
        }
    }

    return Output;
}

// convert a word value to decimal
static PM68K_CHAR ConvertWordDec(M68K_WORD Value, PM68K_CHAR Output)
{
    if (Value != 0)
    {
        M68K_BOOL ForceQuotient = M68K_FALSE;

        for (M68K_WORD DivFactor = 10000; DivFactor != 0; DivFactor /= 10)
        {
            M68K_WORD Quotient = (Value / DivFactor);
            if (Quotient != 0)
            {
                Value -= Quotient * DivFactor;
                ForceQuotient = M68K_TRUE;
            }

            if (ForceQuotient)
                *(Output++) = (M68K_CHAR)('0' + Quotient);
        }
    }
    else
        *(Output++) = '0';

    return Output;
}

// convert a word value to hexadecimal
static PM68K_CHAR ConvertWordHex(M68K_WORD Value, PM68K_CHAR Output)
{
    ConvertByteHex((M68K_BYTE)(Value >> 8), Output);
    return ConvertByteHex((M68K_BYTE)Value, Output + 2);
}

// copy the hex prefixs to the output buffer
static PM68K_CHAR CopyHexPrefixes(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL AsDisplacement, M68K_BOOL NegativeValue, M68K_BOOL ValueHigher10, PM68K_CHAR Output)
{
    // add the sign?
    if (NegativeValue)
        *(Output++) = '-';
    else if (AsDisplacement)
        *(Output++) = '+';

    if (ValueHigher10)
    {
        if ((TextFlags & M68K_DTFLG_C_HEXADECIMALS) != 0)
        {
            Output[0] = '0';
            Output[1] = 'x';
            Output += 2;
        }
        else
            *(Output++) = '$';
    }

    return Output;
}

// copy the IEEE special texts to the output
static PM68K_CHAR CopyIEEEValue(M68K_IEEE_VALUE_TYPE IEEEValue, M68K_BOOL ForXL, M68K_BOOL Negative, PM68K_CHAR Output)
{
    if (ForXL && Negative)
        Output = CopyText("N", Output);

    return CopyText((ForXL ? _M68KTextIEEEValuesXL : _M68KTextIEEEValues)[IEEEValue], Output);
}

// copy the immediate prefix to the output prefix
static PM68K_CHAR CopyImmPrefix(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, PM68K_CHAR Output)
{
    // ignore it?
    if ((TextFlags & M68K_DTFLG_HIDE_IMMEDIATE_PREFIX) != 0)
        return Output;

    *Output = '#';
    return Output + 1;
}

// copy the mantissa and exponent text to the output buffer
static PM68K_CHAR CopyMantissaExponent(M68K_DISASM_TEXT_FLAGS_VALUE TextFlags, M68K_BOOL Normalized, PM68K_CHAR ConvertedMantissa, M68K_SWORD Exponent, PM68K_CHAR Output)
{
    // hex prefix
    Output = CopyHexPrefixes(TextFlags, M68K_FALSE, M68K_FALSE, M68K_TRUE, Output);

    // 0. or 1.
    Output[0] = (Normalized ? '1' : '0');
    Output[1] = '.';
                    
    // mantissa
    Output = CopyText(ConvertedMantissa, Output + 2);

    // power of two and exponent sign
    Output[0] = 'p';

    if (Exponent < 0)
    {
        Exponent = -Exponent;
        Output[1] = '-';
    }
    else
        Output[1] = '+';

    Output = ConvertWordDec((M68K_WORD)Exponent, Output + 2);
    return Output;
}

// copy text to the output buffer
static PM68K_CHAR CopyText(PM68KC_STR Text, PM68K_CHAR Output)
{
    M68K_CHAR Char;
    
    while ((Char = *Text) != 0)
    {
        *(Output++) = Char;
        Text++;
    }

    return Output;
}

// disassemble for a different language
static M68K_LUINT DisassembleFor(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/, PADD_OPERAND_FUNC AddOperandFunc)
{
    // check the parameters
    if (Instruction == M68K_NULL || OutSize <= 0)
        return 0;

    // invalid instructions are never converted
    if (Instruction->Type == M68K_IT_INVALID || Instruction->Type >= M68K_IT__SIZEOF__)
        return 0;

    // init
    DISASM_TEXT_CONTEXT DTCtx;
    
    DTCtx.Address = Address;
    DTCtx.Instruction = Instruction;
    DTCtx.Output.Buffer = OutBuffer;
    DTCtx.Output.AvailSize = (OutBuffer != M68K_NULL ? OutSize : 0);
    DTCtx.Output.TotalSize = 0;
    DTCtx.TextFlags = (Flags | M68K_DTFLG_C_HEXADECIMALS | M68K_DTFLG_SCALE_1 | M68K_DTFLG_CONDITION_CODE_AS_OPERAND | M68K_DTFLG_HIDE_IMMEDIATE_PREFIX);

    // all in lowercase?
    TEXT_CASE TextCase;
    if ((DTCtx.TextFlags & M68K_DTFLG_ALL_LOWERCASE) != 0)
        TextCase = TC_LOWER;

    // all in uppercase?
    else if ((DTCtx.TextFlags & M68K_DTFLG_ALL_UPPERCASE) != 0)
        TextCase = TC_UPPER;

    else
        TextCase = TC_NONE;

    // start with the mnemonic
    M68K_INSTRUCTION_TYPE_VALUE Type = Instruction->Type >= M68K_IT_INVALID && Instruction->Type < M68K_IT__SIZEOF__ ? Instruction->Type : M68K_IT_INVALID;
    AddText(&DTCtx, _M68KTextMnemonics[Type], 0, TextCase);

    // requires a size?
    if (Instruction->Size != M68K_SIZE_NONE)
    {
        if ((_M68KInstrMasterTypeFlags[Type] & IMF_IMPLICIT_SIZE) == 0 || (Flags & M68K_DTFLG_HIDE_IMPLICIT_SIZES) == 0)
        {
            if (Instruction->Size < M68K_SIZE_NONE || Instruction->Size >= M68K_SIZE__SIZEOF__)
                return 0;

            AddChar(&DTCtx, '.', TC_NONE);
            AddChar(&DTCtx, _M68KTextSizeChars[Instruction->Size], TC_NONE);
        }
    }

    // separator
    AddChar(&DTCtx, ' ', TC_NONE);

    // write all operands
    M68K_BOOL IgnoreNextSeparator = M68K_TRUE;
    PM68K_OPERAND Operand = Instruction->Operands;

    for (M68K_UINT Index = 0; Index < M68K_MAXIMUM_NUMBER_OPERANDS; Index++, Operand++)
    {
        // reached the last operand? this is identified by an operand with a type M68K_OT_NONE
        if (Operand->Type == M68K_OT_NONE)
            break;

        // write a separator?
        if (!IgnoreNextSeparator)
            AddText(&DTCtx, ", ", 0, TC_NONE);

        if (!AddOperandFunc(&DTCtx, Operand, TextCase, &IgnoreNextSeparator))
            return 0;
    }

    AddChar(&DTCtx, 0, TC_NONE);
    return DTCtx.Output.TotalSize;
}

// skip the zeros at the end of the converted text for an hexadecimal number
static PM68K_CHAR SkipEndZeros(PM68K_CHAR Start, PM68K_CHAR Last)
{
    for (;Start < Last; Last--)
        if (*Last != '0')
            break;

    return Last;
}

// skip the zeros at the start of the converted text for an hexadecimal number
static PM68K_CHAR SkipStartZeros(PM68K_CHAR Start, PM68K_CHAR Last)
{
    for (;Start < Last; Start++)
        if (*Start != '0')
            break;

    return Start;
}

// default function used to disassemble an instruction
M68K_BOOL M68KDefaultDisassembleTextFunc(PM68K_DISASM_TEXT_INFO TextInfo, PM68K_VOID Data)
{
    M68K_BOOL Result = M68K_FALSE;

    // function data is not used
    Data = M68K_NULL;

    if (TextInfo != M68K_NULL)
    {
        PM68K_CHAR NextOutput;

        switch (TextInfo->Type)
        {
        case M68K_DTIF_ADDRESS_L:
            ConvertImmLHex(TextInfo->TextFlags | M68K_DTFLG_TRAILING_ZEROS | M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, TextInfo->Details.Long, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_ADDRESS_W:
            ConvertImmWHex(TextInfo->TextFlags | M68K_DTFLG_TRAILING_ZEROS | M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, TextInfo->Details.Word, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_CONDITION_CODE:
            CopyText(_M68KTextConditionCodes[TextInfo->Details.ConditionCode & (M68K_CC__SIZEOF__ - 1)], TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_FPU_CONDITION_CODE:
            CopyText(_M68KTextFpuConditionCodes[TextInfo->Details.ConditionCode & (M68K_FPCC__SIZEOF__ - 1)], TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_COPROCESSOR_CONDITION_CODE:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmBHex(TextInfo->TextFlags, M68K_FALSE, (M68K_BYTE)(TextInfo->Details.CoprocessorCC & 0x3f), NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_COPROCESSOR_ID:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmBHex(TextInfo->TextFlags, M68K_FALSE, (M68K_BYTE)(TextInfo->Details.CoprocessorId & 7), NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_DISPLACEMENT_B:
            ConvertImmBHex(TextInfo->TextFlags, M68K_TRUE, (M68K_BYTE)(M68K_SBYTE)TextInfo->Details.Displacement, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_DISPLACEMENT_L:
            ConvertImmLHex(TextInfo->TextFlags, M68K_TRUE, (M68K_DWORD)TextInfo->Details.Displacement, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_DISPLACEMENT_W:
            ConvertImmWHex(TextInfo->TextFlags, M68K_TRUE, (M68K_WORD)(M68K_SWORD)TextInfo->Details.Displacement, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_B:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmBHex(TextInfo->TextFlags, M68K_FALSE, TextInfo->Details.Byte, NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_D:
            ConvertDouble(TextInfo->TextFlags, M68K_FALSE, &(TextInfo->Details.Double), TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_L:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmLHex(TextInfo->TextFlags, M68K_FALSE, TextInfo->Details.Long, NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_P:
            ConvertPackedDecimal(TextInfo->TextFlags, &(TextInfo->Details.PackedDecimal), TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_Q:
            ConvertQuad(TextInfo->TextFlags, &(TextInfo->Details.Quad), TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_S:
            ConvertSingle(TextInfo->TextFlags, M68K_FALSE, &(TextInfo->Details.Single), TextInfo->OutBuffer, M68K_NULL)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_W:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmWHex(TextInfo->TextFlags, M68K_FALSE, TextInfo->Details.Word, NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_IMMEDIATE_X:
            ConvertExtended(TextInfo->TextFlags, M68K_FALSE, &(TextInfo->Details.Extended), TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_INSTRUCTION_MNEMONIC:
            {
                // the instruction type must be valid
                M68K_INSTRUCTION_TYPE Type = (M68K_INSTRUCTION_TYPE)TextInfo->Instruction->Type;
                if (Type > M68K_IT_INVALID && Type < M68K_IT__SIZEOF__)
                {
                    NextOutput = TextInfo->OutBuffer;
                    
                    // mnemonic
                    NextOutput = CopyText(_M68KTextMnemonics[Type], NextOutput);

                    // add the condition code?
                    if ((TextInfo->TextFlags & M68K_DTFLG_CONDITION_CODE_AS_OPERAND) == 0 && (_M68KInstrMasterTypeFlags[Type] & IMF_CONDITION_CODE) != 0)
                    {
                        // first operand is a condition code?
                        if (TextInfo->Instruction->Operands[0].Type == M68K_OT_CONDITION_CODE ||
                            TextInfo->Instruction->Operands[0].Type == M68K_OT_FPU_CONDITION_CODE ||
                            TextInfo->Instruction->Operands[0].Type == M68K_OT_MMU_CONDITION_CODE)
                        {
                            // the last two chars are cc?
                            if (NextOutput >= (TextInfo->OutBuffer + 2))
                            {
                                if (NextOutput[-1] == 'c' && NextOutput[-2] == 'c')
                                {
                                    PM68KC_STR CCText;
                                    if (TextInfo->Instruction->Operands[0].Type == M68K_OT_CONDITION_CODE)
                                        CCText = _M68KTextConditionCodes[TextInfo->Instruction->Operands[0].Info.ConditionCode & (M68K_CC__SIZEOF__ - 1)];
                                    else if (TextInfo->Instruction->Operands[0].Type == M68K_OT_FPU_CONDITION_CODE)
                                        CCText = _M68KTextFpuConditionCodes[TextInfo->Instruction->Operands[0].Info.FpuConditionCode & (M68K_FPCC__SIZEOF__ - 1)];
                                    else if (TextInfo->Instruction->Operands[0].Type == M68K_OT_MMU_CONDITION_CODE)
                                        CCText = _M68KTextMMUConditionCodes[TextInfo->Instruction->Operands[0].Info.MMUConditionCode & (M68K_MMUCC__SIZEOF__ - 1)];
                                    else
                                        CCText = M68K_NULL;

                                    if (CCText != M68K_NULL)
                                        NextOutput = CopyText(CCText, NextOutput - 2);
                                }
                            }
                        }
                    }

                    *NextOutput = 0;
                    Result = M68K_TRUE;
                }
            }
            break;

        case M68K_DTIF_OFFSET_WIDTH_VALUE:
            if (TextInfo->Details.Byte <= 32)
            {
                ConvertWordDec((M68K_WORD)TextInfo->Details.Byte, TextInfo->OutBuffer)[0] = 0;
                Result = M68K_TRUE;
            }
            break;

        case M68K_DTIF_MMU_CONDITION_CODE:
            CopyText(_M68KTextMMUConditionCodes[TextInfo->Details.ConditionCode & (M68K_MMUCC__SIZEOF__ - 1)], TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_OPCODE_WORD:
            ConvertWordHex(TextInfo->Details.Word, TextInfo->OutBuffer)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_SCALE:
            {
                M68K_SCALE Scale = (M68K_SCALE)TextInfo->Details.Scale;
                if ((Scale == M68K_SCALE_1 && (TextInfo->TextFlags & M68K_DTFLG_SCALE_1) != 0) || Scale > M68K_SCALE_1)
                {
                    if (Scale >= M68K_SCALE_1 && Scale < M68K_SCALE__SIZEOF__)
                    {
                        TextInfo->OutBuffer[0] = '*';
                        TextInfo->OutBuffer[1] = _M68KTextScaleChars[(M68K_UINT)Scale - (M68K_UINT)M68K_SCALE_1];
                        TextInfo->OutBuffer[2] = 0;
                        Result = M68K_TRUE;
                    }
                }
                else if (Scale == M68K_SCALE_1)
                {
                    TextInfo->OutBuffer[0] = 0;
                    Result = M68K_TRUE;
                }
            }
            break;

        case M68K_DTIF_SIZE:
            {
                M68K_SIZE Size = (M68K_SIZE)TextInfo->Details.Size;
                if (Size > M68K_SIZE_NONE && Size < M68K_SIZE__SIZEOF__)
                {
                    TextInfo->OutBuffer[0] = '.';
                    TextInfo->OutBuffer[1] = _M68KTextSizeChars[Size];
                    TextInfo->OutBuffer[2] = 0;
                    Result = M68K_TRUE;
                }
                else if (Size == M68K_SIZE_NONE)
                {
                    TextInfo->OutBuffer[0] = 0;
                    Result = M68K_TRUE;
                }
            }
            break;

        case M68K_DTIF_STATIC_KFACTOR:
            NextOutput = CopyImmPrefix(TextInfo->TextFlags, TextInfo->OutBuffer);
            ConvertImmBHex(TextInfo->TextFlags & ~M68K_DTFLG_IGNORE_SIGN, M68K_FALSE, (M68K_BYTE)TextInfo->Details.SByte, NextOutput)[0] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_TEXT_COPROCESSOR_END:
            TextInfo->OutBuffer[0] = '>';
single_char:
            TextInfo->OutBuffer[1] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_TEXT_COPROCESSOR_SEPARATOR:
            TextInfo->OutBuffer[0] = ':';
            goto single_char;

        case M68K_DTIF_TEXT_COPROCESSOR_START:
            TextInfo->OutBuffer[0] = '<';
            goto single_char;

        case M68K_DTIF_TEXT_IMMEDIATE_PREFIX:
            TextInfo->OutBuffer[0] = '#';
            goto single_char;

        case M68K_DTIF_TEXT_INNER_MEMORY_END:
            TextInfo->OutBuffer[0] = ']';
            goto single_char;

        case M68K_DTIF_TEXT_INNER_MEMORY_START:
            TextInfo->OutBuffer[0] = '[';
            goto single_char;

        case M68K_DTIF_TEXT_KFACTOR_END:
            TextInfo->OutBuffer[0] = '}';
            goto single_char;

        case M68K_DTIF_TEXT_KFACTOR_START:
            TextInfo->OutBuffer[0] = '{';
            goto single_char;

        case M68K_DTIF_TEXT_MEMORY_ADD:
            TextInfo->OutBuffer[0] = '+';
            goto single_char;

        case M68K_DTIF_TEXT_MEMORY_END:
            TextInfo->OutBuffer[0] = ')';
            goto single_char;

        case M68K_DTIF_TEXT_MEMORY_POST_INCREMENT:
            TextInfo->OutBuffer[0] = '+';
            goto single_char;

        case M68K_DTIF_TEXT_MEMORY_PRE_DECREMENT:
            TextInfo->OutBuffer[0] = '-';
            goto single_char;

        case M68K_DTIF_TEXT_MEMORY_START:
            TextInfo->OutBuffer[0] = '(';
            goto single_char;

        case M68K_DTIF_TEXT_OFFSET_WIDTH_END:
            TextInfo->OutBuffer[0] = '}';
            goto single_char;

        case M68K_DTIF_TEXT_OFFSET_WIDTH_SEPARATOR:
            TextInfo->OutBuffer[0] = ':';
            goto single_char;

        case M68K_DTIF_TEXT_OFFSET_WIDTH_START:
            TextInfo->OutBuffer[0] = '{';
            goto single_char;

        case M68K_DTIF_TEXT_OPCODE_WORD_SEPARATOR:
        case M68K_DTIF_TEXT_PADDING:
        case M68K_DTIF_TEXT_SEPARATOR:
            TextInfo->OutBuffer[0] = ' ';
            goto single_char;

        case M68K_DTIF_TEXT_OPCODE_WORD_UNKNOWN:
            TextInfo->OutBuffer[0] = '?';
            TextInfo->OutBuffer[1] = '?';
            TextInfo->OutBuffer[2] = '?';
            TextInfo->OutBuffer[3] = '?';
            TextInfo->OutBuffer[4] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_TEXT_OPERAND_SEPARATOR:
            TextInfo->OutBuffer[0] = ',';
            TextInfo->OutBuffer[1] = ' ';
            TextInfo->OutBuffer[2] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_TEXT_RELATIVE_ADDRESS_PREFIX:
            TextInfo->OutBuffer[0] = '&';
            TextInfo->OutBuffer[1] = 0;
            Result = M68K_TRUE;
            break;

        case M68K_DTIF_TEXT_PAIR_SEPARATOR:
            TextInfo->OutBuffer[0] = ':';
            goto single_char;

        case M68K_DTIF_UNKNOWN:
        default:
            break;
        }
    }

    return Result;
}

// convert a disassembled instruction to text
M68K_LUINT M68KDisassembleText(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, PM68KC_STR Format /*M68K_NULL = use default*/, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize)
{
    return M68KDisassembleTextEx(Address, Instruction, Format, Flags, M68KDefaultDisassembleTextFunc, M68K_NULL, OutBuffer, OutSize);
}

// convert a disassembled instruction to text using a custom function
M68K_LUINT M68KDisassembleTextEx(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, PM68KC_STR Format /*M68K_NULL = use default*/, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_DISASSEMBLE_TEXT_FUNCTION DisTextFunc, PM68K_VOID DisTextFuncData, PM68K_CHAR OutBuffer, M68K_LUINT OutSize)
{
    // check the parameters
    if (Instruction == M68K_NULL || DisTextFunc == M68K_NULL || OutSize <= 0)
        return 0;

    // invalid instructions are never converted
    if (Instruction->Type == M68K_IT_INVALID || Instruction->Type >= M68K_IT__SIZEOF__)
        return 0;

    // use the default format?
    if (Format == M68K_NULL)
        Format = M68K_DEFAULT_FORMAT;

    // init
    DISASM_TEXT_CONTEXT DTCtx;
    
    DTCtx.Address = Address;
    DTCtx.DisTextFunc = DisTextFunc;
    DTCtx.DisTextFuncData = DisTextFuncData;
    DTCtx.Instruction = Instruction;
    DTCtx.Output.Buffer = OutBuffer;
    DTCtx.Output.AvailSize = (OutBuffer != M68K_NULL ? OutSize : 0);
    DTCtx.Output.TotalSize = 0;
    DTCtx.TextFlags = Flags;

    // start building the output
    for (;;)
    {
        M68K_CHAR Char;

        // get the next char
        Char = *(Format++);
        if (Char == 0)
            break;

        // found the parameter char?
        if (Char == '%')
        {
            // check the text case
            TEXT_CASE TextCase = TC_NONE;

            Char = *Format;
            if (Char == 'L' || (DTCtx.TextFlags & M68K_DTFLG_ALL_LOWERCASE) != 0)
            {
                TextCase = TC_LOWER;
                if (Char == 'L')
                Format++;
            }
            else if (Char == 'U' || (DTCtx.TextFlags & M68K_DTFLG_ALL_UPPERCASE) != 0)
            {
                TextCase = TC_UPPER;
                if (Char == 'U')
                Format++;
            }

            // get the width
            M68K_LUINT Width = 0;
            for (;;)
            {
                Char = *(Format++);
                if (Char >= '0' && Char <= '9')
                    Width = Width * 10 + ((M68K_LUINT)Char - (M68K_LUINT)'0');
                else
                    break;
            }

            // save the desired size for the text after applying the width
            M68K_LUINT Desiredize = DTCtx.Output.TotalSize + Width;

            switch (Char)
            {
            case '%':
                AddChar(&DTCtx, Char, TC_NONE);
                break;

            case '-':
                if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_SEPARATOR, TextCase))
                    return 0;
                break;

            case 'a':
                DTCtx.TextInfo.Type = M68K_DTIF_ADDRESS_L;
                DTCtx.TextInfo.Details.Long = (M68K_DWORD)(M68K_LWORD)Address;

                if (!CallFunction(&DTCtx, TextCase))
                    return 0;
                break;

            case 'm':
                if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_INSTRUCTION_MNEMONIC, TextCase))
                    return 0;
                break;

            case 'o':
                // write all operands
                {
                    M68K_BOOL IgnoreNextSeparator = M68K_TRUE;
                    PM68K_OPERAND Operand = Instruction->Operands;

                    for (M68K_UINT Index = 0; Index < M68K_MAXIMUM_NUMBER_OPERANDS; Index++, Operand++)
                    {
                        // reached the last operand? this is identified by an operand with a type M68K_OT_NONE
                        if (Operand->Type == M68K_OT_NONE)
                            break;

                        // write a separator?
                        if (!IgnoreNextSeparator)
                            if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_OPERAND_SEPARATOR, TextCase))
                                return 0;

                        // add the operand
                        if (!AddOperand(&DTCtx, Operand, TextCase, &IgnoreNextSeparator))
                            return 0;
                    }
                }
                break;

            case 's':
                if (Instruction->Type >= M68K_IT_INVALID && Instruction->Type < M68K_IT__SIZEOF__)
                {
                    // size can be omitted?
                    if ((_M68KInstrMasterTypeFlags[Instruction->Type] & IMF_IMPLICIT_SIZE) == 0 || (Flags & M68K_DTFLG_HIDE_IMPLICIT_SIZES) == 0)
                    {
                        DTCtx.TextInfo.Type = M68K_DTIF_SIZE;
                        DTCtx.TextInfo.Details.Size = DTCtx.Instruction->Size;

                        if (!CallFunction(&DTCtx, TextCase))
                            return 0;
                    }
                }
                break;

            case 'w':
                // write the words for which we have a value
                {
                    M68K_UINT Count = 0;
                    PM68K_WORD Next = Instruction->Start;
                    
                    for (;Next < Instruction->Stop; Count++, Next++)
                    {
                        // write a separator?
                        if (Count != 0)
                            if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_OPCODE_WORD_SEPARATOR, TextCase))
                                return 0;

                        DTCtx.TextInfo.Type = M68K_DTIF_OPCODE_WORD;
                        DTCtx.TextInfo.Details.Word = M68KReadWord(Next);

                        if (!CallFunction(&DTCtx, TextCase))
                            return 0;
                    }

                    // one or more unknown words?
                    for (;Next < Instruction->End; Count++, Next++)
                    {
                        // write a separator?
                        if (Count != 0)
                            if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_OPCODE_WORD_SEPARATOR, TextCase))
                                return 0;

                        if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_OPCODE_WORD_UNKNOWN, TextCase))
                            return 0;
                    }
                }
                break;

            default:
                // unknown specifier; try to handle it with the callback function
                DTCtx.TextInfo.Type = M68K_DTIF_UNKNOWN;
                DTCtx.TextInfo.Details.Char = Char;

                if (!CallFunction(&DTCtx, TextCase))
                    return 0;

                break;
            }

            // have we reached the minimum width ?
            while (DTCtx.Output.TotalSize < Desiredize)
                if (!CallFunctionNoDetails(&DTCtx, M68K_DTIF_TEXT_PADDING, TextCase))
                    return 0;
        }    
        else
            AddChar(&DTCtx, Char, TC_NONE);
    }

    AddChar(&DTCtx, 0, TC_NONE);
    return DTCtx.Output.TotalSize;
}

// convert a disassembled instruction to a text that can be used by the M68KAssembleText function
M68K_LUINT M68KDisassembleTextForAssembler(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/)
{
    return DisassembleFor(Address, Instruction, Flags, OutBuffer, OutSize, AddOperandAssembler);
}

// convert a disassembled instruction to a text that can be used in the XL language
M68K_LUINT M68KDisassembleTextForXL(PM68K_INSTRUCTION Instruction, M68K_DISASM_TEXT_FLAGS_VALUE Flags, PM68K_CHAR OutBuffer, M68K_LUINT OutSize /*maximum in chars*/)
{
    return DisassembleFor(M68K_NULL, Instruction, Flags | M68K_DTFLG_IGNORE_SIGN, OutBuffer, OutSize, AddOperandXL);
}
