#include "m68kinternal.h"

// table of p-registers
M68KC_REGISTER_TYPE_VALUE _M68KMMUPRegisters[8] =
{
    M68K_RT_TC,
    M68K_RT_DRP,
    M68K_RT_SRP,
    M68K_RT_CRP,
    M68K_RT_CAL,
    M68K_RT_VAL,
    M68K_RT_SCC,
    M68K_RT_AC,
};

// table of implicit size for each p-registers
M68KC_SIZE_VALUE _M68KMMUPRegisterSizes[8] =
{
    M68K_SIZE_L,    // M68K_RT_TC
    M68K_SIZE_Q,    // M68K_RT_DRP
    M68K_SIZE_Q,    // M68K_RT_SRP
    M68K_SIZE_Q,    // M68K_RT_CRP
    M68K_SIZE_B,    // M68K_RT_CAL
    M68K_SIZE_B,    // M68K_RT_VAL
    M68K_SIZE_B,    // M68K_RT_SCC
    M68K_SIZE_W,    // M68K_RT_AC
};
