#ifndef __LEXKIT_JOB__
#define __LEXKIT_JOB__

#include <threads.h>

#include <lexkit/alloc.h>

#define LEXKIT_MAX_WORKERS 32
#define LEXKIT_MAX_JOBS 1024

typedef void (*LkJobFunc)(void*);

typedef struct
{
  LkJobFunc func;
  void* data;
} LkJob;

typedef struct
{
  LkAllocator* alloc;
  int num_workers;
  thrd_t workers[LEXKIT_MAX_WORKERS];
  volatile int end;
  volatile int start;
  volatile int terminate;
  volatile int complete;
  void* semaphore;
  LkJob queue[LEXKIT_MAX_JOBS];
} LkJobQueue;

LkJobQueue* lkJobQueueCreate(LkAllocator* alloc, int num_workers);
void lkJobQueuePush(LkJobQueue* queue, LkJob job);
void lkJobQueueWait(LkJobQueue* queue);
void lkJobQueueDestroy(LkJobQueue* queue);

#endif