#include <lexkit/job.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Returns true if we performed a job, false otherwise

static bool WorkerPerformJob(int worker_id, LkJobQueue* queue)
{
  int queue_front = queue->start;
  int queue_end = queue->end;
  if (queue_front != queue_end)
  {
    LkJob job = queue->queue[queue_front];
    int queue_front_old = _InterlockedCompareExchange(&queue->start, (queue_front + 1) % LEXKIT_MAX_JOBS, queue_front);
    if (queue_front_old == queue_front)
    {
      job.func(worker_id, job.data);
      InterlockedDecrement(&queue->complete);
      return true;
    }
  }
  return false;
}

static int WorkerLoop(void* data)
{
  LkWorkerContext* wctx = data;
  LkJobQueue* queue = wctx->queue;
  while (true)
  {
    if (queue->terminate)
      return 0;
    if (WorkerPerformJob(wctx->worker_id, queue))
      continue;
    
    bool found = false;
    for (int spin = 0; spin < 200; spin++)
    {
      YieldProcessor();
      found = (queue->start != queue->end);
      if (found)
        break;
    }
    if (!found)
      WaitForSingleObject(queue->semaphore, INFINITE);
  }
}

LkJobQueue* lkJobQueueCreate(LkAllocator* alloc, int num_workers)
{
  LkJobQueue* queue = AAlloc(alloc, LkJobQueue);
  queue->alloc = alloc;
  queue->start = 0;
  queue->end = 0;
  queue->terminate = 0;
  queue->complete = 0;
  queue->semaphore = CreateSemaphoreA(NULL, 0, LEXKIT_MAX_JOBS + LEXKIT_MAX_WORKERS, NULL);

  if (num_workers < 0)
    num_workers = 0;
  if (num_workers > LEXKIT_MAX_WORKERS)
    num_workers = LEXKIT_MAX_WORKERS;
  queue->num_workers = num_workers;

  for (int i = 0; i < num_workers; i++)
  {
    queue->worker_ctx[i].worker_id = i;
    queue->worker_ctx[i].queue = queue;
    thrd_create(&queue->workers[i], WorkerLoop, &queue->worker_ctx[i]);
  }
  return queue;
}

void lkJobQueuePush(LkJobQueue* queue, LkJob job)
{
  int queue_end_next = (queue->end + 1) % LEXKIT_MAX_JOBS;
  if (queue_end_next == queue->start)
  {
    job.func(-1, job.data);
    return;
  }
  queue->queue[queue->end] = job;
  InterlockedExchange(&queue->end, queue_end_next);
  InterlockedIncrement(&queue->complete);
  ReleaseSemaphore(queue->semaphore, 1, NULL);
}

void lkJobQueueWait(LkJobQueue* queue)
{
  while (queue->complete)
  {
    if (WorkerPerformJob(-1, queue))
      continue;

    YieldProcessor();
  }
}

void lkJobQueueDestroy(LkJobQueue* queue)
{
  queue->terminate = 1;
  ReleaseSemaphore(queue->semaphore, queue->num_workers, NULL);
  for (int i = 0; i < queue->num_workers; i++)
  {
    int res = 0;
    thrd_join(queue->workers[i], &res);
  }
  CloseHandle(queue->semaphore);
  AFree(queue->alloc, queue, LkJobQueue, 1);
}