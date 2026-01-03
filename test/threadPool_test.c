#include "../threadPool.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define MAX_THREADS (20UL)

typedef struct {
  int num1;
  int num2;
} dummy_t;

static void *dummyTask(void *args) {
  dummy_t *dummyArgs = (dummy_t *)args;

  return (void *)(intptr_t)(dummyArgs->num1 + dummyArgs->num2);
}

/*
* Test that initialization and freeing works on arbitrary number of threads.
*/
static void initTest(size_t maxThreads) {
  printf("Running initialization and freeing test...\n\n");
  assert(maxThreads > 0);

  threadPool_t **tpArray = calloc(maxThreads, sizeof(threadPool_t *));

  for (size_t i = 0; i < maxThreads; i++) {
    printf("Initializing thread pool with %zu threads...\n", i);
    tpArray[i] = threadPoolInit(i + 1);
 
    // Ensure that thread pool is in running state.
    assert(tpArray[i]->state == RUNNING);

    printf("Freeing thread pool with %zu threads...\n\n", i);
    threadPoolFree(tpArray[i]);
  }

  free(tpArray);
}

/*
* Test that tasks are done successfully.
*/
static void taskTest(size_t numTasks) {
  printf("Running task testing...\n\n");

  threadPool_t *tp = threadPoolInit(MAX_THREADS);

  // Try just dummy tasks.
  for (size_t i = 0; i < numTasks; i++) {
    dummy_t *args = malloc(sizeof(dummy_t));
    assert(args != NULL);

    args->num1 = i;
    args->num2 = i + 1;
    threadPoolAddTask(tp, dummyTask, (void *)args, free);
  }

  threadPoolFree(tp);
}

static void runTests() {
  initTest(1UL);
  taskTest(2UL);

  printf("All tests passed!");
  exit(EXIT_SUCCESS);
}

int main() {
  runTests(); 

  exit(EXIT_SUCCESS);
}
