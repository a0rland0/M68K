#include "m68kinternal.h"

typedef enum AMODE_FLAGS
{
    AMF_NONE                = 0x0000,
    AMF_SWAP_MODE_REGISTER  = 0x0001,
    AMF_IGNORE_MODE         = 0x0002,
    AMF_IGNORE_REGISTER     = 0x0004,
    AMF_BASE_BANK_S8E8EW1   = 0x0008,
    AMF_VECTOR_MODE_S8E8W0  = 0x0010,
    AMF_BANK_EW_A           = 0x0020,
    AMF_BANK_EW_B           = 0x0040,
    AMF_BANK_EW_C           = 0x0080,
} AMODE_FLAGS, *PAMODE_FLAGS;

// forward declarations
static M68K_BOOL        AssembleAModeOperand(PASM_CTX ACtx, ADDRESSING_MODE_TYPE_VALUE AMode, PM68K_WORD Word, M68K_UINT StartBit, AMODE_FLAGS Flags);
static AMODE_FLAGS      CheckCanUseBankExtensionWord(PASM_CTX ACtx, PM68K_OPERAND Operand);
static M68K_BOOL        CheckDisplacementB(M68K_SDWORD Displacement);
static M68K_BOOL        CheckDisplacementW(M68K_SDWORD Displacement);
static M68K_BOOL        CheckInstructionSize(PASM_CTX ACtx, M68K_SIZE Size);
static M68K_BOOL        CheckVRegister(M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD Value, PM68K_WORD XBit);
static M68K_BOOL        ExecuteAction(PASM_CTX ACtx, DISASM_ACTION_TYPE ActionType, PM68K_BOOL Stop);
static M68K_BOOL        GetBitsForExtendedFPURegister(PM68K_OPERAND Operand, PM68K_WORD Bits);
static PM68K_OPERAND    GetNextOperand(PASM_CTX ACtx, M68K_OPERAND_TYPE_VALUE OperandType /*M68K_OT__SIZEOF__ = any*/);
static M68K_BOOL        IsValidBaseRegister07(AMODE_FLAGS Flags, M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD RegisterIndex, PM68K_BOOL UsingBank1);
static M68K_BOOL        IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register);
static M68K_BOOL        SavePCOffset(PASM_CTX ACtx, M68K_BYTE PCOffset);
static M68K_BOOL        SaveWord(PASM_CTX ACtx, M68K_WORD Word);
static M68K_BOOL        UpdateBaseBank(PASM_CTX ACtx, AMODE_FLAGS Flags, M68K_BOOL UsingBank1);
static M68K_BOOL        UpdateBaseRegister(PM68K_WORD Word, M68K_REGISTER_TYPE Register, M68K_UINT StartBit, M68K_UINT BankBit);
static M68K_BOOL        UpdateEWordBaseRegister(PASM_CTX ACtx, AMODE_FLAGS Flags, PM68K_WORD EWord, PM68K_WORD Register, PM68K_OPERAND Operand);
static M68K_BOOL        UpdateEWordBaseDisplacement(PM68K_WORD EWord, M68K_SIZE_VALUE BaseDispSize, M68K_SDWORD BaseDispValue, PM68K_SIZE_VALUE DispSize);
static M68K_BOOL        UpdateEWordIndexRegister(PM68K_WORD EWord, PM68K_OPERAND Operand, M68K_BOOL AllowSuppressed);
static M68K_BOOL        UpdateEWordOuterDisplacement(PM68K_WORD EWord, PM68K_OPERAND Operand, PM68K_SIZE_VALUE DispSize);
static M68K_BOOL        UpdateRegister(PM68K_WORD Word, M68K_REGISTER_TYPE BaseRegister, M68K_REGISTER_TYPE Register, M68K_WORD StartBit);
static M68K_BOOL        UpdateVModeBitA(PASM_CTX ACtx, AMODE_FLAGS Flags, M68K_BOOL BitAMustBeSet);

// assemble an addressing mode operand
static M68K_BOOL AssembleAModeOperand(PASM_CTX ACtx, ADDRESSING_MODE_TYPE_VALUE AMode, PM68K_WORD Word, M68K_UINT StartBit, AMODE_FLAGS Flags)
{
    M68K_BOOL Result = M68K_FALSE;

    // get the next operand
    PM68K_OPERAND Operand = GetNextOperand(ACtx, M68K_OT__SIZEOF__);
    if (Operand != M68K_NULL)
    {
        M68K_BOOL VEA = ((Flags & AMF_VECTOR_MODE_S8E8W0) != 0);
        M68K_BOOL UsingBank1 = M68K_FALSE;
        M68K_BOOL CanConvertTo = M68K_TRUE;

        M68K_WORD Bits;
        M68K_WORD Mode = 0;
        M68K_WORD EWord = 0;
        M68K_WORD Register = 0;

        M68K_SDWORD BaseDispValue;
        M68K_SDWORD OuterDispValue;
        M68K_SIZE_VALUE BaseDispSize = M68K_SIZE_NONE;
        M68K_SIZE_VALUE OuterDispSize = M68K_SIZE_NONE;

        M68K_OPERAND AltOperand;

        Flags |= CheckCanUseBankExtensionWord(ACtx, Operand);

        // use the operand type to check the addressing modes
        switch (Operand->Type)
        {
        case M68K_OT_IMMEDIATE_B:
            BaseDispSize = M68K_SIZE_B;

immediate:
            if ((AMode & AMT_IMMEDIATE_DATA) != 0 ||
                ((AMode & AMT_IMMEDIATE_DATA_P20H) != 0 && (ACtx->Architectures & M68K_ARCH__P20H__) != 0))
            {
                // make an implicit conversion to the required size?
                if (CanConvertTo && (
                    (BaseDispSize == M68K_SIZE_B || BaseDispSize == M68K_SIZE_W || BaseDispSize == M68K_SIZE_L) ||
                    // VEA will also allow Q
                    (VEA && BaseDispSize == M68K_SIZE_Q)
                ))
                {
                    M68K_SIZE_VALUE InstrSize = ACtx->Instruction->Size;

                    if (VEA)
                    {
                        // in VEA mode we must convert to W or Q
                        // 1) B => convert to W
                        if (BaseDispSize == M68K_SIZE_B)
                            InstrSize = M68K_SIZE_W;

                        // 2) L => convert to Q
                        else if (BaseDispSize == M68K_SIZE_L)
                            InstrSize = M68K_SIZE_Q;

                        // 3) W and Q => no need to convert
                        else
                            InstrSize = BaseDispSize;
                    }

                    if (InstrSize != BaseDispSize)
                    {
                        M68K_OPERAND_TYPE operandType;
                        
                        if (InstrSize == M68K_SIZE_B)
                            operandType = M68K_OT_IMMEDIATE_B;
                        else if (InstrSize == M68K_SIZE_W)
                            operandType = M68K_OT_IMMEDIATE_W;
                        else if (InstrSize == M68K_SIZE_L)
                            operandType = M68K_OT_IMMEDIATE_L;
                        else if (InstrSize == M68K_SIZE_Q)
                            operandType = M68K_OT_IMMEDIATE_Q;
                        else
                            operandType = M68K_OT_NONE;

                        if (operandType != M68K_OT_NONE)
                        {
                            if (M68KConvertOperandTo(ACtx->Flags, Operand, operandType, ACtx->NextConvOperand))
                            {
                                Operand = (ACtx->NextConvOperand)++;
                                BaseDispSize = InstrSize;
                            }
                        }
                    }
                }

                if (CheckInstructionSize(ACtx, BaseDispSize) || (VEA && (BaseDispSize == M68K_SIZE_W || BaseDispSize == M68K_SIZE_Q)))
                {
                    M68K_DWORD Values[2];
                    PM68K_DWORD NextDWord;
                    M68K_UINT NumberDWords;

                    // save the immediate data
                    switch (Operand->Type)
                    {
                    case M68K_OT_IMMEDIATE_B:
                        Result = SaveWord(ACtx, (M68K_WORD)Operand->Info.Byte);
                        break;

                    case M68K_OT_IMMEDIATE_D:
#ifdef M68K_TARGET_IS_BIG_ENDIAN
                        Values[0] = ((PM68K_DWORD)&(Operand->Info.Double))[0];
                        Values[1] = ((PM68K_DWORD)&(Operand->Info.Double))[1];
#else
                        Values[1] = ((PM68K_DWORD)&(Operand->Info.Double))[0];
                        Values[0] = ((PM68K_DWORD)&(Operand->Info.Double))[1];
#endif

                        NumberDWords = 2;
                        NextDWord = Values;
                        goto write_dwords;

                    case M68K_OT_IMMEDIATE_L:
                    case M68K_OT__XL_LANG_ELEMENT__:
                        if (!SaveWord(ACtx, (M68K_WORD)(Operand->Info.Long >> 16)))
                            break;

                        Result = SaveWord(ACtx, (M68K_WORD)Operand->Info.Long);
                        break;

                    case M68K_OT_IMMEDIATE_P:
                        NumberDWords = 3;
                        NextDWord = Operand->Info.PackedDecimal;
                        goto write_dwords;

                    case M68K_OT_IMMEDIATE_Q:
                        NumberDWords = 2;
                        NextDWord = Operand->Info.Quad;
                        goto write_dwords;

                    case M68K_OT_IMMEDIATE_S:
                        // single is stored as a 32-bit value
                        Values[0] = Operand->Info.Long;

                        NumberDWords = 1;
                        NextDWord = Values;
                        goto write_dwords;

                    case M68K_OT_IMMEDIATE_W:
                        Result = SaveWord(ACtx, Operand->Info.Word);
                        break;

                    case M68K_OT_IMMEDIATE_X:
                        NumberDWords = 3;
                        NextDWord = Operand->Info.Extended;

write_dwords:
                        for (;NumberDWords != 0; NumberDWords--, NextDWord++)
                        {
                            if (!SaveWord(ACtx, (M68K_WORD)(*NextDWord >> 16)))
                                break;

                            if (!SaveWord(ACtx, (M68K_WORD)*NextDWord))
                                break;
                        }

                        Result = (NumberDWords == 0);
                        break;

                    default:
                        break;
                    }
                
                    if (Result)
                    {
                        Mode = 7;
                        Register = 4;

                        if (VEA && BaseDispSize == M68K_SIZE_W)
                        {
                            // bit A must be set
                            if (!UpdateVModeBitA(ACtx, Flags, M68K_TRUE))
                                break;
                        }

save_mode_register:
                        {
                            M68K_BOOL IgnoreMode = ((Flags & AMF_IGNORE_MODE) != 0);
                            M68K_BOOL IgnoreRegister = ((Flags & AMF_IGNORE_REGISTER) != 0);

                            // swap mode and register?
                            if ((Flags & AMF_SWAP_MODE_REGISTER) != 0)
                            {
                                // mode and register can't be ignored
                                if (IgnoreMode || IgnoreRegister)
                                    break;

                                M68K_WORD Temp = Mode;
                                Mode = Register;
                                Register = Temp;
                            }

                            if (!IgnoreMode && IgnoreRegister)
                            {
                                // mode + ignore register
                                StartBit += 3;
                                *Word &= ~(0b111 << StartBit);
                                *Word |= ((Mode & 7) << StartBit);
                            }
                            else if (IgnoreMode && !IgnoreRegister)
                            {
                                // ignore mode + register
                                *Word &= ~(0b111 << StartBit);
                                *Word |= ((Register & 7) << StartBit);
                            }
                            else if (!IgnoreMode && !IgnoreRegister)
                            {
                                // mode + register
                                *Word &= ~(0b111111 << StartBit);
                                *Word |= ((((Mode & 7) << 3) | (Register & 7)) << StartBit);
                            }

                            Result = M68K_TRUE;
                        }
                        break;
                    }
                }
            }
            break;

        case M68K_OT_IMMEDIATE_D:
            BaseDispSize = M68K_SIZE_D;
            goto immediate;

        case M68K_OT_IMMEDIATE_L:
            BaseDispSize = M68K_SIZE_L;
            goto immediate;

        case M68K_OT_IMMEDIATE_P:
            BaseDispSize = M68K_SIZE_P;
            goto immediate;

        case M68K_OT_IMMEDIATE_Q:
            BaseDispSize = M68K_SIZE_Q;
            goto immediate;

        case M68K_OT_IMMEDIATE_S:
            BaseDispSize = M68K_SIZE_S;
            goto immediate;

        case M68K_OT_IMMEDIATE_W:
            BaseDispSize = M68K_SIZE_W;
            goto immediate;

        case M68K_OT_IMMEDIATE_X:
            BaseDispSize = M68K_SIZE_X;
            goto immediate;

        case M68K_OT_MEM_ABSOLUTE_W:
            // (xxx).w
            if ((AMode & AMT_ABSOLUTE_SHORT) != 0)
            {
                // address fits in a word?
                if (!CheckDisplacementW((M68K_SDWORD)Operand->Info.Long))
                    break;

                if (!SaveWord(ACtx, (M68K_WORD)Operand->Info.Long))
                    break;

                Mode = 7;
                Register = 0;
                goto save_mode_register;
            }
            break;

        case M68K_OT_MEM_ABSOLUTE_L:
            // (xxx).l
            if ((AMode & AMT_ABSOLUTE_LONG) != 0)
            {
                if (!SaveWord(ACtx, (M68K_WORD)(Operand->Info.Long >> 16)))
                    break;

                if (!SaveWord(ACtx, (M68K_WORD)Operand->Info.Long))
                    break;

                Mode = 7;
                Register = 1;
                goto save_mode_register;
            }
            break;

        case M68K_OT_MEM_INDIRECT:
            // (An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, &Register, &UsingBank1) &&
                    Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                    Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_NONE &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                        break;

                    Mode = 2;
                    goto save_mode_register;
                }
            }

            // we can also try it as M68K_OT_MEM_INDIRECT_DISP_W/M68K_OT_MEM_INDIRECT_INDEX_DISP_L with zero displacement
            if ((AMode & (AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT | AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L)) != 0)
            {
                AltOperand.Type = M68K_OT_MEM_INDIRECT_DISP_W;
                AltOperand.Info.Memory.Increment = M68K_INC_NONE;
                AltOperand.Info.Memory.Base = Operand->Info.Memory.Base;
                AltOperand.Info.Memory.Index.Register = M68K_RT_NONE;
                AltOperand.Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                AltOperand.Info.Memory.Displacement.BaseValue = 0;
                AltOperand.Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                Operand = &AltOperand;
                goto check_mem_indirect_disp_w;
            }
            break;

        case M68K_OT_MEM_INDIRECT_DISP_W:
check_mem_indirect_disp_w:
            // (d16, An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, &Register, &UsingBank1) &&
                    Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                    Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_W &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                        break;

                    Mode = 5;
                    BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

mem_indirect_disp_w:
                    if (!CheckDisplacementW(BaseDispValue))
                        break;
                    
                    // save the displacement
                    if (!SaveWord(ACtx, (M68K_WORD)BaseDispValue))
                        break;

                    goto save_mode_register;
                }
            }

            // we can also try it as M68K_OT_MEM_INDIRECT_INDEX_DISP_L with zero displacement
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0)
            {
                // note: Operand might be & AltOperand
                AltOperand.Type = M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L;
                AltOperand.Info.Memory.Increment = M68K_INC_NONE;
                AltOperand.Info.Memory.Base = Operand->Info.Memory.Base;
                AltOperand.Info.Memory.Index.Register = M68K_RT_NONE;
                AltOperand.Info.Memory.Displacement.BaseSize = M68K_SIZE_W;
                AltOperand.Info.Memory.Displacement.BaseValue = Operand->Info.Memory.Displacement.BaseValue;
                AltOperand.Info.Memory.Displacement.OuterSize = M68K_SIZE_NONE;
                Operand = &AltOperand;
                goto check_mem_indirect_index_disp_l;
            }
            break;

        case M68K_OT_MEM_INDIRECT_INDEX_DISP_B:
            // (d8, An/Bn, Xn.SIZE * SCALE)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_B) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, &Register, &UsingBank1) &&
                    Operand->Info.Memory.Scale >= M68K_SCALE_1 &&
                    Operand->Info.Memory.Scale <= M68K_SCALE_8 &&
                    Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_B &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                        break;

                    // scale must be 1 in older architectures i.e. 68000, 68008 and 68010
                    if ((ACtx->Architectures & M68K_ARCH__P10L__) != 0 && (ACtx->Architectures & M68K_ARCH__P20H__) == 0 && Operand->Info.Memory.Scale != M68K_SCALE_1)
                        break;

                    Mode = 6;
                    BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

