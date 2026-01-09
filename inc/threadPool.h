#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include "worker.h"
#include "ringBuffer.h"

typedef enum {
  RUNNING,
  DESTROYING
} threadPoolState_t;

typedef struct {
  size_t            numThreads;
  pthread_mutex_t   threadPoolLock;
  pthread_cond_t    hasWork;
  threadWorker_t   *workers;
  ringBuffer_t     *ringBuffer;
  threadPoolState_t state;
} threadPool_t;

typedef struct {
  threadWorker_t   *worker;
  threadPoolTask_t *task;
  threadPool_t     *tp;
} threadPoolWorkerLoop_t;

threadPool_t *threadPoolInit(size_t numThreads);
void          threadPoolAddTask(threadPool_t *tp, task_func func, void *args, argsFree_func argsFree,
                                taskCompleteCB_func onComplete, void *resultObj);
void          threadPoolFree(threadPool_t *tp);

#endif
