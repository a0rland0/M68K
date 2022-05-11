#include "m68kinternal.h"

// table with the index of first words for each master instruction type
M68KC_WORD _M68KAsmIndexFirstWord[M68K_IT__SIZEOF__ - 1] =
{
#include "gen/index_first_words.h"
};

// table with the opcode map mask for each master instruction type
M68KC_WORD _M68KAsmOpcodeMaps[M68K_IT__SIZEOF__- 1] =
{
#include "gen/opcode_maps.h"
};

// table with the words for all master instruction types
M68KC_WORD _M68KAsmWords[] =
{
#include "gen/words.h"
};
