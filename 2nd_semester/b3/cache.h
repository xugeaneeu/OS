#pragma once

#include <pthread.h>
#include <stddef.h>


/*------------types-------------*/
typedef struct cache_entry_t {
  char*  key;
  char*  value;
  size_t size;

  struct cache_entry_t* prev;
  struct cache_entry_t* next;
  struct cache_entry_t* hnext;
} cache_entry_t;


typedef struct LRU_Cache_t {
  size_t capacity;
  size_t size;

  cache_entry_t* head;
  cache_entry_t* tail;

  size_t buckets;
  cache_entry_t **table;

  pthread_mutex_t lock;
} LRU_Cache_t;


/*-------------API--------------*/

int            Cache(LRU_Cache_t* cache, size_t capacity, size_t buckets);
int            DestroyCache(LRU_Cache_t* cache);
int            AddElem(LRU_Cache_t* cache, const char* key, const char* value, const size_t size);
cache_entry_t* FindElem(LRU_Cache_t* cache, const char* key);