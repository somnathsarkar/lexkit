#ifndef __LEXKIT_TWOSTEP__
#define __LEXKIT_TWOSTEP__

#include <lexkit/types.h>
#include <lexkit/alloc.h>

#include <immintrin.h>

typedef struct
{
  i32* block0;
  i32* block1;
  u64 block0_len;
  u64 block1_len;
} LkTwoStep;

typedef enum
{
  UNIFMT_A,
  UNIFMT_B,
  UNIFMT_C,
  UNIFMT_D,
  UNIFMT_E,

  UNIFMT_Count
} UNIFMT;

LkTwoStep* LkTwoStepCreate(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, UNIFMT unifmt);
i32 LkTwoStepLookup(LkTwoStep* ts, u32 ch);

#ifdef __AVX2__
__m128i LkTwoStepLookupAvx2(LkTwoStep* ts, __m128i ch);
#endif

#endif // __LEXKIT_TWOSTEP__