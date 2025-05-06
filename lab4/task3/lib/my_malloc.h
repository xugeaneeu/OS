#ifndef LAB4_TASK_3_MY_MALLOC_H
#define LAB4_TASK_3_MY_MALLOC_H

#include <stddef.h>

void* my_malloc(size_t size);
void my_free(void* ptr);
void destroy_heap(void);

#endif