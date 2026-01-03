#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

typedef pthread_t threadWorker_t;

typedef void (*argsFree_func)(void *);
typedef void *(*task_func)(void *);

typedef struct {
  task_func     func;
  void         *args;
  argsFree_func argsFree;
} threadPoolTask_t;

typedef struct queueNode_struct queueNode_t;

typedef struct queueNode_struct {
  threadPoolTask_t *task;
  queueNode_t      *next; 
} queueNode_t;

typedef struct {
  queueNode_t *head;
  queueNode_t *tail;
} taskQueue_t;

typedef enum {
  RUNNING,
  DESTROYING
} threadPoolState_t;

typedef struct {
  size_t            numThreads;
  pthread_mutex_t   threadPoolLock;
  pthread_cond_t    hasWork;
  threadWorker_t   *workers;
  taskQueue_t      *taskQueue;
  threadPoolState_t state;
} threadPool_t;

typedef struct {
  threadWorker_t   *worker;
  threadPoolTask_t *task;
  threadPool_t     *tp;
} threadPoolWorkerLoop_t;

threadPool_t *threadPoolInit(size_t numThreads);
void          threadPoolAddTask(threadPool_t *tp, task_func func, void *args, argsFree_func argsFree);
void          threadPoolFree(threadPool_t *tp);

#endif
