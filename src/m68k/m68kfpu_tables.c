#include "m68kinternal.h"

// table of fpu sizes
M68KC_SIZE_VALUE _M68KFpuSizes[8] =
{
    M68K_SIZE_L,    // 000 — Long-Word Integer (L)
    M68K_SIZE_S,    // 001 — Single-Precision Real (S)
    M68K_SIZE_X,    // 010 — Extended-Precision Real (X)
    M68K_SIZE_P,    // 011 — Packed-Decimal Real (P)
    M68K_SIZE_W,    // 100 — Word Integer (W)
    M68K_SIZE_D,    // 101 — Double-Precision Real (D)
    M68K_SIZE_B,    // 110 — Byte Integer (B)
    M68K_SIZE_NONE, // 111 - Reserved
};

// table of fpu size codes for each size
M68KC_BYTE _M68KFpuSizeCodes[M68K_SIZE__SIZEOF__] =
{
    8,      // M68K_SIZE_NONE
    0b110,  // M68K_SIZE_B
    0b101,  // M68K_SIZE_D
    0b000,  // M68K_SIZE_L
    0b011,  // M68K_SIZE_P
    8,      // M68K_SIZE_Q
    0b001,  // M68K_SIZE_S
    0b100,  // M68K_SIZE_W
    0b010,  // M68K_SIZE_X
};