mem_indirect_index_disp_b:
                    if (!CheckDisplacementB(BaseDispValue))
                        break;

                    EWord = (M68K_WORD)(M68K_BYTE)BaseDispValue;
                    if (!UpdateEWordIndexRegister(&EWord, Operand, M68K_FALSE))
                        break;

                    // save the brief extension word
                    if (!SaveWord(ACtx, EWord))
                        break;

                    goto save_mode_register;
                }
            }
            break;

        case M68K_OT_MEM_INDIRECT_INDEX_DISP_L:
check_mem_indirect_index_disp_l:
            // (d32, An/Bn, Xn.SIZE * SCALE)
            // note: d32, An and Xn are optional
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20H__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    Mode = 6;
                    if (!UpdateEWordBaseRegister(ACtx, Flags, &EWord, &Register, Operand))
                        break;

                    BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                    BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

mem_indirect_index_disp_l:
                    // requires a full extension word
                    EWord |= 0x0100;

                    if (!UpdateEWordIndexRegister(&EWord, Operand, M68K_TRUE))
                        break;

                    if (!UpdateEWordBaseDisplacement(&EWord, BaseDispSize, BaseDispValue, &BaseDispSize))
                        break;

                    OuterDispSize = M68K_SIZE_NONE;
                    OuterDispValue = Operand->Info.Memory.Displacement.OuterValue;

