#include "../inc/threadPool.h"
#include "../inc/xmalloc.h"
#include "../inc/config.h"
#include <stdio.h>

/*
* Thread pool worker loop.
*/
static void *threadPoolWorkerLoop(void *args) {
  DEBUG_ASSERT(args != NULL);
  threadPool_t *tp = (threadPool_t *)args;

  while (true) {
    pthread_mutex_lock(&tp->threadPoolLock);

    // Wait until there is work or we are freeing.
    while (tp->state == RUNNING && ringBufferEmpty(tp->ringBuffer)) {
      pthread_cond_wait(&tp->hasWork, &tp->threadPoolLock);
    }

    // If we are freeing and there is no work left, just exit.
    if (tp->state == DESTROYING && ringBufferEmpty(tp->ringBuffer)) {
      pthread_mutex_unlock(&tp->threadPoolLock);
      break;
    }

    // Otherwise, grab a task and complete it.
    threadPoolTask_t *task = ringBufferPoll(tp->ringBuffer);
    pthread_mutex_unlock(&tp->threadPoolLock);

    if (task != NULL) {
      void *result = (task->func)(task->args);
      void *resultObj = task->resultObj;
      taskCompleteCB_func onComplete = task->onComplete;

      taskFree(task);

      if (onComplete != NULL) {
        (onComplete)(result, resultObj);
      }
    }
  }

  return NULL;
}

/*
* Thread pool functions.
*/

threadPool_t *threadPoolInit(size_t numThreads) {
  DEBUG_PRINT("threadPoolInit called.\n");

  if (numThreads == 0) {
    fprintf(stderr, "Number of threads must be positive.\n");
    return NULL;
  }

  threadPool_t *tp = xcalloc(1, sizeof(threadPool_t));
  tp->numThreads = numThreads;
  tp->ringBuffer = ringBufferInit(100, taskFree);
  tp->state = RUNNING;
  pthread_mutex_init(&tp->threadPoolLock, NULL);
  pthread_cond_init(&tp->hasWork, NULL);

  threadWorker_t *workers = xcalloc(numThreads, sizeof(threadWorker_t));

  for (size_t i = 0; i < numThreads; i++) {
    DEBUG_PRINT("Creating thread %zu.\n", i);
    int rc = pthread_create(&workers[i], NULL, threadPoolWorkerLoop, (void *) tp);
    if (rc != 0) {
      fprintf(stderr, "pthread_create failed (%d)\n", rc);
      exit(EXIT_FAILURE);
    }
  }

  tp->workers = workers;

  return tp;
}

void threadPoolAddTask(threadPool_t *tp, task_func func, void *args, argsFree_func argsFree,
                       taskCompleteCB_func onComplete, void *resultObj) 
{
  if (tp == NULL || func == NULL) {
    return;
  }

  threadPoolTask_t *task = xmalloc(sizeof(threadPoolTask_t));
  task->func = func;
  task->args = args;
  task->argsFree = argsFree;
  task->onComplete = onComplete;
  task->resultObj = resultObj;

  pthread_mutex_lock(&tp->threadPoolLock);
  if (tp->state == DESTROYING) {
    pthread_mutex_unlock(&tp->threadPoolLock);
    taskFree(task);
    return;
  }

  ringBufferInsert(tp->ringBuffer, task);

  // Just wake one worker to complete the task.
  pthread_cond_signal(&tp->hasWork);
  pthread_mutex_unlock(&tp->threadPoolLock);
}

void threadPoolFree(threadPool_t *tp) {
  DEBUG_PRINT("threadPoolFree called.\n");

  if (tp == NULL) {
    return;
  }

  pthread_mutex_lock(&tp->threadPoolLock);
  tp->state = DESTROYING;
  DEBUG_PRINT("Waking all workers.\n");

  // Wake all workers so they can exit.
  pthread_cond_broadcast(&tp->hasWork);
  pthread_mutex_unlock(&tp->threadPoolLock);

  for (size_t i = 0; i < tp->numThreads; i++) {
    DEBUG_PRINT("Joining thread %zu.\n", i);
    pthread_join(tp->workers[i], NULL);
  }

  free(tp->workers);

  DEBUG_PRINT("Freeing the task queue.\n");
  ringBufferFree(tp->ringBuffer);
  pthread_mutex_destroy(&tp->threadPoolLock);
  pthread_cond_destroy(&tp->hasWork);

  free(tp);
}
