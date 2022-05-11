#include "m68kinternal.h"

#define _C  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_CLEAR
#define _N  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_NONE
#define _O  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_OPERATION
#define _S  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_SET
#define _T  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_TEST
#define _TO (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)(_T|_O)
#define _U  (M68K_CONDITION_CODE_FLAG_ACTION_VALUE)M68K_CCA_UNDEFINED

// actions for each condition code flag
M68KC_CONDITION_CODE_FLAG_ACTIONS _M68KConditionCodeFlagActions[CCF__SIZEOF__] =
{
    {{_N , _N, _N , _N, _N}}, // CCF_N_N_N_N_N
    {{_N , _C, _S , _C, _C}}, // CCF_N_C_S_C_C
    {{_N , _N, _O , _N, _N}}, // CCF_N_N_O_N_N
    {{_N , _O, _O , _C, _C}}, // CCF_N_O_O_C_C
    {{_N , _O, _O , _C, _O}}, // CCF_N_O_O_C_O
    {{_N , _O, _O , _O, _C}}, // CCF_N_O_O_O_C
    {{_N , _O, _O , _O, _O}}, // CCF_N_O_O_O_O
    {{_N , _O, _U , _U, _U}}, // CCF_N_O_U_U_U
    {{_N , _U, _O , _U, _O}}, // CCF_N_U_O_U_O
    {{_O , _O, _O , _C, _O}}, // CCF_O_O_O_C_O
    {{_O , _O, _O , _O, _O}}, // CCF_O_O_O_O_O
    {{_TO, _O, _O , _C, _O}}, // CCF_TO_O_O_C_O
    {{_TO, _O, _TO, _O, _O}}, // CCF_TO_O_TO_O_O
    {{_TO, _U, _TO, _U, _O}}, // CCF_TO_U_TO_U_O
};