save_full_eword_disps:
                    // save the full extension word
                    if (!SaveWord(ACtx, EWord))
                        break;
                    
                    // save the base displacement value
                    if (BaseDispSize == M68K_SIZE_W)
                    {
                        if (!SaveWord(ACtx, (M68K_WORD)BaseDispValue))
                            break;
                    } 
                    else if (BaseDispSize == M68K_SIZE_L)
                    {
                        if (!SaveWord(ACtx, (M68K_WORD)(BaseDispValue >> 16)))
                            break;

                        if (!SaveWord(ACtx, (M68K_WORD)BaseDispValue))
                            break;
                    }

                    // save the outer displacement value?
                    if (OuterDispSize == M68K_SIZE_W)
                    {
                        if (!SaveWord(ACtx, (M68K_WORD)OuterDispValue))
                            break;
                    }
                    else if (OuterDispSize == M68K_SIZE_L)
                    {
                        if (!SaveWord(ACtx, (M68K_WORD)(OuterDispValue >> 16)))
                            break;

                        if (!SaveWord(ACtx, (M68K_WORD)OuterDispValue))
                            break;
                    }

                    goto save_mode_register;
                }
            }
            break;

        case M68K_OT_MEM_INDIRECT_POST_INCREMENT:
            // (An/Bn)+
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_POST_INCREMENT &&
                    IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, &Register, &UsingBank1) &&
                    Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                    Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_NONE &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                        break;

                    Mode = 3;
                    goto save_mode_register;
                }
            }
            break;

        case M68K_OT_MEM_INDIRECT_POST_INDEXED:
            // ([d32i, An/Bn], Xn.SIZE * SCALE, d32o)
            // notes: 
            // - post-indexed is not available in CPU32
            // - d32i, An, Xn and d32o are optional
            if ((AMode & AMT_MEMORY_INDIRECT_POST_INDEXED) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20304060__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE)
                {
                    Mode = 6;
                    if (!UpdateEWordBaseRegister(ACtx, Flags, &EWord, &Register, Operand))
                        break;

                    BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                    BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

mem_indirect_post_indexed:
                    // requires a full extension word
                    EWord |= 0x0100;

                    // index is suppressed?
                    if (Operand->Info.Memory.Index.Register != M68K_RT_NONE)
                        // IS is forced to 1xx
                        EWord |= 0x0004;

                    if (!UpdateEWordIndexRegister(&EWord, Operand, M68K_TRUE))
                        break;

                    if (!UpdateEWordBaseDisplacement(&EWord, BaseDispSize, BaseDispValue, &BaseDispSize))
                        break;
                
                    if (!UpdateEWordOuterDisplacement(&EWord, Operand, &OuterDispSize))
                        break;
                
                    OuterDispValue = Operand->Info.Memory.Displacement.OuterValue;
                    goto save_full_eword_disps;
                }
            }
            break;

        case M68K_OT_MEM_INDIRECT_PRE_DECREMENT:
            // -(An/Bn)
            if ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_PRE_DECREMENT &&
                    IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, &Register, &UsingBank1) &&
                    Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                    Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_NONE &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                        break;

                    // uses the "pre-decrement" addressing mode
                    ACtx->UsesPredecrement = M68K_TRUE;

                    Mode = 4;
                    goto save_mode_register;
                }
            }
            break;

        case M68K_OT_MEM_INDIRECT_PRE_INDEXED:
            // ([d32i, An/Bn, Xn.SIZE * SCALE], d32o)
            // notes: 
            // - pre-indexed is not available in CPU32
            // - d32i, An, Xn and d32o are optional
            if ((AMode & AMT_MEMORY_INDIRECT_PRE_INDEXED) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20304060__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE)
                {
                    Mode = 6;
                    if (!UpdateEWordBaseRegister(ACtx, Flags, &EWord, &Register, Operand))
                        break;

                    BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                    BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

mem_indirect_pre_indexed:
                    // requires a full extension and IS = 0xx
                    EWord |= 0x0100;

                    if (!UpdateEWordIndexRegister(&EWord, Operand, M68K_TRUE))
                        break;

                    if (!UpdateEWordBaseDisplacement(&EWord, BaseDispSize, BaseDispValue, &BaseDispSize))
                        break;
                
                    if (!UpdateEWordOuterDisplacement(&EWord, Operand, &OuterDispSize))
                        break;
                
                    OuterDispValue = Operand->Info.Memory.Displacement.OuterValue;
                    goto save_full_eword_disps;
                }
            }
            break;

        case M68K_OT_MEM_PC_INDIRECT_DISP_W:
            // (d16, PC)
            if ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT) != 0 ||
                ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_DISPLACEMENT_P20H) != 0 && (ACtx->Architectures & M68K_ARCH__P20H__) != 0))
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    (Operand->Info.Memory.Base == M68K_RT_PC || Operand->Info.Memory.Base == M68K_RT__RPC) &&
                    Operand->Info.Memory.Index.Register == M68K_RT_NONE &&
                    (Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_W || (Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_L && Operand->Info.Memory.Base == M68K_RT__RPC)) &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!SavePCOffset(ACtx, (M68K_BYTE)(ACtx->Opcodes.NumberWords << 1)))
                        break;

                    // the base displacement value is an address?
                    if (Operand->Info.Memory.Base == M68K_RT__RPC)
                    {
                        // yes! calculate the effective displacement value
                        BaseDispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));

                        // if the displacement doesn't fit a word value we can try to assemble as M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L (base = pc, index = -)
                        if (!CheckDisplacementW(BaseDispValue))
                        {
                            // can we really convert?
                            if (!(
                                (AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0 &&
                                (ACtx->Architectures & M68K_ARCH__P20H__) != 0
                            ))
                                // no
                                break;

                            BaseDispSize = M68K_SIZE_L;

                            Mode = 7;
                            Register = 3;
                            goto mem_indirect_index_disp_l;
                        }
                    }
                    else
                        BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

                    Mode = 7;
                    Register = 2;
                    goto mem_indirect_disp_w;
                }
            }
            break;

        case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_B:
            // (d8, PC, Xn.SIZE * SCALE)
            if ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_B) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    (Operand->Info.Memory.Base == M68K_RT_PC || Operand->Info.Memory.Base == M68K_RT__RPC) &&
                    Operand->Info.Memory.Scale >= M68K_SCALE_1 &&
                    Operand->Info.Memory.Scale <= M68K_SCALE_8 &&
                    (Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_B || (Operand->Info.Memory.Displacement.BaseSize == M68K_SIZE_L && Operand->Info.Memory.Base == M68K_RT__RPC)) &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    // scale must be 1 in older architectures i.e. 68000, 68008 and 68010
                    if ((ACtx->Architectures & M68K_ARCH__P10L__) != 0 && (ACtx->Architectures & M68K_ARCH__P20H__) == 0 && Operand->Info.Memory.Scale != M68K_SCALE_1)
                        break;

                    if (!SavePCOffset(ACtx, (M68K_BYTE)(ACtx->Opcodes.NumberWords << 1)))
                        break;

                    // the base displacement value is an address?
                    if (Operand->Info.Memory.Base == M68K_RT__RPC)
                    {
                        // yes! calculate the effective displacement value
                        BaseDispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
                    
                        // if the displacement doesn't fit a byte value we can try to assemble as M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L
                        if (!CheckDisplacementB(BaseDispValue))
                        {
                            // can we really convert?
                            if (!(
                                (AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0 &&
                                (ACtx->Architectures & M68K_ARCH__P20H__) != 0
                            ))
                                // no
                                break;

                            BaseDispSize = (CheckDisplacementW(BaseDispValue) ? M68K_SIZE_W : M68K_SIZE_L);

                            Mode = 7;
                            Register = 3;
                            goto mem_indirect_index_disp_l;
                        }
                    }
                    else
                        BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;

                    Mode = 7;
                    Register = 3;
                    goto mem_indirect_index_disp_b;
                }
            }
            break;

        case M68K_OT_MEM_PC_INDIRECT_INDEX_DISP_L:
            // (d32, PC, Xn.SIZE * SCALE)
            // note: d32, PC (as ZPC) and Xn are optional
            if ((AMode & AMT_PROGRAM_COUNTER_INDIRECT_INDEX_DISPLACEMENT_L) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20H__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    (Operand->Info.Memory.Base == M68K_RT_PC || Operand->Info.Memory.Base == M68K_RT_ZPC || Operand->Info.Memory.Base == M68K_RT__RPC) &&
                    Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE)
                {
                    if (!SavePCOffset(ACtx, (M68K_BYTE)(ACtx->Opcodes.NumberWords << 1)))
                        break;

                    // PC is suppressed?
                    if (Operand->Info.Memory.Base == M68K_RT_ZPC)
                        EWord |= 0x0080;

                    // the base displacement value is an address?
                    if (Operand->Info.Memory.Base == M68K_RT__RPC)
                    {
                        // yes! calculate the effective displacement value
                        BaseDispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
                    
                        // auto-adjust the displacement size
                        if (BaseDispValue == 0)
                            BaseDispSize = M68K_SIZE_NONE;
                        else
                            BaseDispSize = (CheckDisplacementW(BaseDispValue) ? M68K_SIZE_W : M68K_SIZE_L);
                    }
                    else
                    {
                        BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                        BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;
                    }

                    Mode = 7;
                    Register = 3;
                    goto mem_indirect_index_disp_l;
                }
            }
            break;

        case M68K_OT_MEM_PC_INDIRECT_POST_INDEXED:
            // ([d32i, PC], Xn.SIZE * SCALE, d32o)
            // notes: 
            // - post-indexed is not available in CPU32
            // - d32i, PC (as ZPC), Xn and d32o are optional
            if ((AMode & AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_POST_INDEXED) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20304060__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    (Operand->Info.Memory.Base == M68K_RT_PC || Operand->Info.Memory.Base == M68K_RT_ZPC || Operand->Info.Memory.Base == M68K_RT__RPC))
                {
                    if (!SavePCOffset(ACtx, (M68K_BYTE)(ACtx->Opcodes.NumberWords << 1)))
                        break;

                    // PC is suppressed?
                    if (Operand->Info.Memory.Base == M68K_RT_ZPC)
                        EWord |= 0x0080;

                    // the base displacement value is an address?
                    if (Operand->Info.Memory.Base == M68K_RT__RPC)
                    {
                        // yes! calculate the effective displacement value
                        BaseDispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));

                        // auto-adjust the displacement size
                        if (BaseDispValue == 0)
                            BaseDispSize = M68K_SIZE_NONE;
                        else
                            BaseDispSize = (CheckDisplacementW(BaseDispValue) ? M68K_SIZE_W : M68K_SIZE_L);
                    }
                    else
                    {
                        BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                        BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;
                    }

                    Mode = 7;
                    Register = 3;
                    goto mem_indirect_post_indexed;
                }
            }
            break;

        case M68K_OT_MEM_PC_INDIRECT_PRE_INDEXED:
            // ([d32i, PC, Xn.SIZE * SCALE], d32o)
            // notes: 
            // - pre-indexed is not available in CPU32
            // - d32i, PC (as ZPC), Xn and d32o are optional
            if ((AMode & AMT_PROGRAM_COUNTER_MEMORY_INDIRECT_PRE_INDEXED) != 0 &&
                (ACtx->Architectures & M68K_ARCH__P20304060__) != 0)
            {
                if (Operand->Info.Memory.Increment == M68K_INC_NONE &&
                    (Operand->Info.Memory.Base == M68K_RT_PC || Operand->Info.Memory.Base == M68K_RT_ZPC || Operand->Info.Memory.Base == M68K_RT__RPC))
                {
                    if (!SavePCOffset(ACtx, (M68K_BYTE)(ACtx->Opcodes.NumberWords << 1)))
                        break;

                    // PC is suppressed?
                    if (Operand->Info.Memory.Base == M68K_RT_ZPC)
                        EWord |= 0x0080;

                    // the base displacement value is an address?
                    if (Operand->Info.Memory.Base == M68K_RT__RPC)
                    {
                        // yes! calculate the effective displacement value
                        BaseDispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Memory.Displacement.BaseValue - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
                    
                        // auto-adjust the displacement size
                        if (BaseDispValue == 0)
                            BaseDispSize = M68K_SIZE_NONE;
                        else
                            BaseDispSize = (CheckDisplacementW(BaseDispValue) ? M68K_SIZE_W : M68K_SIZE_L);
                    }
                    else
                    {
                        BaseDispSize = Operand->Info.Memory.Displacement.BaseSize;
                        BaseDispValue = Operand->Info.Memory.Displacement.BaseValue;
                    }

                    Mode = 7;
                    Register = 3;
                    goto mem_indirect_pre_indexed;
                }
            }
            break;

        case M68K_OT_REGISTER:
            // Dn/En (when size is NONE, B, W, L or S because all other sizes are invalid);
            // in VEA mode we also allow size Q
            if ((AMode & AMT_DATA_REGISTER_DIRECT) != 0)
            {
                BaseDispSize = ACtx->Instruction->Size;
                if (BaseDispSize == M68K_SIZE_NONE || BaseDispSize == M68K_SIZE_B || BaseDispSize == M68K_SIZE_W || BaseDispSize == M68K_SIZE_L || BaseDispSize == M68K_SIZE_S || (VEA && BaseDispSize == M68K_SIZE_Q))
                {
                    if (IsValidRegister07(M68K_RT_D0, Operand->Info.Register))
                    {
                        Mode = 0;
                        Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_D0);
                        goto save_mode_register;
                    }
                    else if (VEA)
                    {
                        if (IsValidRegister07(M68K_RT_E0, Operand->Info.Register))
                        {
                            Mode = 1;
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E0);
                            goto save_mode_register;
                        }
                        else if (IsValidRegister07(M68K_RT_E8, Operand->Info.Register))
                        {
                            // bit A must be set
                            if (!UpdateVModeBitA(ACtx, Flags, M68K_TRUE))
                                break;
                        
                            Mode = 0;
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E8);
                            goto save_mode_register;
                        }
                        else if (IsValidRegister07(M68K_RT_E16, Operand->Info.Register))
                        {
                            // bit A must be set
                            if (!UpdateVModeBitA(ACtx, Flags, M68K_TRUE))
                                break;

                            Mode = 1;
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E16);
                            goto save_mode_register;
                        }
                    }
                    // we need the bank extension word
                    else if ((Flags & (AMF_BANK_EW_A | AMF_BANK_EW_B)) != 0)
                    {
                        if (IsValidRegister07(M68K_RT_E0, Operand->Info.Register))
                        {
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E0);
                            Bits = 1;

                        dreg_bankew:
                            ACtx->AFlags |= ACF_BANK_EXTENSION_WORD;

                            if ((Flags & AMF_BANK_EW_A) != 0)
                                ACtx->BankExtensionInfo.A = Bits;
                            else
                                ACtx->BankExtensionInfo.B = Bits;

                            Mode = 0;
                            goto save_mode_register;
                        }
                        else if (IsValidRegister07(M68K_RT_E8, Operand->Info.Register))
                        {
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E8);
                            Bits = 2;
                            goto dreg_bankew;
                        }
                        else if (IsValidRegister07(M68K_RT_E16, Operand->Info.Register))
                        {
                            Register = (M68K_WORD)(Operand->Info.Register - M68K_RT_E16);
                            Bits = 3;
                            goto dreg_bankew;
                        }
                    }
                }
            }
            
            // An/Bn?
            if (!VEA && (
                (AMode & AMT_ADDRESS_REGISTER_DIRECT) != 0 ||
                ((AMode & AMT_ADDRESS_REGISTER_INDIRECT_P20H) != 0 && (ACtx->Architectures & M68K_ARCH__P20H__) != 0)
            ))
            {
                // note: 
                // address registers can't be used with byte operations so the size must be W or L;
                // the exception is apollo core 68080 which includes some instructions where An/Bn can be used as a byte;
                if (ACtx->Instruction->Size == M68K_SIZE_W || ACtx->Instruction->Size == M68K_SIZE_L || (ACtx->AFlags & ACF_AMODE1B) != 0)
                {
                    if (IsValidBaseRegister07(Flags, Operand->Info.Register, &Register, &UsingBank1))
                    {
                        if (!UpdateBaseBank(ACtx, Flags, UsingBank1))
                            break;

                        Mode = 1;
                        goto save_mode_register;
                    }
                }
            }
            break;

        case M68K_OT__XL_LANG_ELEMENT__:
            if ((ACtx->Flags & M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS) != 0)
            {
                BaseDispSize = M68K_SIZE_L;
                CanConvertTo = M68K_FALSE;
                goto immediate;
            }
            break;

        case M68K_OT_REGISTER_FP_LIST:
        case M68K_OT_REGISTER_FPCR_LIST:
        case M68K_OT_REGISTER_LIST:

        default:
            break;
        }
    }

    return Result;
}

// check if the instruction can use a bank extension word
static AMODE_FLAGS CheckCanUseBankExtensionWord(PASM_CTX ACtx, PM68K_OPERAND Operand)
{
    if ((ACtx->Flags & M68K_AFLG_ALLOW_BANK_EXTENSION_WORD) != 0 &&
        (ACtx->Architectures & M68K_ARCH_AC68080) != 0)
    {
        INSTRUCTION_FLAGS_VALUE IFlags = ACtx->IInfo->IFlags;

        if ((IFlags & IF_FPU_OPMODE) != 0)
            IFlags = _M68KInstrFPUOpmodes[_M68KInstrMasterTypeFPUOpmodes[ACtx->Instruction->Type]].IFlags;

        if (Operand != M68K_NULL)
        {
            switch (Operand - ACtx->Instruction->Operands)
            {
            case 0:
                if ((IFlags & IF_BANK_EW_A) != 0)
                    return AMF_BANK_EW_A;
                break;

            case 1:
                if ((IFlags & IF_BANK_EW_B) != 0)
                    return AMF_BANK_EW_B;
                break;

            default:
                break;
            }
        }
        else if ((IFlags & IF_BANK_EW_C) != 0)
            return AMF_BANK_EW_C;
    }

    return 0;
}

