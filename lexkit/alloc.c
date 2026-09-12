#include <lexkit/alloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define LK_ARENA_RESERVE (1024llu * 1024llu * 1024llu * 16)
#define LK_ARENA_CHUNK (1024llu * 1024llu)

static void* lkAllocatorDefaultReserve(void* user, u64 size)
{
  return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}

static bool lkAllocatorDefaultCommit(void* user, void* ptr, u64 size)
{
  return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

static void lkAllocatorDefaultRelease(void* user, void* ptr, u64 size)
{
  VirtualFree(ptr, 0, MEM_RELEASE);
}

static LkAllocator g_lk_allocator_default = {
  lkAllocatorDefaultReserve,
  lkAllocatorDefaultCommit,
  lkAllocatorDefaultRelease,
  NULL,
};

LkAllocator* lkAllocatorDefault(void)
{
  return &g_lk_allocator_default;
}

void* lkAllocatorAlloc(LkAllocator* alloc, u64 sz, u64 aln)
{
  if (alloc == NULL)
    alloc = lkAllocatorDefault();
  assert(aln > 0 && (aln & (aln - 1)) == 0);
  void* block = alloc->reserve(alloc->user, sz);
  if (block == NULL)
    return NULL;
  assert(((u64)block % aln) == 0);
  if (!alloc->commit(alloc->user, block, sz))
  {
    alloc->release(alloc->user, block, sz);
    return NULL;
  }
  return block;
}

void* lkAllocatorAllocArray(LkAllocator* alloc, u64 sz, u64 aln, u64 count)
{
  return lkAllocatorAlloc(alloc, sz * count, aln);
}

void lkAllocatorFree(LkAllocator* alloc, void* ptr, u64 sz)
{
  if (alloc == NULL)
    alloc = lkAllocatorDefault();
  if (ptr == NULL)
    return;
  alloc->release(alloc->user, ptr, sz);
}

LkArena* lkArenaCreate()
{
  return lkArenaCreateFrom(NULL);
}

LkArena* lkArenaCreateFrom(LkAllocator* alloc)
{
  if (alloc == NULL)
    alloc = lkAllocatorDefault();
  LkArena* arena = (LkArena*)calloc(1, sizeof(LkArena));
  arena->pos = 0;
  arena->reserved = LK_ARENA_RESERVE;
  arena->committed = 0;
  arena->alloc = alloc;
  arena->data = alloc->reserve(alloc->user, LK_ARENA_RESERVE);
  assert(arena->data != NULL);
  arena->alt = (LkArena*)calloc(1, sizeof(LkArena));
  arena->alt->pos = 0;
  arena->alt->reserved = LK_ARENA_RESERVE;
  arena->alt->committed = 0;
  arena->alt->alloc = alloc;
  arena->alt->data = alloc->reserve(alloc->user, LK_ARENA_RESERVE);
  assert(arena->alt->data != NULL);
  arena->alt->alt = arena;
  return arena;
}

LkArena* lkArenaCreateFixed(u64 sz)
{
  LkArena* arena = (LkArena*)calloc(1, sizeof(LkArena));
  arena->pos = 0;
  arena->reserved = sz;
  arena->committed = sz;
  arena->alloc = lkAllocatorDefault();
  arena->data = VirtualAlloc(NULL, sz, MEM_COMMIT, PAGE_READWRITE);
  arena->alt = (LkArena*)calloc(1, sizeof(LkArena));
  arena->alt->pos = 0;
  arena->alt->reserved = sz;
  arena->alt->committed = sz;
  arena->alt->alloc = lkAllocatorDefault();
  arena->alt->data = VirtualAlloc(NULL, sz, MEM_COMMIT, PAGE_READWRITE);
  arena->alt->alt = arena;
  return arena;
}

void lkArenaDestroy(LkArena* arena)
{
  arena->alt->alloc->release(arena->alt->alloc->user, arena->alt->data, arena->alt->reserved);
  free(arena->alt);
  arena->alloc->release(arena->alloc->user, arena->data, arena->reserved);
  free(arena);
}

static void ArenaCommit(LkArena* arena, u64 pos_required)
{
  u64 pos_target = LK_ARENA_CHUNK * ((pos_required + (LK_ARENA_CHUNK - 1)) / LK_ARENA_CHUNK);
  bool ok = arena->alloc->commit(arena->alloc->user, (char*)(arena->data) + arena->committed, pos_target - arena->committed);
  assert(ok);
  arena->committed = pos_target;
}

void* lkArenaPush(LkArena* arena, u64 sz, u64 aln)
{
  assert(aln > 0);
  aln = (aln > 0) ? aln : 1;
  u64 pos_aln = ((arena->pos + aln - 1) / aln) * aln;
  u64 pos_target = pos_aln + sz;
  if (pos_target > arena->committed)
    ArenaCommit(arena, pos_target);
  char* result = ((char*)arena->data) + pos_aln;
  arena->pos = pos_target;
  assert(arena->pos <= arena->reserved);
  return result;
}

void* lkArenaPushArray(LkArena* arena, u64 sz, u64 aln, u64 count)
{
  assert(aln > 0);
  aln = (aln > 0) ? aln : 1;
  u64 pos_aln = ((arena->pos + aln - 1) / aln) * aln;
  u64 pos_target = pos_aln + sz * count;
  if (pos_target > arena->committed)
    ArenaCommit(arena, pos_target);
  char* result = ((char*)arena->data) + pos_aln;
  arena->pos = pos_target;
  assert(arena->pos <= arena->reserved);
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
