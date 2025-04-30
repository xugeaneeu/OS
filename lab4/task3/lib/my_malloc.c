#include "my_malloc.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define FREE 1
#define BUSY 0

typedef struct heap_block {
  size_t size;
  char status;
} heap_block;

void* heap_start = NULL;
size_t memory_size = 0;


int init_heap(void) {
  memory_size = getpagesize();
  heap_start = mmap(NULL, memory_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (heap_start == MAP_FAILED) {
    printf("can't mirror memory\n");
    return -1;
  }

  heap_block* block = (heap_block*)heap_start;
  block->status = FREE;
  block->size = memory_size - sizeof(heap_block);

  return EXIT_SUCCESS;
}

void destroy_heap(void) {
  munmap(heap_start, memory_size);
}

void* my_malloc(size_t size) {

}

void my_free(void *ptr) {
  if (NULL == ptr) {
    return;
  }

  heap_block* block = (heap_block*) ((char*)ptr -sizeof(heap_block));
  block->status = FREE;
}