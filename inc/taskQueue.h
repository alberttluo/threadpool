#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdbool.h>
#include "worker.h"

typedef struct queueNode_struct queueNode_t;

typedef struct queueNode_struct {
  threadPoolTask_t *task;
  queueNode_t      *next; 
} queueNode_t;

typedef struct {
  queueNode_t *head;
  queueNode_t *tail;
} taskQueue_t;

taskQueue_t *taskQueueInit();
void taskQueueFree(taskQueue_t *taskQueue);
threadPoolTask_t *taskQueuePoll(taskQueue_t *taskQueue);
void taskQueueInsert(taskQueue_t *taskQueue, threadPoolTask_t *task);
bool taskQueueEmpty(taskQueue_t *taskQueue);

#endif