// check if a value fits a 8-bit displacement
static M68K_BOOL CheckDisplacementB(M68K_SDWORD Displacement)
{
    return (Displacement >= MIN_BYTE && Displacement <= MAX_BYTE);
}

// check if a value fits a 32-bit displacement
static M68K_BOOL CheckDisplacementW(M68K_SDWORD Displacement)
{
    return (Displacement >= MIN_WORD && Displacement <= MAX_WORD);
}

// check if the instruction size is valid
static M68K_BOOL CheckInstructionSize(PASM_CTX ACtx, M68K_SIZE Size)
{
    if (ACtx->Instruction->Size == Size)
        return M68K_TRUE;

    // instruction allows implicit size?
    if ((_M68KInstrMasterTypeFlags[ACtx->Instruction->Type] & IMF_IMPLICIT_SIZE) != 0)
    {
        if (ACtx->Instruction->Size == M68K_SIZE_NONE)
        {
            // force it
            ACtx->Instruction->Size = Size;
            return M68K_TRUE;
        }
    }

    return M68K_FALSE;
}

// get the value and extension bits for a vector register
static M68K_BOOL CheckVRegister(M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD Value, PM68K_WORD XBit)
{
    if (Register >= M68K_RT_D0 && Register <= M68K_RT_D7)
    {
        *Value = (M68K_WORD)(Register - M68K_RT_D0);
        *XBit = 0;
    }
    else if (Register >= M68K_RT_E0 && Register <= M68K_RT_E7)
    {
        *Value = 8 | (M68K_WORD)(Register - M68K_RT_E0);
        *XBit = 0;
    }
    else if (Register >= M68K_RT_E8 && Register <= M68K_RT_E15)
    {
        *Value = (M68K_WORD)(Register - M68K_RT_E8);
        *XBit = 1;
    }
    else if (Register >= M68K_RT_E16 && Register <= M68K_RT_E23)
    {
        *Value = 8 | (M68K_WORD)(Register - M68K_RT_E16);
        *XBit = 1;
    }
    else
        return M68K_FALSE;

    return M68K_TRUE;
}

// execute an action
static M68K_BOOL ExecuteAction(PASM_CTX ACtx, DISASM_ACTION_TYPE ActionType, PM68K_BOOL Stop)
{
    M68K_WORD XBit;
    M68K_WORD Value;
    M68K_WORD WordIndex;
    M68K_SDWORD DispValue;
    PM68K_OPERAND Operand;
    M68K_REGISTER_TYPE Register;
    
    M68K_BOOL Result = M68K_FALSE;

    // check a value and mask?
    if (ActionType >= DAT_C__VMSTART__ && ActionType < DAT_C__VMEND__)
    {
        ACtx->Opcodes.Words[ActionType >= DAT_C__VMEW2START__ ? 2 : 1] |= _M68KDisValueMasks[ActionType - DAT_C__VMSTART__].Value;
        Result = M68K_TRUE;
    }
    else
    {
        switch (ActionType)
        {
        case DAT_NONE:
            // note: Result must be M68K_FALSE to stop executing the action\s
            *Stop = M68K_TRUE;
            break;

        case DAT_F_AMode1B:
            ACtx->AFlags |= ACF_AMODE1B;
            Result = M68K_TRUE;
            break;

        case DAT_FPOP_S0E7EW1:
        
            // the instruction id must be a valid fpu opmode
            Value = _M68KInstrMasterTypeFPUOpmodes[ACtx->Instruction->Type];
            if (Value >= 128)
                break;

            ACtx->Opcodes.Words[1] |= Value;
            Result = M68K_TRUE;
            break;

        case DAT_O_AbsL_NW:
            Result = AssembleAModeOperand(ACtx, AMT_ABSOLUTE_LONG, ACtx->Opcodes.Words, 0, AMF_IGNORE_MODE | AMF_IGNORE_REGISTER);
            break;

        case DAT_O_AC80AMode_S0E5W0_S8E8EW1:
            Result = AssembleAModeOperand(ACtx, AMT__AC80_ALL__, ACtx->Opcodes.Words, 0, AMF_BASE_BANK_S8E8EW1);
            break;

        case DAT_O_AC80AModeA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__AC80_ALTERABLE__, ACtx->Opcodes.Words, 0, 0);
            break;

        case DAT_O_AC80AModeA_S0E5W0_S8E8EW1:
            Result = AssembleAModeOperand(ACtx, AMT__AC80_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_BASE_BANK_S8E8EW1);
            break;

        case DAT_O_AC80AReg_S12E14EW1_S7E7EW1:
            
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateBaseRegister(ACtx->Opcodes.Words + 1, Operand->Info.Register, 12, 7);
            break;

        case DAT_O_AC80BankEWSize_S6E7W0:
            
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [4,6,8,10]
            if (Operand->Info.Byte < 4 ||
                Operand->Info.Byte > 10 ||
                (Operand->Info.Byte & 1) != 0)
                break;

            ACtx->Opcodes.Words[0] &= ~0x00c0;
            ACtx->Opcodes.Words[0] |= (((M68K_WORD)(Operand->Info.Byte - 4) >> 1) << 6);
            Result = M68K_TRUE;
            break;

        case DAT_O_AC80BReg_S0E2W0:
            Value = 0;

breg_w0:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + 0, M68K_RT_B0, Operand->Info.Register, Value);
            break;

        case DAT_O_AC80BReg_S9E11W0:
            Value = 9;
            goto breg_w0;

        case DAT_O_AC80DReg_S12E14EW1_S7E7EW1:

            // the data register cannot be banked with one bit
            ACtx->Opcodes.Words[1] &= 0xff7f;
            Value = 12;

dreg_ew1:
            WordIndex = 1;

dreg:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_D0, Operand->Info.Register, Value);
            if (!Result)
            {
dreg_fpreg_bank_ew:
                {
                    AMODE_FLAGS Flags = CheckCanUseBankExtensionWord(ACtx, Operand);
                    if (Flags != 0)
                    {
                        M68K_WORD Bits;

                        if (UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_E0, Operand->Info.Register, Value))
                            Bits = 1;
                        else if (UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_E8, Operand->Info.Register, Value))
                            Bits = 2;
                        else if (UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_E16, Operand->Info.Register, Value))
                            Bits = 3;
                        else
                            break;

                        ACtx->AFlags |= ACF_BANK_EXTENSION_WORD;
                        if ((Flags & AMF_BANK_EW_A) != 0)
                            ACtx->BankExtensionInfo.A = Bits;
                        else
                            ACtx->BankExtensionInfo.B = Bits;

                        Result = M68K_TRUE;
                    }
                }
            }
            break;

        case DAT_O_AC80VAMode_S0E5W0_S8E8W0:
            Result = AssembleAModeOperand(ACtx, AMT__AC80_ALL__, ACtx->Opcodes.Words, 0, AMF_VECTOR_MODE_S8E8W0);
            break;

        case DAT_O_AC80VAModeA_S0E5W0_S8E8W0:
            Result = AssembleAModeOperand(ACtx, AMT__AC80_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_VECTOR_MODE_S8E8W0);
            break;

        case DAT_O_AC80VReg_S0E3EW1_S8E8W0:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            if (CheckVRegister(Operand->Info.Register, &Value, &XBit))
            {
                ACtx->Opcodes.Words[0] |= (XBit << 8);
                ACtx->Opcodes.Words[1] |= Value;
                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AC80VReg_S12E15EW1_S7E7W0:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            if (CheckVRegister(Operand->Info.Register, &Value, &XBit))
            {
                ACtx->Opcodes.Words[0] |= (XBit << 7);
                ACtx->Opcodes.Words[1] |= (Value << 12);
                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AC80VReg_S8E11EW1_S6E6W0:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            if (CheckVRegister(Operand->Info.Register, &Value, &XBit))
            {
                ACtx->Opcodes.Words[0] |= (XBit << 6);
                ACtx->Opcodes.Words[1] |= (Value << 8);
                Result = M68K_TRUE;
            }
            break;

        case DAT_O_AC80VRegPM2_S8E11EW1_S6E6W0:
            Operand = GetNextOperand(ACtx, M68K_OT_VREGISTER_PAIR_M2);
            if (Operand == M68K_NULL)
                break;

            if (CheckVRegister(Operand->Info.RegisterPair.Register1, &Value, &XBit))
            {
                // multiple of 2?
                if ((Value & 1) == 0)
                {
                    if (Operand->Info.RegisterPair.Register2 == (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 1))
                    {
                        ACtx->Opcodes.Words[0] |= (XBit << 6);
                        ACtx->Opcodes.Words[1] |= (Value << 8);
                        Result = M68K_TRUE;
                    }
                }
            }
            break;

        case DAT_O_AC80VRegPM4_S0E3W0_S8E8W0:
            Operand = GetNextOperand(ACtx, M68K_OT_VREGISTER_PAIR_M4);
            if (Operand == M68K_NULL)
                break;

            if (CheckVRegister(Operand->Info.RegisterPair.Register1, &Value, &XBit))
            {
                // multiple of 4?
                if ((Value & 3) == 0)
                {
                    if (Operand->Info.RegisterPair.Register2 == (M68K_REGISTER_TYPE_VALUE)(Operand->Info.RegisterPair.Register1 + 3))
                    {
                        ACtx->Opcodes.Words[0] |= ((XBit << 8) | Value);
                        Result = M68K_TRUE;
                    }
                }
            }
            break;

        case DAT_O_AMode_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__ALL__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeC_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCAPD_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCAPDPI_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL_ALTERABLE__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCPD_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCPDPI_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeCPI_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__CONTROL__ | AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeD_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__DATA__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeDA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__DATA_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeDA_S6E11W0:
            Result = AssembleAModeOperand(ACtx, AMT__DATA_ALTERABLE__, ACtx->Opcodes.Words, 6, AMF_SWAP_MODE_REGISTER);
            break;

        case DAT_O_AModeDAM_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__DATA_ALTERABLE_MEMORY__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeDAPC_S0E5W0:
            Result = AssembleAModeOperand(ACtx, ((ACtx->Architectures & M68K_ARCH__P20H__) != 0 ? AMT__DATA_ALTERABLE_PC__ : AMT__DATA_ALTERABLE__), ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeDRDCA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__DATA_REGISTER_DIRECT_CONTROL_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeIA_S0E2W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT, ACtx->Opcodes.Words, 0, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModeIAD_S0E2W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_DISPLACEMENT, ACtx->Opcodes.Words, 0, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModeMA_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__MEMORY_ALTERABLE__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModePD_S0E2W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, ACtx->Opcodes.Words, 0, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModePD_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, ACtx->Opcodes.Words, 0, 0);
            break;

        case DAT_O_AModePD_S9E11W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_PRE_DECREMENT, ACtx->Opcodes.Words, 9, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModePFR_S0E5W0:
            // size is forced to word
            Result = AssembleAModeOperand(ACtx, AMT__PFR__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModePI_S0E2W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words, 0, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModePI_S12E14W1:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words + 1, 12, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModePI_S9E11W0:
            Result = AssembleAModeOperand(ACtx, AMT_ADDRESS_REGISTER_INDIRECT_POST_INCREMENT, ACtx->Opcodes.Words, 9, AMF_IGNORE_MODE);
            break;

        case DAT_O_AModeTOUCH_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__TOUCH__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AModeTST_S0E5W0:
            Result = AssembleAModeOperand(ACtx, AMT__TST__, ACtx->Opcodes.Words, 0, AMF_NONE);
            break;

        case DAT_O_AReg_S0E2W0:
            Value = 0;

areg_w0:
            WordIndex = 0;

areg:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_A0, Operand->Info.Register, Value);
            if (!Result)
            {
                AMODE_FLAGS Flags = CheckCanUseBankExtensionWord(ACtx, Operand);
                if (Flags != 0)
                {
                    Result = UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_B0, Operand->Info.Register, Value);
                    if (Result)
                        Result = UpdateBaseBank(ACtx, Flags, M68K_TRUE);
                }
            }
            break;

        case DAT_O_AReg_S0E2EW1:
            Value = 0;

areg_ew1:
            WordIndex = 1;
            goto areg;

        case DAT_O_AReg_S12E14EW1:
            Value = 12;
            goto areg_ew1;

        case DAT_O_AReg_S5E7EW1:
            Value = 5;
            goto areg_ew1;

        case DAT_O_AReg_S9E11W0:
            Value = 9;
            goto areg_w0;

        case DAT_O_BACReg_S2E4EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_BAC0, Operand->Info.Register, 2);
            break;

        case DAT_O_BADReg_S2E4EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_BAD0, Operand->Info.Register, 2);
            break;

        case DAT_O_CC_S8E11W0:
            Operand = GetNextOperand(ACtx, M68K_OT_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;

condition_code:
            if (Operand->Info.ConditionCode < 0 || Operand->Info.ConditionCode >= 16)
                break;
        
            ACtx->Opcodes.Words[0] &= ~0x0f00;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)(Operand->Info.ConditionCode << 8);
            Result = M68K_TRUE;
            break;

        case DAT_O_CC2H_S8E11W0:
            Operand = GetNextOperand(ACtx, M68K_OT_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;
        
            if (Operand->Info.ConditionCode < 2)
                break;
        
            goto condition_code;

        case DAT_O_CPID_S9E11W0:
            Operand = GetNextOperand(ACtx, M68K_OT_COPROCESSOR_ID);
            if (Operand == M68K_NULL)
                break;
        
            if (Operand->Info.CoprocessorId < 0 || Operand->Info.CoprocessorId > 8)
                break;

cpid_s9e11w0:
            ACtx->Opcodes.Words[0] |= (M68K_WORD)(Operand->Info.CoprocessorId << 9);
            Result = M68K_TRUE;
            break;

        case DAT_O_CPIDCC_S9E11W0_S0E5W0:
            Operand = GetNextOperand(ACtx, M68K_OT_COPROCESSOR_ID_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.CoprocessorIdCC.Id < 0 || Operand->Info.CoprocessorIdCC.Id > 8 || Operand->Info.CoprocessorIdCC.CC >= 64)
                break;

            ACtx->Opcodes.Words[0] |= (M68K_WORD)Operand->Info.CoprocessorIdCC.CC;
            goto cpid_s9e11w0;

        case DAT_O_CPIDCC_S9E11W0_S0E5EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_COPROCESSOR_ID_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.CoprocessorIdCC.Id < 0 || Operand->Info.CoprocessorIdCC.Id > 8 || Operand->Info.CoprocessorIdCC.CC >= 64)
                break;

            ACtx->Opcodes.Words[1] |= (M68K_WORD)Operand->Info.CoprocessorIdCC.CC;
            goto cpid_s9e11w0;

        case DAT_O_CReg_S0E11EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            // check if the control register is valid and allowed
            for (PC_CONTROL_REGISTER NextCReg = _M68KControlRegisters; NextCReg->Value < 0xf000; NextCReg++)
            {
                // found the same register?
                if (NextCReg->Register == Operand->Info.Register)
                {
                    // the architecture allows it?
                    if ((ACtx->Architectures & NextCReg->Architectures) != 0)
                    {
                        ACtx->Opcodes.Words[1] &= ~0x0fff;
                        ACtx->Opcodes.Words[1] |= NextCReg->Value;
                        Result = M68K_TRUE;
                        break;
                    }
                }
            }
            break;

        case DAT_O_CT_S6E7W0:
            Operand = GetNextOperand(ACtx, M68K_OT_CACHE_TYPE);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.CacheType < 0 || Operand->Info.CacheType > 3)
                break;

            ACtx->Opcodes.Words[0] &= ~0x00c0;
            ACtx->Opcodes.Words[0] |= ((M68K_WORD)Operand->Info.CacheType << 6);
            Result = M68K_TRUE;
            break;

        case DAT_O_DispB_S0E7W0:
            Operand = GetNextOperand(ACtx, M68K_OT__SIZEOF__);
            if (Operand == M68K_NULL)
                break;

            if (!SavePCOffset(ACtx, _M68KDisGetImplicitPCOffset(ACtx->Instruction->Type)))
                break;

            if (Operand->Type == M68K_OT_DISPLACEMENT_B)
            {
                // value cannot be 0; for example 6000 is not "bra.b %db(0)" but the opcode for "bra.w"
                if ((DispValue = Operand->Info.Displacement) == 0)
                    break;
            }
            else if (Operand->Type == M68K_OT_ADDRESS_B)
                DispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Displacement - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
            else if ((ACtx->Flags & M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS) != 0 && Operand->Type == M68K_OT__XL_LABEL__)
                // see the note a few lines above
                DispValue = 0x7f;
            else
                break;

            if (!CheckDisplacementB(DispValue))
                break;

            ACtx->Opcodes.Words[0] &= ~0x00ff;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)(M68K_BYTE)DispValue;
            Result = M68K_TRUE;
            break;

        case DAT_O_DispL_NW:
            Operand = GetNextOperand(ACtx, M68K_OT__SIZEOF__);
            if (Operand == M68K_NULL)
                break;

            if (!SavePCOffset(ACtx, _M68KDisGetImplicitPCOffset(ACtx->Instruction->Type)))
                break;

            if (Operand->Type == M68K_OT_DISPLACEMENT_L)
                DispValue = Operand->Info.Displacement;
            else if (Operand->Type == M68K_OT_ADDRESS_L)
                DispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Displacement - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
            else if ((ACtx->Flags & M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS) != 0 && (
                Operand->Type == M68K_OT__XL_LABEL__ || 
                Operand->Type == M68K_OT__XL_LANG_ELEMENT__
            ))
                DispValue = 0x7fffffff;
            else
                break;

            if (!SaveWord(ACtx, (M68K_WORD)(DispValue >> 16)))
                break;

            Result = SaveWord(ACtx, (M68K_WORD)DispValue);
            break;

        case DAT_O_DispW_NW:
            Operand = GetNextOperand(ACtx, M68K_OT__SIZEOF__);
            if (Operand == M68K_NULL)
                break;

            if (!SavePCOffset(ACtx, _M68KDisGetImplicitPCOffset(ACtx->Instruction->Type)))
                break;

            if (Operand->Type == M68K_OT_DISPLACEMENT_W)
                DispValue = Operand->Info.Displacement;
            else if (Operand->Type == M68K_OT_ADDRESS_W)
                DispValue = (M68K_SDWORD)(M68K_DWORD)(M68K_LWORD)((M68K_SLWORD)Operand->Info.Displacement - ((M68K_SLWORD)ACtx->Address + (M68K_SLWORD)(M68K_SWORD)ACtx->Opcodes.PCOffset));
            else if ((ACtx->Flags & M68K_AFLG_ALLOW_XLANGLIB_EXTENSIONS) != 0 && Operand->Type == M68K_OT__XL_LABEL__)
                DispValue = 0x7fff;
            else
                break;

            if (!CheckDisplacementW(DispValue))
                break;

            Result = SaveWord(ACtx, (M68K_WORD)DispValue);
            break;

        case DAT_O_DKFac_S4E6EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_DYNAMIC_KFACTOR);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.Register, 4);
            break;

        case DAT_O_DReg_S0E2W0:
            Value = 0;

