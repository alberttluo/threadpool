#include "../threadPool.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_THREADS (20UL)
#define MAX_TASKS (10000UL)

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
  int num1;
  int num2;
  int *num3;
} dummy_t;

typedef struct {
  size_t remaining;
  pthread_mutex_t lock;
  pthread_cond_t done;
} doneContext_t;

static void dummyFree(void *ptr) {
  dummy_t *d = (dummy_t *) ptr;
  free(d->num3);

  free(d);
}

static void dummyCB(void *result, void *resultObj) {
  struct {size_t *slot; doneContext_t *ctx;} *d = resultObj;
  size_t *r = (size_t *)result;

  *(d->slot) = *r;

  pthread_mutex_lock(&d->ctx->lock);
  if (--d->ctx->remaining == 0) pthread_cond_signal(&d->ctx->done);
  pthread_mutex_unlock(&d->ctx->lock);

  free(d);
  free(r);
}

static void *dummyTask(void *args) {
  dummy_t *dummyArgs = (dummy_t *)args;
  size_t *result = malloc(sizeof(size_t));
  *result = dummyArgs->num1 + dummyArgs->num2 * (*dummyArgs->num3);
  return (void *)result;
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
* Test that dummy tasks are done successfully.
*/
static void dummyTaskTest(size_t numTasks) {
  printf("Running task testing...\n\n");

  threadPool_t *tp = threadPoolInit(MAX_THREADS);
  assert(tp != NULL);

  size_t *array = calloc(numTasks, sizeof(size_t));
  assert(array != NULL);

  doneContext_t ctx = {
    .remaining = numTasks
  };
  pthread_mutex_init(&ctx.lock, NULL);
  pthread_cond_init(&ctx.done, NULL);

  // Try just dummy tasks.
  for (size_t i = 0; i < numTasks; i++) {
    dummy_t *args = malloc(sizeof(dummy_t));
    assert(args != NULL);

    args->num1 = i;
    args->num2 = i + 1;
    args->num3 = malloc(sizeof(int));
    *args->num3 = i + 2; 

    struct {size_t *slot; doneContext_t *d;} *cbArg = malloc(sizeof(*cbArg));
    cbArg->slot = &array[i];
    cbArg->d = &ctx;
    threadPoolAddTask(tp, dummyTask, (void *)args, dummyFree, dummyCB, cbArg);
  }

  pthread_mutex_lock(&ctx.lock);
  while (ctx.remaining != 0) {
    pthread_cond_wait(&ctx.done, &ctx.lock);
  }
  pthread_mutex_unlock(&ctx.lock);

  for (size_t i = 0; i < numTasks; i++) {
    printf("array[%zu] = %zu\n", i, array[i]);
    assert(array[i] == (i) + (i + 1) * (i + 2));
  }

  threadPoolFree(tp);
  free(array);
}

static void runTests(size_t threads, size_t tasks) {
  initTest(MIN(threads, MAX_THREADS));
  dummyTaskTest(MIN(tasks, MAX_TASKS));

  printf("All tests passed!");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: ./test <num_threads> <num_tasks>\n");
    exit(EXIT_FAILURE);
  }

  char *endptr;

  size_t threads = strtoul(argv[1], &endptr, 10);
  size_t tasks = strtoul(argv[2], &endptr, 10);

  runTests(threads, tasks); 

  exit(EXIT_SUCCESS);
}
