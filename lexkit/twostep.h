#ifndef __LEXKIT_TWOSTEP__
#define __LEXKIT_TWOSTEP__

#include <lexkit/types.h>
#include <lexkit/alloc.h>

typedef struct
{
  i32* block0;
  i32* block1;
  u64 block0_len;
  u64 block1_len;
} LkTwoStep;

LkTwoStep* LkTwoStepCreate(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max);
i32 LkTwoStepLookup(LkTwoStep* ts, u32 ch);

#endif // __LEXKIT_TWOSTEP__