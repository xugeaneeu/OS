#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>


/*------------types-------------*/
typedef struct cache_entry_t {
  char*  key;
  char*  value;
  size_t size;     // already downloaded to cache
  size_t capacity; // complete size

  atomic_int      complete; // 0 - downloading, 1 - complete
  pthread_mutex_t lock;
  pthread_cond_t  cond;

  struct cache_entry_t* prev;
  struct cache_entry_t* next;

  struct cache_entry_t* hnext;
} cache_entry_t;


typedef struct LRU_Cache_t {
  size_t        capacity;
  atomic_size_t size;

  cache_entry_t* head;
  cache_entry_t* tail;

  size_t          buckets;
  cache_entry_t** table;

  pthread_mutex_t lock;
} LRU_Cache_t;


/*-------------API--------------*/

int            CreateCache(LRU_Cache_t* cache, size_t capacity, size_t buckets);
int            DestroyCache(LRU_Cache_t* cache);
cache_entry_t* CacheGetOrCreate(LRU_Cache_t* cache, const char* key,
                                int* created);
int            CacheAppend(cache_entry_t* entry, const char* buf, size_t len);
int            CacheFinish(LRU_Cache_t* cache, cache_entry_t* entry);