dreg_ew0:   
            WordIndex = 0;
            goto dreg;

        case DAT_O_DReg_S0E2EW1:
            Value = 0;
            goto dreg_ew1;

        case DAT_O_DReg_S12E14EW1:
            Value = 12;
            goto dreg_ew1;

        case DAT_O_DReg_S4E6EW1:
            Value = 4;
            goto dreg_ew1;

        case DAT_O_DReg_S6E8EW1:
            Value = 6;
            goto dreg_ew1;

        case DAT_O_DReg_S9E11W0:
            Value = 9;
            goto dreg_ew0;

        case DAT_O_DRegP_S0E2W0_S0E2EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_PAIR);
            if (Operand == M68K_NULL)
                break;

            if (!UpdateRegister(ACtx->Opcodes.Words + 0, M68K_RT_D0, Operand->Info.RegisterPair.Register1, 0))
                break;

            Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.RegisterPair.Register2, 0);
            break;

        case DAT_O_DRegP_S0E2EW1_S0E2EW2:
            Value = 0;

dregp_ew1_ew2:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_PAIR);
            if (Operand == M68K_NULL)
                break;

            if (!UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.RegisterPair.Register1, Value))
                break;

            Result = UpdateRegister(ACtx->Opcodes.Words + 2, M68K_RT_D0, Operand->Info.RegisterPair.Register2, Value);
            break;

        case DAT_O_DRegP_S0E2EW1_S12E14EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_PAIR);
            if (Operand == M68K_NULL)
                break;

            if (!UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.RegisterPair.Register1, 0))
                break;

            Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.RegisterPair.Register2, 12);
            break;

        case DAT_O_DRegP_S6E8EW1_S6E8EW2:
            Value = 6;
            goto dregp_ew1_ew2;

        case DAT_O_FC_S0E4EW1:
            Operand = GetNextOperand(ACtx, M68K_OT__SIZEOF__);
            if (Operand == M68K_NULL)
                break;
            
            // sfc or dfc?
            if (Operand->Type == M68K_OT_REGISTER)
            {
                if (Operand->Info.Register == M68K_RT_SFC)
                    // 00000
                    Result = M68K_TRUE;
                else if (Operand->Info.Register == M68K_RT_DFC)
                {
                    // 00001
                    ACtx->Opcodes.Words[1] |= 0b00001;
                    Result = M68K_TRUE;
                }
                else if (Operand->Info.Register >= M68K_RT_D0 && Operand->Info.Register <= M68K_RT_D7)
                {
                    // 01rrr
                    ACtx->Opcodes.Words[1] |= 0b01000;
                    Result = UpdateRegister(ACtx->Opcodes.Words + 1, M68K_RT_D0, Operand->Info.Register, 0);
                }
            }
            // immediate?
            else if (Operand->Type == M68K_OT_IMMEDIATE_B)
            {
FC_S0E4EW1_immB:
                if ((ACtx->Architectures & M68K_ARCH_68851) != 0)
                {
                    // valid values are 0-15
                    if (Operand->Info.Byte >= 16)
                        break;
                }
                else if ((ACtx->Architectures & (M68K_ARCH_68030 | M68K_ARCH_68EC030)) != 0)
                {
                    // valid values are 0-7
                    if (Operand->Info.Byte >= 8)
                        break;
                }
                else
                    break;

                // 1dddd or 10ddd
                ACtx->Opcodes.Words[1] |= 0b10000;
                ACtx->Opcodes.Words[1] |= (M68K_WORD)Operand->Info.Byte;
                Result = M68K_TRUE;
            }
            // implicit conversion?
            else if (M68KConvertOperandTo(ACtx->Flags, Operand, M68K_OT_IMMEDIATE_B, ACtx->NextConvOperand))
            {
                Operand = (ACtx->NextConvOperand)++;
                goto FC_S0E4EW1_immB;
            }
            break;

        case DAT_O_FPCC_S0E4W0:
        case DAT_O_FPCC_S0E4EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_FPU_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;
        
            if (Operand->Info.FpuConditionCode < 0 || Operand->Info.FpuConditionCode >= 32)
                break;

            Value = (ActionType == DAT_O_FPCC_S0E4W0 ? 0 : 1);
            ACtx->Opcodes.Words[Value] &= ~0x001f;
            ACtx->Opcodes.Words[Value] |= (M68K_WORD)Operand->Info.FpuConditionCode;
            Result = M68K_TRUE;
            break;

        case DAT_O_FPCRRegList_S10E12EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_FPCR_LIST);
            if (Operand == M68K_NULL)
                break;

            if ((Operand->Info.RegisterList & ~0b111) != 0 || Operand->Info.RegisterList == 0)
                break;

            ACtx->Opcodes.Words[1] &= ~0x1c00;

            if ((Operand->Info.RegisterList & 0b001) != 0)
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(1 << 12);

            if ((Operand->Info.RegisterList & 0b010) != 0)
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(1 << 11);

            if ((Operand->Info.RegisterList & 0b100) != 0)
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(1 << 10);

            Result = M68K_TRUE;
            break;

        case DAT_O_FPReg_S0E2EW1:
            Value = 0;

