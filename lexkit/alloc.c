#include <lexkit/alloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

LkArena* lkArenaCreate(u64 sz)
{
  LkArena* arena = (LkArena*)calloc(1, sizeof(LkArena));
  arena->pos = 0;
  arena->size = sz;
  arena->data = (u8*)calloc(sz, sizeof(u8));
  arena->alt = (LkArena*)calloc(1, sizeof(LkArena));
  arena->alt->pos = 0;
  arena->alt->size = sz;
  arena->alt->data = (u8*)calloc(sz, sizeof(u8));
  arena->alt->alt = arena;
  return arena;
}

void* lkArenaPush(LkArena* arena, u64 sz, u64 aln)
{
  assert(aln > 0);
  aln = (aln > 0) ? aln : 1;
  arena->pos = ((arena->pos + aln - 1) / aln) * aln;
  char* result = ((char*)arena->data) + arena->pos;
  arena->pos += sz;
  assert(arena->pos <= arena->size);
  return result;
}

void* lkArenaPushArray(LkArena* arena, u64 sz, u64 aln, u64 count)
{
  assert(aln > 0);
  aln = (aln > 0) ? aln : 1;
  arena->pos = ((arena->pos + aln - 1) / aln) * aln;
  char* result = ((char*)arena->data) + arena->pos;
  arena->pos += sz * count;
  assert(arena->pos <= arena->size);
  return result;
}

u64 lkArenaGetPos(LkArena* arena)
{
  return arena->pos;
}

void lkArenaRestore(LkArena* arena, u64 pos)
{
  assert(pos >= 0 && pos <= arena->pos);
  if (pos < arena->pos)
  {
    memset((char*)(arena->data) + pos, 0, arena->pos - pos);
    arena->pos = pos;
  }
}
