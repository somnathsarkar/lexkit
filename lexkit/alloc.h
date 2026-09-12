#ifndef __LEXKIT_ALLOC__
#define __LEXKIT_ALLOC__

#include <lexkit/types.h>

typedef struct LkAllocator LkAllocator;

struct LkAllocator
{
  void* (*reserve)(void* user, u64 size);
  bool  (*commit)(void* user, void* ptr, u64 size);
  void  (*release)(void* user, void* ptr, u64 size);
  void* user;
};

LkAllocator* lkAllocatorDefault(void);
void* lkAllocatorAlloc(LkAllocator* alloc, u64 sz, u64 aln);
void* lkAllocatorAllocArray(LkAllocator* alloc, u64 sz, u64 aln, u64 count);
void lkAllocatorFree(LkAllocator* alloc, void* ptr, u64 sz);

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
#define AAlloc(alc, tp) (tp*)lkAllocatorAlloc((alc), sizeof(tp), _Alignof(tp))
#define AAllocArray(alc, tp, count) (tp*)lkAllocatorAllocArray((alc), sizeof(tp), _Alignof(tp), (count))
#define AFree(alc, ptr, tp, count) lkAllocatorFree((alc), (ptr), sizeof(tp) * (count))

#endif // __LEXKIT_ALLOC__
