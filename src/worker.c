#include "../inc/worker.h"
#include <stdlib.h>

// Free function for tasks.
void taskFree(void *args) {
  threadPoolTask_t *task = (threadPoolTask_t *) args;
  if (task == NULL || task->argsFree == NULL) {
    return;
  }

  (task->argsFree)(task->args);
  free(task);
}