fpreg_ew1:
            WordIndex = 1;

//fpreg:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;
        
            Result = UpdateRegister(ACtx->Opcodes.Words + WordIndex, M68K_RT_FP0, Operand->Info.Register, Value);
            if (!Result)
                goto dreg_fpreg_bank_ew;
            break;

        case DAT_O_FPReg_S7E9EW1:
            Value = 7;
            goto fpreg_ew1;

        case DAT_O_FPReg_S10E12EW1:
            Value = 10;
            goto fpreg_ew1;

        case DAT_O_FPRegIList_S0E7EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_FP_LIST);
            if (Operand == M68K_NULL)
                break;
        
            Value = (M68K_WORD)(_M68KDisInvertWordBits((M68K_WORD)Operand->Info.RegisterList) >> 8);

fpreglist_ew1:
            if ((Operand->Info.RegisterList & 0xff00) != 0)
                break;

            ACtx->Opcodes.Words[1] &= ~0x00ff;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)(M68K_BYTE)Value;
            Result = M68K_TRUE;
            break;

        case DAT_O_FPRegList_S0E7EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_FP_LIST);
            if (Operand == M68K_NULL)
                break;

            Value = (M68K_WORD)(M68K_BYTE)Operand->Info.RegisterList;
            goto fpreglist_ew1;

        case DAT_O_ImmB0_I:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.Byte != 0)
                break;

            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E1W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,3]
            if (Operand->Info.Byte >= 4)
                break;

            ACtx->Opcodes.Words[0] &= ~0x0003;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)Operand->Info.Byte;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E2W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,7]
            if (Operand->Info.Byte >= 8)
                break;

            ACtx->Opcodes.Words[0] &= ~0x0007;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)Operand->Info.Byte;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E3W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,15]
            if (Operand->Info.Byte >= 16)
                break;

            ACtx->Opcodes.Words[0] &= ~0x000f;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)Operand->Info.Byte;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E6EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,127]
            if (Operand->Info.Byte >= 128)
                break;

            ACtx->Opcodes.Words[1] &= ~0x007f;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)(Operand->Info.Byte & 0x7f);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E7W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            ACtx->Opcodes.Words[0] &= ~0x00ff;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)Operand->Info.Byte;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S0E7EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            ACtx->Opcodes.Words[1] &= ~0x00ff;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)Operand->Info.Byte;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S10E12EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,7]
            if (Operand->Info.Byte >= 8)
                break;

            ACtx->Opcodes.Words[1] &= ~0x1c00;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)(Operand->Info.Byte << 10);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S2E3W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,3]
            if (Operand->Info.Byte >= 4)
                break;

            ACtx->Opcodes.Words[0] &= ~0x000c;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)(Operand->Info.Byte << 2);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S8E9EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,3]
            if (Operand->Info.Byte >= 4)
                break;

            ACtx->Opcodes.Words[1] |= (M68K_WORD)((M68K_WORD)Operand->Info.Byte << 8);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S9E11W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,7]
            if (Operand->Info.Byte >= 8)
                break;

            ACtx->Opcodes.Words[0] &= ~0x0e00;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)((M68K_WORD)Operand->Info.Byte << 9);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmB_S9E11W0_S4E5W0:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,31]
            if (Operand->Info.Byte >= 32)
                break;

            ACtx->Opcodes.Words[0] &= ~0x0e30;
            ACtx->Opcodes.Words[0] |= ((M68K_WORD)(Operand->Info.Byte & 7) << 9);
            ACtx->Opcodes.Words[0] |= ((M68K_WORD)(Operand->Info.Byte & 0x18) << 1);
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmL_NW:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_L);
            if (Operand == M68K_NULL)
                break;

            if (!SaveWord(ACtx, (M68K_WORD)(Operand->Info.Long >> 16)))
                break;

            Result = SaveWord(ACtx, (M68K_WORD)Operand->Info.Long);
            break;

        case DAT_O_ImmS:
            switch ((M68K_SIZE)ACtx->Instruction->Size)
            {
            case M68K_SIZE_B:
                Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
                if (Operand == M68K_NULL)
                    break;

                Result = SaveWord(ACtx, (M68K_WORD)Operand->Info.Byte);
                break;

            case M68K_SIZE_W:
                Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_W);
                if (Operand == M68K_NULL)
                    break;

                Result = SaveWord(ACtx, Operand->Info.Word);
                break;

            case M68K_SIZE_L:
                Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_L);
                if (Operand == M68K_NULL)
                    break;

                if (!SaveWord(ACtx, (M68K_WORD)(Operand->Info.Long >> 16)))
                    break;

                Result = SaveWord(ACtx, (M68K_WORD)Operand->Info.Long);
                break;

            default:
                break;
            }
            break;

        case DAT_O_ImmW_S0E11EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_W);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,0b111_111_111_111 = 0xfff]
            if (Operand->Info.Word > 0xfff)
                break;

            ACtx->Opcodes.Words[1] &= ~0x0fff;
            ACtx->Opcodes.Words[1] |= Operand->Info.Word;
            Result = M68K_TRUE;
            break;

        case DAT_O_ImmW_NW:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_W);
            if (Operand == M68K_NULL)
                break;

            Result = SaveWord(ACtx, Operand->Info.Word);
            break;

        case DAT_O_MMUCC_S0E3W0:
        case DAT_O_MMUCC_S0E3EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_MMU_CONDITION_CODE);
            if (Operand == M68K_NULL)
                break;
        
            if (Operand->Info.MMUConditionCode < 0 || Operand->Info.MMUConditionCode >= 16)
                break;

            Value = (ActionType == DAT_O_MMUCC_S0E3W0 ? 0 : 1);
            ACtx->Opcodes.Words[Value] &= ~0x000f;
            ACtx->Opcodes.Words[Value] |= (M68K_WORD)Operand->Info.MMUConditionCode;
            Result = M68K_TRUE;
            break;

        case DAT_O_MMUMask_S5E8EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            // valid values are [0,7] for 68030 and [0,15] for 68851
            if ((ACtx->Architectures & (M68K_ARCH_68030 | M68K_ARCH_68851)) == 0 ||
                // 68851 => 0..15
                Operand->Info.Byte > 15 ||
                // 68030 => 0..7
                ((ACtx->Architectures & (M68K_ARCH_68030 | M68K_ARCH_68851)) == M68K_ARCH_68030 && Operand->Info.Byte > 7))
                break;

            ACtx->Opcodes.Words[1] &= ~0x01e0;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)((M68K_WORD)Operand->Info.Byte << 5);
            Result = M68K_TRUE;
            break;

        case DAT_O_MRegP_S12E15EW1_S12E15EW2:
            Operand = GetNextOperand(ACtx, M68K_OT_MEM_REGISTER_PAIR);
            if (Operand == M68K_NULL)
                break;

            ACtx->Opcodes.Words[1] &= ~0xf000;
        
            if (IsValidRegister07(M68K_RT_D0, Operand->Info.RegisterPair.Register1))
                ACtx->Opcodes.Words[1] |= (M68K_WORD)((Operand->Info.RegisterPair.Register1 - M68K_RT_D0) << 12);
            else if (IsValidRegister07(M68K_RT_A0, Operand->Info.RegisterPair.Register1))
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(0x8000 | (Operand->Info.RegisterPair.Register1 - M68K_RT_A0) << 12);
            else
                break;
            
            ACtx->Opcodes.Words[2] &= ~0xf000;
        
            if (IsValidRegister07(M68K_RT_D0, Operand->Info.RegisterPair.Register2))
                ACtx->Opcodes.Words[2] |= (M68K_WORD)((Operand->Info.RegisterPair.Register2 - M68K_RT_D0) << 12);
            else if (IsValidRegister07(M68K_RT_A0, Operand->Info.RegisterPair.Register2))
                ACtx->Opcodes.Words[2] |= (M68K_WORD)(0x8000 | (Operand->Info.RegisterPair.Register2 - M68K_RT_A0) << 12);
            else
                break;
        
            Result = M68K_TRUE;
            break;

        case DAT_O_OW_S0E11EW1:
            // always applied to the last operand, so we'll check if it exists
            if (ACtx->NextOperand <= ACtx->Instruction->Operands)
                break;

            Operand = GetNextOperand(ACtx, M68K_OT_OFFSET_WIDTH);
            if (Operand == M68K_NULL)
                break;

            // the information for the {offset:width} must be valid i.e.
            //
            //  {0-31:1-32} number with number
            //  {0-31:Dn}   number with data register
            //  {Dn:1-32}   data register with number
            //  {Dn:Dm}     data register with data register
            //
            // data registers are represented with a value in [M68K_RT_D0, M68K_RT_D7];
            // numbers are represented as a negative value with the formula: -n-1
        
            // offset
            if (Operand->Info.OffsetWidth.Offset >= (M68K_INT)M68K_RT_D0 && Operand->Info.OffsetWidth.Offset <= (M68K_INT)M68K_RT_D7)
                // data register
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(0x0800 | ((Operand->Info.OffsetWidth.Offset - M68K_RT_D0) << 6));
            else if (Operand->Info.OffsetWidth.Offset >= -32 && Operand->Info.OffsetWidth.Offset <= -1)
                // number
                ACtx->Opcodes.Words[1] |= (M68K_WORD)((-Operand->Info.OffsetWidth.Offset - 1) << 6);
            else
                break;

            // width
            if (Operand->Info.OffsetWidth.Width >= (M68K_INT)M68K_RT_D0 && Operand->Info.OffsetWidth.Width <= (M68K_INT)M68K_RT_D7)
                // data register
                ACtx->Opcodes.Words[1] |= (M68K_WORD)(0x0020 | (Operand->Info.OffsetWidth.Width - M68K_RT_D0));
            else if (Operand->Info.OffsetWidth.Width >= -33 && Operand->Info.OffsetWidth.Width <= -2)
            {
                M68K_WORD Width = (M68K_WORD)(-Operand->Info.OffsetWidth.Width - 1);

                // number
                ACtx->Opcodes.Words[1] |= (Width != 32 ? Width : 0);
            }
            else
                break;

            Result = M68K_TRUE;
            break;

        case DAT_O_PReg_S10E12EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            for (M68K_UINT Index = 0; Index < 8; Index++)
            {
                if (_M68KMMUPRegisters[Index] == Operand->Info.Register)
                {
                    // same size?
                    if (_M68KMMUPRegisterSizes[Index] == ACtx->Instruction->Size)
                    {
                        ACtx->Opcodes.Words[1] &= ~0x1c00;
                        ACtx->Opcodes.Words[1] |= (M68K_WORD)(Index << 10);
                        Result = M68K_TRUE;
                    }
                    break;
                }
            }
            break;

        case DAT_O_RegAC0_I:
            Register = M68K_RT_AC0;

