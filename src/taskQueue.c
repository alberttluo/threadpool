#include "../inc/taskQueue.h"
#include "../inc/xmalloc.h"
#include "../inc/config.h"

taskQueue_t *taskQueueInit() {
  taskQueue_t *tq = xmalloc(sizeof(taskQueue_t));
  tq->head = NULL;
  tq->tail = NULL;

  return tq;
}

void taskQueueFree(taskQueue_t *taskQueue) {
  if (taskQueue == NULL) {
    return;
  }

  while (!taskQueueEmpty(taskQueue)) {
    threadPoolTask_t *task = taskQueuePoll(taskQueue);
    taskFree(task);
  }

  free(taskQueue);
}

threadPoolTask_t *taskQueuePoll(taskQueue_t *taskQueue) {
  DEBUG_ASSERT(taskQueue != NULL);
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

void taskQueueInsert(taskQueue_t *taskQueue, threadPoolTask_t *task) {
  DEBUG_ASSERT(taskQueue != NULL);
  DEBUG_ASSERT(task != NULL);
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

bool taskQueueEmpty(taskQueue_t *taskQueue) {
  DEBUG_ASSERT(taskQueue != NULL);
  return (taskQueue->head == NULL && taskQueue->tail == NULL);
}
