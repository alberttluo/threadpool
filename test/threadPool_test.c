#include "../threadPool.h"
#include <stdio.h>
#include <assert.h>

#define MAX_THREADS (100UL)

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
}

static void runTests() {
  initTest(MAX_THREADS);

  printf("All tests passed!");
  exit(EXIT_SUCCESS);
}

int main() {
  runTests(); 

  exit(EXIT_SUCCESS);
}