implicit_register:
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER);
            if (Operand == M68K_NULL)
                break;

            Result = (Operand->Info.Register == Register);
            break;

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
            Operand = GetNextOperand(ACtx, M68K_OT_REGISTER_LIST);
            if (Operand == M68K_NULL)
                break;

            // save the index of the word with the register list because we might need to invert all bits
            ACtx->Opcodes.Words[1] = Operand->Info.RegisterList;
            ACtx->UsesRegisterList = M68K_TRUE;

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
            Operand = GetNextOperand(ACtx, M68K_OT_IMMEDIATE_B);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.Byte == 0 || Operand->Info.Byte > 8)
                break;

            ACtx->Opcodes.Words[0] &= ~0x0e00;
            ACtx->Opcodes.Words[0] |= (M68K_WORD)((M68K_WORD)(Operand->Info.Byte != 8 ? Operand->Info.Byte : 0) << 9);
            Result = M68K_TRUE;
            break;

        case DAT_O_SKFac_S0E6EW1:
            Operand = GetNextOperand(ACtx, M68K_OT_STATIC_KFACTOR);
            if (Operand == M68K_NULL)
                break;

            if (Operand->Info.SByte < -64 || Operand->Info.SByte > 63)
                break;

            ACtx->Opcodes.Words[1] &= ~0x003f;
            ACtx->Opcodes.Words[1] |= (M68K_WORD)(Operand->Info.SByte & 0x3f);
            Result = M68K_TRUE;
            break;

        case DAT_S_B_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_B);
            break;

        case DAT_S_BWL_S6E7W0:
            WordIndex = 0;
            Value = 0x0040;
        
size_bwl:
            // 01
            if (ACtx->Instruction->Size == M68K_SIZE_W)
                ACtx->Opcodes.Words[WordIndex] |= Value;
            // 10
            else if (ACtx->Instruction->Size == M68K_SIZE_L)
                ACtx->Opcodes.Words[WordIndex] |= (Value << 1);
            // 00
            else if (ACtx->Instruction->Size != M68K_SIZE_B)
                break;

            Result = M68K_TRUE;
            break;

        case DAT_S_BWL_S6E7EW1:
            WordIndex = 1;
            Value = 0x0040;
            goto size_bwl;

        case DAT_S_BWL_S9E10W0:
            WordIndex = 0;
            Value = 0x0200;
            goto size_bwl;

        case DAT_S_FP_S10E12EW1:
            if (ACtx->Instruction->Size < M68K_SIZE_NONE || ACtx->Instruction->Size >= M68K_SIZE__SIZEOF__)
                break;

            Value = _M68KFpuSizeCodes[ACtx->Instruction->Size];
            if (Value >= 8)
                break;

            ACtx->Opcodes.Words[1] |= (M68K_WORD)(Value << 10);
            Result = M68K_TRUE;
            break;

        case DAT_S_L_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_L);
            break;

        case DAT_S_P_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_P);
            break;

        case DAT_S_PReg_S10E12EW1:
            // ignored because we'll make sure the size is valid when we analyse DAT_O_PReg_S10E12EW1
            Result = M68K_TRUE;
            break;

        case DAT_S_Q_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_Q);
            break;

        case DAT_S_W_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_W);
            break;

        case DAT_S_WL_S6E6W0:
            Value = 0x0040;

size_wl:
            if (ACtx->Instruction->Size == M68K_SIZE_L)
                ACtx->Opcodes.Words[0] |= Value;
            else if (ACtx->Instruction->Size != M68K_SIZE_W)
                break;

            Result = M68K_TRUE;
            break;

        case DAT_S_WL_S8E8W0:
            Value = 0x0100;
            goto size_wl;

        case DAT_S_X_I:
            Result = CheckInstructionSize(ACtx, M68K_SIZE_X);
            break;

        default:
            break;
        }
    }

    return Result;
}

// get the bits used by an extended fpu register
static M68K_BOOL GetBitsForExtendedFPURegister(PM68K_OPERAND Operand, PM68K_WORD Bits)
{
    if (Operand->Type == M68K_OT_REGISTER)
    {
        M68K_REGISTER_TYPE_VALUE Register = Operand->Info.Register;

        if (IsValidRegister07(M68K_RT_FP0, Register))
        {
            *Bits = (M68K_WORD)(Register - M68K_RT_FP0);
            return M68K_TRUE;
        }

        if (IsValidRegister07(M68K_RT_E0, Register))
        {
            *Bits = (M68K_WORD)(0x8 | (Register - M68K_RT_E0));
            return M68K_TRUE;
        }

        if (IsValidRegister07(M68K_RT_E8, Register))
        {
            *Bits = (M68K_WORD)(0x10 | (Register - M68K_RT_E8));
            return M68K_TRUE;
        }

        if (IsValidRegister07(M68K_RT_E16, Register))
        {
            *Bits = (M68K_WORD)(0x18 | (Register - M68K_RT_E16));
            return M68K_TRUE;
        }
    }

    *Bits = 0;
    return M68K_FALSE;
}

// get the next operand in the instruction
static PM68K_OPERAND GetNextOperand(PASM_CTX ACtx, M68K_OPERAND_TYPE_VALUE OperandType /*M68K_OT__SIZEOF__ = any*/)
{
    // too many operands?
    PM68K_OPERAND Operand = (ACtx->NextOperand)++;
    if (Operand >= (ACtx->Instruction->Operands + M68K_MAXIMUM_NUMBER_OPERANDS))
        return M68K_NULL;

    // same type?
    if (Operand->Type != OperandType && OperandType != M68K_OT__SIZEOF__)
        // try the implicit conversion to the required operand type
        return (M68KConvertOperandTo(ACtx->Flags, Operand, OperandType, ACtx->NextConvOperand) ? (ACtx->NextConvOperand)++ : M68K_NULL);

    return Operand;
}

// check if a base register is valid
static M68K_BOOL IsValidBaseRegister07(AMODE_FLAGS Flags, M68K_REGISTER_TYPE_VALUE Register, PM68K_WORD RegisterIndex, PM68K_BOOL UsingBank1)
{
    // An
    if (IsValidRegister07(M68K_RT_A0, Register))
    {
        *RegisterIndex = (M68K_WORD)(Register - M68K_RT_A0);

        if (UsingBank1 != M68K_NULL)
            *UsingBank1 = M68K_FALSE;

        return M68K_TRUE;
    }

    if ((Flags & (AMF_BASE_BANK_S8E8EW1 | AMF_VECTOR_MODE_S8E8W0 | AMF_BANK_EW_A | AMF_BANK_EW_B)) != 0)
    {
        if (IsValidRegister07(M68K_RT_B0, Register))
        {
            *RegisterIndex = (M68K_WORD)(Register - M68K_RT_B0);

            if (UsingBank1 != M68K_NULL)
                *UsingBank1 = M68K_TRUE;

            return M68K_TRUE;
        }
    }

    return M68K_FALSE;
}

// check if a register is valid
static M68K_BOOL IsValidRegister07(M68K_REGISTER_TYPE_VALUE BaseRegister, M68K_REGISTER_TYPE_VALUE Register)
{
    return (Register >= BaseRegister && Register < (BaseRegister + 8));
}

// save the PC offset for the displacements
static M68K_BOOL SavePCOffset(PASM_CTX ACtx, M68K_BYTE PCOffset)
{
    // already saved?
    if (ACtx->Opcodes.PCOffset == 0)
    {
        ACtx->Opcodes.PCOffset = PCOffset;
        return M68K_TRUE;
    }
    else
        return M68K_FALSE;
}

// save a word in the context
static M68K_BOOL SaveWord(PASM_CTX ACtx, M68K_WORD Word)
{
    // too many words?
    if (ACtx->Opcodes.NumberWords >= M68K_MAXIMUM_SIZE_INSTRUCTION_WORDS)
        return M68K_FALSE;

    ACtx->Opcodes.Words[ACtx->Opcodes.NumberWords++] = Word;
    return M68K_TRUE;
}

// update the instruction with the bank information for the base register
static M68K_BOOL UpdateBaseBank(PASM_CTX ACtx, AMODE_FLAGS Flags, M68K_BOOL UsingBank1)
{
    // bit must be set?
    if (!UsingBank1)
        return M68K_TRUE;

    // where is it located?
    if ((Flags & AMF_BASE_BANK_S8E8EW1) != 0)
    {
        // the first extension word is available?
        if (ACtx->Opcodes.NumberWords >= 2)
        {
            ACtx->Opcodes.Words[1] |= 0x0100;
            return M68K_TRUE;
        }
    }
    else if ((Flags & AMF_BANK_EW_A) != 0)
    {
        ACtx->BankExtensionInfo.A = 1;
        ACtx->AFlags |= ACF_BANK_EXTENSION_WORD;
        return M68K_TRUE;
    }
    else if ((Flags & AMF_BANK_EW_B) != 0)
    {
        ACtx->BankExtensionInfo.B = 1;
        ACtx->AFlags |= ACF_BANK_EXTENSION_WORD;
        return M68K_TRUE;
    }
    // it must be for VEA
    else if (UpdateVModeBitA(ACtx, Flags, M68K_TRUE))
        return M68K_TRUE;

    return M68K_FALSE;
}

// update a word with the index of a base register that can be banked using one bit
static M68K_BOOL UpdateBaseRegister(PM68K_WORD Word, M68K_REGISTER_TYPE Register, M68K_UINT StartBit, M68K_UINT BankBit)
{
    M68K_BOOL UsingBank1;
    M68K_WORD RegisterIndex;

    if (IsValidBaseRegister07(AMF_BASE_BANK_S8E8EW1, (M68K_REGISTER_TYPE_VALUE)Register, &RegisterIndex, &UsingBank1))
    {
        (*Word) &= ~(7 << StartBit);
        (*Word) |= (M68K_WORD)(RegisterIndex << StartBit);
    
        if (UsingBank1)
            (*Word) |= (M68K_WORD)(1 << BankBit);
        else
            (*Word) &= (M68K_WORD)~(M68K_WORD)(1 << BankBit);

        return M68K_TRUE;
    }
    else
        return M68K_FALSE;
}

// update the (full) extension word with the information from the base register
static M68K_BOOL UpdateEWordBaseRegister(PASM_CTX ACtx, AMODE_FLAGS Flags, PM68K_WORD EWord, PM68K_WORD Register, PM68K_OPERAND Operand)
{
    M68K_BOOL UsingBank1;

    // register is suppressed?
    if (Operand->Info.Memory.Base == M68K_RT_NONE)
        (*EWord) |= 0x0080;

    // is it a valid address register?
    else if (IsValidBaseRegister07(Flags, Operand->Info.Memory.Base, Register, &UsingBank1))
        return UpdateBaseBank(ACtx, Flags, UsingBank1);

    else
        return M68K_FALSE;

    return M68K_TRUE;
}

// update the (full) extension word with the information from the base displacement
static M68K_BOOL UpdateEWordBaseDisplacement(PM68K_WORD EWord, M68K_SIZE_VALUE BaseDispSize, M68K_SDWORD BaseDispValue, PM68K_SIZE_VALUE DispSize)
{
    // no displacement?
    if (BaseDispSize == M68K_SIZE_NONE || BaseDispValue == 0)
    {
        (*EWord) |= (0b01 << 4);
        *DispSize = M68K_SIZE_NONE;
    }
    else if (BaseDispSize == M68K_SIZE_W)
    {
        if (!CheckDisplacementW(BaseDispValue))
            return M68K_FALSE;
        
        (*EWord) |= (0b10 << 4);
        *DispSize = M68K_SIZE_W;
    }
    else if (BaseDispSize == M68K_SIZE_L)
    {
        (*EWord) |= (0b11 << 4);
        *DispSize = M68K_SIZE_L;
    }
    else
        return M68K_FALSE;

    // the displacement value must be written by the caller function after the extension word
    return M68K_TRUE;
}

