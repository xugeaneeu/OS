#ifndef LAB4_TASK_3_MY_MALLOC_H
#define LAB4_TASK_3_MY_MALLOC_H

#include <stddef.h>

int init_heap(void);
void destroy_heap(void);
void* my_malloc(size_t size);
void my_free(void* ptr);

#endif