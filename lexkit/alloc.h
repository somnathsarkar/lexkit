#ifndef __LEXKIT_ALLOC__
#define __LEXKIT_ALLOC__

#include <lexkit/types.h>

struct LkArena
{
  void *data;
  u64 reserved;
  u64 committed;
  u64 pos;
  struct LkArena* alt;
};

typedef struct LkArena LkArena;

LkArena* lkArenaCreate(void);
LkArena* lkArenaCreateFixed(u64 sz);
void lkArenaDestroy(LkArena* arena);
void* lkArenaPush(LkArena* arena, u64 sz, u64 aln);
void* lkArenaPushArray(LkArena* arena, u64 sz, u64 aln, u64 count);
u64 lkArenaGetPos(LkArena* arena);
void lkArenaRestore(LkArena* arena, u64 pos);

#define APush(arena, tp) (tp*)lkArenaPush((arena), sizeof(tp), _Alignof(tp))
#define APushArray(arena, tp, count) (tp*)lkArenaPushArray((arena), sizeof(tp), _Alignof(tp), (count))

#endif // __LEXKIT_ALLOC__
