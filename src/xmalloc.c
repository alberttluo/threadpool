#include "../inc/xmalloc.h"
#include "stdio.h"

/*
* Safe malloc/calloc.
*/
void *xmalloc(size_t size) {
  void *ptr = malloc(size);

  if (ptr == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  return ptr;
}

void *xcalloc(size_t count, size_t size) {
  void *ptr = calloc(count, size);

  if (ptr == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  return ptr;
}