// update the (brief/full) extension word with the information from the index register
static M68K_BOOL UpdateEWordIndexRegister(PM68K_WORD EWord, PM68K_OPERAND Operand, M68K_BOOL AllowSuppressed)
{
    M68K_BOOL IgnoreSizeScale = M68K_FALSE;

    // register is suppressed?
    if (Operand->Info.Memory.Index.Register == M68K_RT_NONE)
    {
        // can it be suppressed?
        if (!AllowSuppressed)
            return M68K_FALSE;

        (*EWord) |= 0x0040;
        IgnoreSizeScale = M68K_TRUE;
    }

    // index register can be an address register...
    else if (IsValidRegister07(M68K_RT_A0, Operand->Info.Memory.Index.Register))
        (*EWord) |= (0x8000 | ((M68K_WORD)(Operand->Info.Memory.Index.Register - M68K_RT_A0) << 12));

    // ... or a data register
    else if (IsValidRegister07(M68K_RT_D0, Operand->Info.Memory.Index.Register))
        (*EWord) |= ((M68K_WORD)(Operand->Info.Memory.Index.Register - M68K_RT_D0) << 12);

    else
        return M68K_FALSE;

    if (!IgnoreSizeScale)
    {
        // index size is valid?
        if (Operand->Info.Memory.Index.Size == M68K_SIZE_L)
            (*EWord) |= 0x0800;
        else if (Operand->Info.Memory.Index.Size != M68K_SIZE_W)
            return M68K_FALSE;

        // save the scale
        if (Operand->Info.Memory.Scale < M68K_SCALE_1 || Operand->Info.Memory.Scale > M68K_SCALE_8)
            return M68K_FALSE;

        (*EWord) |= ((M68K_WORD)(Operand->Info.Memory.Scale - M68K_SCALE_1) << 9);
    }

    return M68K_TRUE;
}

// update the (full) extension word with the information from the outer displacement
static M68K_BOOL UpdateEWordOuterDisplacement(PM68K_WORD EWord, PM68K_OPERAND Operand, PM68K_SIZE_VALUE DispSize)
{
    // no displacement?
    if (Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_NONE || Operand->Info.Memory.Displacement.OuterValue == 0)
    {
        (*EWord) |= 0b01;
        *DispSize = M68K_SIZE_NONE;
    }
    else if (Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_W)
    {
        if (!CheckDisplacementW(Operand->Info.Memory.Displacement.OuterValue))
            return M68K_FALSE;
        
        (*EWord) |= 0b10;
        *DispSize = M68K_SIZE_W;
    }
    else if (Operand->Info.Memory.Displacement.OuterSize == M68K_SIZE_L)
    {
        (*EWord) |= 0b11;
        *DispSize = M68K_SIZE_L;
    }
    else
        return M68K_FALSE;

    // the displacement value must be written by the caller function after the extension word and possibly the base displacement
    return M68K_TRUE;
}

// update a word with the index of a register
static M68K_BOOL UpdateRegister(PM68K_WORD Word, M68K_REGISTER_TYPE BaseRegister, M68K_REGISTER_TYPE Register, M68K_WORD StartBit)
{
    if (!IsValidRegister07(BaseRegister, Register))
        return M68K_FALSE;

    (*Word) &= ~(7 << StartBit);
    (*Word) |= (M68K_WORD)((Register - BaseRegister) << StartBit);
    return M68K_TRUE;
}

// update A bit used by the vector addressing mode
static M68K_BOOL UpdateVModeBitA(PASM_CTX ACtx, AMODE_FLAGS Flags, M68K_BOOL BitAMustBeSet)
{
    // bit must be set?
    if (!BitAMustBeSet)
        return M68K_TRUE;

    // where is it located?
    if ((Flags & AMF_VECTOR_MODE_S8E8W0) != 0)
    {
        // the opcode word is available? (must be ?!?!?)
        if (ACtx->Opcodes.NumberWords >= 1)
        {
            ACtx->Opcodes.Words[0] |= 0x0100;
            return M68K_TRUE;
        }
    }

    return M68K_FALSE;
}

// assemble "Instruction" for the supplied "Architectures" and write the result to "OutBuffer";
// returns the number of words copied to "OutBuffer" or 0 if an error occurs (can't assemble or buffer too small);
// you can specify "OutBuffer" as M68K_NULL to determine the total number of words that are required
M68K_WORD M68KAssemble(PM68K_WORD Address, PM68K_INSTRUCTION Instruction, M68K_ARCHITECTURE Architectures, M68K_ASM_FLAGS Flags, PM68K_WORD OutBuffer /*M68K_NULL = return total size*/, M68K_WORD OutSize /*maximum in words*/)
{
    if (Instruction != M68K_NULL && Architectures != 0)
    {
        // note: the fields "Start", "Stop", "End", "InternalType" and "PCOffset" in the instruction are always ignored
        // the instruction type is valid?
        if (Instruction->Type >= M68K_IT_INVALID && Instruction->Type < M68K_IT__SIZEOF__)
        {
            // init
            ASM_CTX ACtx;

            ACtx.Address = Address;
            ACtx.Instruction = Instruction;
            ACtx.Architectures = Architectures;
            ACtx.Flags = Flags;

            // use the assembler tables to analyse all possible instructions with the same type;
            // we might have to analyse different opcode maps and the mask will tell us which ones are required;
            M68K_WORD OpcodeMapMask = _M68KAsmOpcodeMaps[ACtx.Instruction->Type - 1];
            PM68KC_WORD M68K_CONST *NextOpcodeMap = _M68KDisOpcodeMaps;
            PM68KC_WORD AsmWords = _M68KAsmWords + _M68KAsmIndexFirstWord[ACtx.Instruction->Type - 1];

            for (; OpcodeMapMask != 0; OpcodeMapMask >>= 1, NextOpcodeMap++)
            {
                // bit is set?
                if ((OpcodeMapMask & 1) != 0)
                {
                    // get the start of the disassembler table for the current opcode map
                    PM68KC_WORD OpcodeMap = *NextOpcodeMap;

                    M68K_WORD OpcodeMapIndex;
                    while ((OpcodeMapIndex = *(AsmWords++)) != 0)
                    {
                        // check the next instruction
                        PM68KC_WORD NextWord = OpcodeMap + OpcodeMapIndex;
                        
                        ACtx.IInfo = _M68KInstrTypeInfos + NextWord[1];

                        // start by checking the architecture
                        if ((ACtx.IInfo->Architectures & ACtx.Architectures) != 0)
                        {
                        ACtx.NextOperand = ACtx.Instruction->Operands;
                        ACtx.NextConvOperand = ACtx.ConvOperands;
                        ACtx.Opcodes.NumberWords = 1;
                        ACtx.Opcodes.PCOffset = 0;
                        ACtx.Opcodes.Words[0] = NextWord[0];
                        ACtx.BankExtensionInfo.A = 0;
                        ACtx.BankExtensionInfo.B = 0;
                        ACtx.BankExtensionInfo.C = 0;
                        ACtx.AFlags = 0;
                        ACtx.UsesPredecrement = M68K_FALSE;
                        ACtx.UsesRegisterList = M68K_FALSE;

                            // requires one or more extension words?
                            if ((ACtx.IInfo->IFlags & IF_EXTENSION_WORD_1) != 0)
                            {
                                ACtx.Opcodes.Words[1] = 0;
                                ACtx.Opcodes.NumberWords = 2;
                            }

                            if ((ACtx.IInfo->IFlags & IF_EXTENSION_WORD_2) != 0)
                            {
                                ACtx.Opcodes.Words[2] = 0;
                                ACtx.Opcodes.NumberWords = 3;
                            }

                            // execute all actions; we'll stop when we have an error OR we reached the maximum number of actions (success)
                            M68K_UINT ActionIndex;
                            M68K_BOOL Stop = M68K_FALSE;

                            for (ActionIndex = 2; ActionIndex < NUM_WORDS_INSTR; ActionIndex++)
                                if (!ExecuteAction(&ACtx, (DISASM_ACTION_TYPE)NextWord[ActionIndex], &Stop))
                                    break;

                            // success?
                            if (ActionIndex == NUM_WORDS_INSTR || Stop)
                            {
                                // the remaining operands must have "none" type;
                                // if the instruction can use the bank extension word 
                                // then we might have the possibility of using an extra operand as destination
                                PM68K_OPERAND LastOperand;
                                AMODE_FLAGS AFlags = CheckCanUseBankExtensionWord(&ACtx, M68K_NULL);

                                for (LastOperand = ACtx.Instruction->Operands + M68K_MAXIMUM_NUMBER_OPERANDS; ACtx.NextOperand < LastOperand; ACtx.NextOperand++)
                                {
                                    if (ACtx.NextOperand->Type != M68K_OT_NONE)
                                    {
                                        // is this an instruction that allows an extra operand as destination?
                                        if ((AFlags & AMF_BANK_EW_C) != 0)
                                        {
                                            // the instructions must have
                                            // 1) at least two operands before the one we are checking
                                            // 2) the second operand must be an fpu or extended register
                                            // 3) the operadn we are checking must be an fpu or extended register
                                            if (ACtx.NextOperand >= (ACtx.Instruction->Operands + 2))
                                            {
                                                M68K_WORD BitsB;
                                                if (GetBitsForExtendedFPURegister(ACtx.NextOperand - 1, &BitsB))
                                                {
                                                    // make sure we use the bits from the bank extension word,
                                                    // because the operand will always be fp0-fp7
                                                    if ((ACtx.AFlags & ACF_BANK_EXTENSION_WORD) != 0)
                                                        BitsB |= (ACtx.BankExtensionInfo.B << 3);

                                                    if (GetBitsForExtendedFPURegister(ACtx.NextOperand, &(ACtx.BankExtensionInfo.C)))
                                                    {
                                                        // the bits for C are always xored with the bits for B;
                                                        // this ensures that C = 0 when the destination is not used/is the same
                                                        ACtx.BankExtensionInfo.C ^= BitsB;
                                                        ACtx.AFlags |= ACF_BANK_EXTENSION_WORD;

                                                        // make sure that the remaining operands are not used as destination
                                                        AFlags = 0;
                                                        continue;
                                                    }
                                                }
                                            }
                                        }

                                        // no! error
                                        break;
                                    }
                                }

                                // all operands ok?
                                if (ACtx.NextOperand == LastOperand)
                                {
                                    // invert the list of registers?
                                    if (ACtx.UsesPredecrement && ACtx.UsesRegisterList)
                                        ACtx.Opcodes.Words[1] = _M68KDisInvertWordBits(ACtx.Opcodes.Words[1]);

                                    // requires a bank extension word?
                                    M68K_WORD FinalNumberWords;
                                    M68K_WORD BankExtensionWord;
                                    M68K_BOOL RequiresBankExtensionWord = ((ACtx.AFlags & ACF_BANK_EXTENSION_WORD) != 0);

                                    if (RequiresBankExtensionWord)
                                    {
                                        // the total size of the opcodes in bytes is between 4 and 10 bytes?
                                        // in other words, the instruction can only use a maximum of 4 words;
                                        // everything else cannot be encoded
                                        if (ACtx.Opcodes.NumberWords > 4)
                                            goto error;
                                        
                                        // build the extension word
                                        BankExtensionWord = 0x7100;

                                        // Size
                                        BankExtensionWord |= ((ACtx.Opcodes.NumberWords - 1) << 6);

                                        // A
                                        BankExtensionWord |= ((ACtx.BankExtensionInfo.A & 3) << 2);

                                        // B
                                        BankExtensionWord |= (ACtx.BankExtensionInfo.B & 3);

                                        // C
                                        BankExtensionWord |= ((ACtx.BankExtensionInfo.C & 7) << 9);
                                        BankExtensionWord |= ((ACtx.BankExtensionInfo.C & 0x18) << 1);

                                        FinalNumberWords = 1 + ACtx.Opcodes.NumberWords;
                                    }
                                    else
                                    {
                                        BankExtensionWord = 0;
                                        FinalNumberWords = ACtx.Opcodes.NumberWords;
                                    }

                                    // copy to the output?
                                    if (OutBuffer == M68K_NULL)
                                        return (M68K_UINT)FinalNumberWords;

                                    // enough space?
                                    if (OutSize < FinalNumberWords)
                                        goto error;

                                    // write the bank extension word?
                                    if (RequiresBankExtensionWord)
                                        M68KWriteWord(OutBuffer++, BankExtensionWord);

                                    // write all opcodes
                                    for (OutSize = 0; OutSize < ACtx.Opcodes.NumberWords; OutSize++)
                                        M68KWriteWord(OutBuffer++, ACtx.Opcodes.Words[OutSize]);

                                    return FinalNumberWords;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

error:
    // error
    return 0;
}
