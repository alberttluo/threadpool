#include "threadPool.h"
#include <stdio.h>
#include <assert.h>

#define DEBUG (1)

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
 #define DEBUG_ASSERT(cond) assert(cond)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

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
static threadPoolTask_t *taskQueuePoll(taskQueue_t *taskQueue);
static void taskQueueInsert(taskQueue_t *taskQueue, threadPoolTask_t *task);
static bool taskQueueEmpty(taskQueue_t *taskQueue);

// Free function for tasks.
static void taskFree(threadPoolTask_t *task) {
  if (task == NULL || task->argsFree == NULL) {
    return;
  }

  (task->argsFree)(task->args);
  free(task);
}

/*
* Thread pool worker loop.
*/
static void *threadPoolWorkerLoop(void *args) {
  threadPool_t *tp = (threadPool_t *)args;

  while (true) {
    pthread_mutex_lock(&tp->threadPoolLock);

    // Wait until there is work or we are freeing.
    while (tp->state == RUNNING && taskQueueEmpty(tp->taskQueue)) {
      pthread_cond_wait(&tp->hasWork, &tp->threadPoolLock);
    }

    // If we are freeing and there is no work left, just exit.
    if (tp->state == DESTROYING && taskQueueEmpty(tp->taskQueue)) {
      pthread_mutex_unlock(&tp->threadPoolLock);
      break;
    }

    // Otherwise, grab a task and complete it.
    threadPoolTask_t *task = taskQueuePoll(tp->taskQueue);
    pthread_mutex_unlock(&tp->threadPoolLock);

    if (task != NULL) {
      (task->func)(task->args);
      taskFree(task);
    }
  }

  return NULL;
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

  while (!taskQueueEmpty(taskQueue)) {
    threadPoolTask_t *task = taskQueuePoll(taskQueue);
    taskFree(task);
  }

  free(taskQueue);
}

static threadPoolTask_t *taskQueuePoll(taskQueue_t *taskQueue) {
  if (taskQueue->head == NULL) {
    return NULL;
  }

  queueNode_t *front = taskQueue->head;
  threadPoolTask_t *task = front->task;

  taskQueue->head = front->next;

  if (taskQueue->head == NULL) {
    taskQueue->tail = NULL;
  }

  free(front);
  return task;
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
  DEBUG_PRINT("threadPoolInit called.\n");

  if (numThreads == 0) {
    fprintf(stderr, "Number of threads must be positive.\n");
    return NULL;
  }

  threadPool_t *tp = xcalloc(1, sizeof(threadPool_t));
  tp->numThreads = numThreads;
  tp->taskQueue = taskQueueInit();
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

void threadPoolAddTask(threadPool_t *tp, task_func func, void *args, argsFree_func argsFree) {
  if (tp == NULL || func == NULL) {
    return;
  }

  threadPoolTask_t *task = xmalloc(sizeof(threadPoolTask_t));
  task->func = func;
  task->args = args;
  task->argsFree = argsFree;

  pthread_mutex_lock(&tp->threadPoolLock);
  if (tp->state == DESTROYING) {
    pthread_mutex_unlock(&tp->threadPoolLock);
    taskFree(task);
    return;
  }

  taskQueueInsert(tp->taskQueue, task);

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
  taskQueueFree(tp->taskQueue);
  pthread_mutex_destroy(&tp->threadPoolLock);
  pthread_cond_destroy(&tp->hasWork);

  free(tp);
}
