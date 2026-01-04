#include "../inc/ringBuffer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void initTests(size_t cap) {
  for (size_t i = 2; i <= cap; i++) {
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

int main() {
  initTests(100UL);

  return 0;
}
