#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

typedef pthread_t threadWorker_t;

typedef void (*argsFree_func)(void *);
typedef void *(*task_func)(void *);
typedef void (*taskCompleteCB_func)(void *result, void *resultObj);

typedef struct {
  task_func           func;
  void               *args;
  argsFree_func       argsFree;
  taskCompleteCB_func onComplete; 
  void               *resultObj;
} threadPoolTask_t;

void taskFree(void *task);
#endif
