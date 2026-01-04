#include "../inc/ringBuffer.h"
#include "../inc/xmalloc.h"
#include "../inc/config.h"

// Computes idx % cap (faster if cap is a power of 2).
#define MOD(idx, cap) ((((cap) & ((cap) - 1UL)) == 0UL) ? ((idx) & ((cap) - 1UL)) : ((idx) % (cap)))

ringBuffer_t *ringBufferInit(size_t cap, elemFree_func elemFree) {
  // Capacity must be greater than 1.
  DEBUG_ASSERT(cap > 1UL);

  ringBuffer_t *rb = xcalloc(1, sizeof(ringBuffer_t));
  rb->buffer = xcalloc(cap, sizeof(elem_t));
  rb->cap = cap;
  rb->head = 0;
  rb->tail = 0;
  rb->size = 0;
  rb->elemFree = elemFree;
  pthread_mutex_init(&rb->lock, NULL);

  return rb;
}

void ringBufferFree(ringBuffer_t *rb) {
  if (rb == NULL) {
    return;
  }

  for (size_t i = 0; i < rb->cap; i++) {
    ((rb->elemFree == NULL) ? free : rb->elemFree)(rb->buffer[i]);
  }

  free(rb->buffer);
  pthread_mutex_destroy(&rb->lock);

  free(rb);
}

void ringBufferInsert(ringBuffer_t *rb, elem_t elem) {
  DEBUG_ASSERT(rb != NULL);
  DEBUG_ASSERT(elem != NULL);
  DEBUG_ASSERT(!ringBufferFull(rb));

  pthread_mutex_lock(&rb->lock);
  rb->buffer[rb->tail] = elem;
  rb->tail = MOD(rb->tail + 1, rb->cap);
  rb->size++;
  pthread_mutex_unlock(&rb->lock);
}

elem_t ringBufferPoll(ringBuffer_t *rb) {
  DEBUG_ASSERT(rb != NULL);
  DEBUG_ASSERT(!ringBufferEmpty(rb));

  pthread_mutex_lock(&rb->lock);
  elem_t polled = rb->buffer[rb->head];
  rb->head = MOD(rb->head + 1, rb->cap);
  rb->size--;
  pthread_mutex_unlock(&rb->lock);

  return polled;
}

bool ringBufferEmpty(ringBuffer_t *rb) {
  DEBUG_ASSERT(rb != NULL);
  
  pthread_mutex_lock(&rb->lock);
  bool empty = (rb->size == 0UL);
  pthread_mutex_unlock(&rb->lock);

  return empty;
}

bool ringBufferFull(ringBuffer_t *rb) {
  DEBUG_ASSERT(rb != NULL);

  pthread_mutex_lock(&rb->lock);
  bool full = (rb->size == rb->cap);
  pthread_mutex_unlock(&rb->lock);

  return full;
}
