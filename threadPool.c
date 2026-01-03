#include "threadPool.h"
#include <stdio.h>

/*
* Safe malloc/calloc.
*/
static void *xmalloc(size_t size) {
  void *ptr = malloc(size);

  if (ptr == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  return ptr;
}

static void *xcalloc(size_t count, size_t size) {
  void *ptr = calloc(count, size);

  if (ptr == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  return ptr;
}


/*
* Static task queue functions.
*/
static taskQueue_t *taskQueueInit();
static void taskQueueFree(taskQueue_t *taskQueue);
static queueNode_t *taskQueuePoll(taskQueue_t *taskQueue);
static void taskQueueInsert(taskQueue_t *taskQueue, threadPoolTask_t *task);
static bool taskQueueEmpty(taskQueue_t *taskQueue);

/*
* Thread pool and thread pool worker loops.
*/
static void *threadPoolWorkerLoop(void *twlStruct);
static void *threadPoolLoop(void *tpPtr);

static void *threadPoolWorkerLoop(void *twlStruct) {
  threadPoolWorkerLoop_t *castedWT = (threadPoolWorkerLoop_t *)(twlStruct);
  threadWorker_t *worker = castedWT->worker;
  threadPoolTask_t *task = castedWT->task;
  threadPool_t *tp = castedWT->tp;

  // Wait until a task is delegated, then complete the task.
  pthread_cond_wait(&worker->taskDelegated, NULL);

  // Lock the worker to change availablilty.
  pthread_mutex_lock(&worker->threadWorkerLock);
  worker->available = false;

  // Decrement an available worker from the thread pool.
  pthread_mutex_lock(&tp->threadPoolLock);
  tp->numWorkersAvailable--;
  pthread_mutex_unlock(&tp->threadPoolLock);

  void *ret = (task->func)(task->args);

  // Reset availability and increment available workers.
  pthread_mutex_lock(&tp->threadPoolLock);
  tp->numWorkersAvailable++;
  pthread_mutex_unlock(&tp->threadPoolLock);

  worker->available = true;
  pthread_mutex_unlock(&worker->threadWorkerLock);

  return ret;
}

/*
* Thread pool loop.
*/
static void *threadPoolLoop(void *tpPtr) {
  threadPool_t *tp = (threadPool_t *)tpPtr;

  while (tp->state != DESTROYING) {
    if (taskQueueEmpty(tp->taskQueue)) {
      continue;
    }
    
    // Loop until we find an available worker.
    else {
      size_t i = 0;
      while (true) {
        // Check worker availability.
        pthread_mutex_lock(&tp->workers[i]->threadWorkerLock);
        if (tp->workers[i]->available) {
          pthread_cond_signal(&tp->workers[i]->taskDelegated);
          // HELP HERE TO PASS TASK TO WORKER.
        }
        pthread_mutex_unlock(&tp->workers[i]->threadWorkerLock);

        i = (i + 1) % tp->numThreads;
      }
    }
  }
}

static taskQueue_t *taskQueueInit() {
  taskQueue_t *tq = xmalloc(sizeof(taskQueue_t));
  tq->head = NULL;
  tq->tail = NULL;

  return tq;
}

static void taskQueueFree(taskQueue_t *taskQueue) {
  if (taskQueue == NULL) {
    return;
  }

  while (taskQueue->head != taskQueue->tail && taskQueue->head != NULL) {
    queueNode_t *nextTask = taskQueue->head->next;

    // Free the task arguments first.
    taskQueue->head->task->argsFree(taskQueue->head->task->args);
    free(taskQueue->head->task);
    free(taskQueue->head);

    taskQueue->head = nextTask;
  }

  taskQueue->tail->task->argsFree(taskQueue->tail->task->args);
  free(taskQueue->tail->task);
  free(taskQueue->tail);

  free(taskQueue);
}

static queueNode_t *taskQueuePoll(taskQueue_t *taskQueue) {
  if (taskQueue->head == NULL) {
    return NULL;
  }

  queueNode_t *front = taskQueue->head;

  if (front == taskQueue->tail) {
    taskQueue->tail = NULL;
  }

  taskQueue->head = front->next;
  front->next = NULL;
  
  return front;
}

static void taskQueueInsert(taskQueue_t *taskQueue, threadPoolTask_t *task) {
  queueNode_t *taskNode = xmalloc(sizeof(queueNode_t));
  taskNode->task = task;
  taskNode->next = NULL;

  if (taskQueueEmpty(taskQueue)) {
    taskQueue->head = taskNode;
    taskQueue->tail = taskNode;
  }

  else {
    taskQueue->tail->next = taskNode;
    taskQueue->tail = taskNode;
  }
}

static bool taskQueueEmpty(taskQueue_t *taskQueue) {
  return (taskQueue->head == NULL && taskQueue->tail == NULL);
}

/*
* Thread pool functions.
*/

threadPool_t *threadPoolInit(size_t numThreads) {
  if (numThreads <= 0) {
    fprintf(stderr, "Number of threads must be positive.\n");
    exit(EXIT_FAILURE);
  }

  threadPool_t *tp = xmalloc(sizeof(threadPool_t));

  threadWorker_t **workers = xcalloc(numThreads, sizeof(threadWorker_t));

  for (size_t i = 0; i < numThreads; i++) {
    workers[i] = xmalloc(sizeof(threadWorker_t));
    pthread_create(&workers[i]->thread, NULL, threadPoolWorkerLoop, NULL);
    pthread_mutex_init(&workers[i]->threadWorkerLock, NULL);
    pthread_cond_init(&workers[i]->taskDelegated, NULL);
    workers[i]->available = true;
  }

  tp->numThreads = numThreads;
  tp->numWorkersAvailable = numThreads;
  pthread_mutex_init(&tp->threadPoolLock, NULL);
  tp->workers = workers;
  tp->taskQueue = taskQueueInit();

  return tp;
}

void threadPoolAddTask(threadPool_t *tp, task_func func, void *args, argsFree_func argsFree) {
  if (tp == NULL || func == NULL) {
    return;
  }

  pthread_mutex_lock(&tp->threadPoolLock);
  threadPoolTask_t *task = xmalloc(sizeof(threadPoolTask_t));
  task->func = func;
  task->args = args;
  task->argsFree = argsFree;

  taskQueueInsert(tp->taskQueue, task);
  pthread_mutex_unlock(&tp->threadPoolLock);
}

size_t getNumAvailable(threadPool_t *tp) {
  pthread_mutex_lock(&tp->threadPoolLock);
  size_t numAvailable = tp->numWorkersAvailable;
  pthread_mutex_unlock(&tp->threadPoolLock);

  return numAvailable;
}

void threadPoolFree(threadPool_t *tp) {
  if (tp == NULL) {
    return;
  }

  pthread_mutex_lock(&tp->threadPoolLock);

  for (size_t i = 0; i < tp->numThreads; i++) {
    pthread_join(tp->workers[i]->thread, NULL);
    pthread_mutex_destroy(&tp->workers[i]->threadWorkerLock);
    pthread_cond_destroy(&tp->workers[i]->taskDelegated);
    free(tp->workers[i]);
  }

  free(tp->workers);
  taskQueueFree(tp->taskQueue);
  pthread_mutex_destroy(&tp->threadPoolLock);

  free(tp);
}
