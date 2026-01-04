#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

typedef void *elem_t;
typedef void (*elemFree_func)(elem_t);

typedef struct {
  elem_t         *buffer;
  size_t          cap;
  size_t          head, tail, size;
  pthread_mutex_t lock;

  elemFree_func   elemFree;
} ringBuffer_t;

ringBuffer_t *ringBufferInit(size_t cap, elemFree_func elemFree);
void ringBufferFree(ringBuffer_t *rb);
void ringBufferInsert(ringBuffer_t *rb, elem_t elem);
elem_t ringBufferPoll(ringBuffer_t *rb);
bool ringBufferEmpty(ringBuffer_t *rb);
bool ringBufferFull(ringBuffer_t *rb);

#endif
