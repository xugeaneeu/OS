#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>


#define ASC 0
#define DESC 1
#define EQ 2
#define SWAP 3

#define MAX_STRING_LENGTH 100

typedef struct Node {
  struct Node *next;
  int counter_swap, counter_asc, counter_dsc, counter_eq;
  char value[MAX_STRING_LENGTH];
  pthread_mutex_t sync;
} Node;

typedef struct _Storage {
  Node *first;
  int capacity;
  pthread_mutex_t sync;
} Storage;

typedef struct _ThreadData {
  Storage *storage;
  int type; //comparisson type
} ThreadData;

Storage *init_storage(int capacity);
void add_node(Storage *storage, const char *value);
void fill_storage(Storage *storage);