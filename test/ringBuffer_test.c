#include "../inc/ringBuffer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_CAP (10000UL)

typedef struct {
  ringBuffer_t *rb;
  elem_t        elem;
} rbArg_t;

static void *prodInsert(void *args) {
  rbArg_t *ra = (rbArg_t *) args;

  if (!ringBufferFull(ra->rb)) {
    ringBufferInsert(ra->rb, ra->elem);
  }

  free(ra);
  return NULL;
}

static void initTests(size_t cap) {
  printf("Running initialization testing...\n");
  for (size_t i = 2; i <= cap; i++) {
    printf("Initializing ring buffer with capacity %zu.\n", i);
    ringBuffer_t *rb = ringBufferInit(i, free);
    assert(ringBufferEmpty(rb));

    for (size_t j = 0; j < i; j++) {
      int *elem = malloc(sizeof(int));
      *elem = 1;
      ringBufferInsert(rb, (void *)elem);
      assert(!ringBufferEmpty(rb));
    }

    assert(ringBufferFull(rb));

    ringBufferFree(rb);
  }

  printf("All initialization tests passed!\n");
}

static void insertTests(size_t numProds) {
  ringBuffer_t *rb = ringBufferInit(MAX_CAP, free);

  // Insert with many producers.
  pthread_t *prods = calloc(numProds, sizeof(pthread_t));

  for (size_t i = 0; i < numProds; i++) {
    rbArg_t *args = malloc(sizeof(rbArg_t));
    args->rb = rb;
    args->elem = malloc(sizeof(int));
    *(int *)args->elem = i;

    pthread_create(&prods[i], NULL, prodInsert, args);
  }

  for (size_t i = 0; i < numProds; i++) {
    pthread_join(prods[i], NULL);
  }
  
  bool *seen = calloc(numProds, sizeof(bool));

  // Print values.
  size_t j = 0;
  while (!ringBufferEmpty(rb)) {
    int *p = (int *) ringBufferPoll(rb);
    int polled = *p;
    printf("buffer[%zu] = %d\n", j++, polled);

    assert(!seen[polled]);
    seen[polled] = true;
    free(p);
  }

  // Ensure all values seen.
  for (size_t i = 0; i < numProds; i++) {
    assert(seen[i]);
  }

  free(seen);
  free(prods);
  ringBufferFree(rb);

  printf("All insert tests passed!\n");
}

int main() {
  initTests(10);

  for (size_t i = 1; i < 10UL; i++) {
    insertTests(i);
  }

  printf("\n\nAll tests passed!\n");
  return 0;
}
