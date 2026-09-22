#ifndef __LEXKIT_JOB__
#define __LEXKIT_JOB__

#include <threads.h>

#include <lexkit/alloc.h>

#define LEXKIT_MAX_WORKERS 32
#define LEXKIT_MAX_JOBS 1024

// worker_id = -1 for jobs on the main thread
typedef void (*LkJobFunc)(int worker_id, void* data);

typedef struct
{
  LkJobFunc func;
  void* data;
} LkJob;

typedef struct LkJobQueue LkJobQueue;

typedef struct
{
  i32 worker_id;
  LkJobQueue* queue;
} LkWorkerContext;

struct LkJobQueue
{
  LkAllocator* alloc;
  int num_workers;
  thrd_t workers[LEXKIT_MAX_WORKERS];
  LkWorkerContext worker_ctx[LEXKIT_MAX_WORKERS];
  volatile int end;
  volatile int start;
  volatile int terminate;
  volatile int complete;
  void* semaphore;
  LkJob queue[LEXKIT_MAX_JOBS];
};

LkJobQueue* lkJobQueueCreate(LkAllocator* alloc, int num_workers);
void lkJobQueuePush(LkJobQueue* queue, LkJob job);
void lkJobQueueWait(LkJobQueue* queue);
void lkJobQueueDestroy(LkJobQueue* queue);

#endif