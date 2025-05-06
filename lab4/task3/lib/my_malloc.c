#include "my_malloc.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

//place for definitions

#define HEAP_SIZE (1<<10)
#define ALIGNMENT (alignof(max_align_t))
#define ALIGN_UP(x) (((x) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

//place for structs

enum mem_block_status {
  BUSY,
  FREE,
};

typedef struct start_mem_block {
  size_t size;
  char is_free;
  struct start_mem_block* next;
} smb;

typedef struct end_mem_block {
  size_t size;
  char is_free;
} emb;

//place for statics

static void* heap = NULL;
static smb* free_list = NULL;

static emb* get_footer(smb* header) {
  return (emb*)((char*)header + header->size - sizeof(emb));
}

static void heap_init(void) {
  heap = mmap(NULL, HEAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (heap == MAP_FAILED) {
    printf("error mmap\n");
    exit(1);
  }
  
  smb *h = heap;
  h->size = HEAP_SIZE;
  h->is_free = 1;
  h->next = NULL;

  emb *f = get_footer(h);
  f->size = h->size;
  f->is_free = 1;
  free_list = h;
}

static void remove_block(smb* ptr) {
  smb** pp = &free_list;
  while (*pp) {
    if (*pp == ptr) {
      *pp = ptr->next;
      return;
    }
    pp = &(*pp)->next;
  }
}

static void add_block(smb* ptr) {
  ptr->next = free_list;
  free_list = ptr;
}

static smb* get_next(smb* ptr) {
  char* end = (char*)ptr + ptr->size;
  return (end < (char*)heap + HEAP_SIZE) ? (smb*)end : NULL;
}

static smb* get_prev(smb* ptr) {
  if (ptr == heap) {
    return NULL;
  }
  emb* prev_footer = (emb*)((char*)ptr - sizeof(emb));
  size_t prev_size = prev_footer->size;
  smb* prev = (smb*)((char*)ptr - prev_size);
  return prev;
}

static smb* merge(smb* ptr) {
  smb* next = get_next(ptr);
  if (next && (next->is_free == FREE)) {
    remove_block(next);
    ptr->size += next->size;
    emb* cur_footer = get_footer(ptr);
    cur_footer->size = ptr->size;
    cur_footer->is_free = FREE;
  }

  smb* prev = get_prev(ptr);
  if (prev && (prev->is_free == FREE)) {
    remove_block(prev);
    prev->size += ptr->size;
    emb* prev_footer = get_footer(prev);
    prev_footer->size = prev->size;
    prev_footer->is_free = FREE;
    ptr = prev;
  }

  return ptr;
}

//place for interface

void* malloc(size_t size) {
  if (size == 0) return NULL;
  if (!heap) heap_init();

  size_t final_size = ALIGN_UP(size) + sizeof(smb) + sizeof(emb);
  smb* prev = NULL;
  smb* cur_block = free_list;
  while (cur_block) {
    if (cur_block->size >= final_size) {
      break;
    }
    prev = cur_block;
    cur_block = cur_block->next;
  }
  if (cur_block == NULL) {
    printf("malo pamyati, sorry\n");
    return NULL;
  }

  remove_block(cur_block);
  if (cur_block->size >= final_size + sizeof(smb) + sizeof(emb) + ALIGNMENT) {
    size_t remain_part = cur_block->size - final_size;
    cur_block->size = final_size;
    emb* footer = get_footer(cur_block);
    footer->size = final_size;
    footer->is_free = BUSY;
    
    smb* new_block = (smb*)((char*)cur_block + final_size);
    new_block->size = remain_part;
    new_block->is_free = FREE;
    emb* new_footer = get_footer(new_block);
    new_footer->size = remain_part;
    new_footer->is_free = FREE;
    add_block(new_block);
  } else {
    emb* footer = get_footer(cur_block);
    footer->is_free = BUSY;
  }
  cur_block->is_free = BUSY;

  return (void*)((char*)cur_block + sizeof(smb));
}

void free(void* ptr) {
  if (!ptr) {
    return;
  }

  smb* cur_block = (smb*)((char*)ptr - sizeof(smb));
  cur_block->is_free = FREE;
  emb* cur_footer = get_footer(cur_block);
  cur_footer->is_free = FREE;
  cur_block = merge(cur_block);
  add_block(cur_block); 
}