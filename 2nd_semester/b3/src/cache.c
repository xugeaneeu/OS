#include "cache.h"

#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*------------place for statics------------*/

/*
  Simple hash to str function.
*/
static unsigned long hash_str(const char* str) {
  unsigned long hash = 5381;

  int c;
  while ((c = (unsigned char)*str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}


/*
  Delete node from cache double-linked list.
*/
static void detach_node(LRU_Cache_t* cache, cache_entry_t* node) {
  if (!node)
    return;

  if (node->prev) {
    node->prev->next = node->next;
  } else {
    cache->head = node->next;
  }

  if (node->next) {
    node->next->prev = node->prev;
  } else {
    cache->tail = node->prev;
  }

  node->prev = node->next = NULL;
}


/*
  Push node to cahce double-linked list.
*/
static void insert_front(LRU_Cache_t* cache, cache_entry_t* node) {
  node->prev = NULL;
  node->next = cache->head;
  if (cache->head)
    cache->head->prev = node;
  cache->head = node;
  if (!cache->tail)
    cache->tail = node;
}


/*
  Pop node (complete = 1) from tail.
*/
static void evict_tail(LRU_Cache_t* c) {
  // удаление из таблицы и LRU
  pthread_mutex_lock(&c->lock);
  cache_entry_t* e = c->tail;
  while (e && !atomic_load(&e->complete))
    e = e->prev;
  if (!e) {
    pthread_mutex_unlock(&c->lock);
    return;
  }

  unsigned long  h = hash_str(e->key) % c->buckets;
  cache_entry_t* cur = c->table[h];
  cache_entry_t* prev = NULL;
  while (cur) {
    if (cur == e) {
      if (prev)
        prev->hnext = cur->hnext;
      else
        c->table[h] = cur->hnext;
      break;
    }
    prev = cur;
    cur = cur->hnext;
  }

  detach_node(c, e);
  pthread_mutex_unlock(&c->lock);

  atomic_fetch_sub(&c->size, e->size);

  pthread_mutex_destroy(&e->lock);
  pthread_cond_destroy(&e->cond);
  free(e->value);
  free(e->key);
  free(e);
}



/*-------------------API-------------------*/

/*
  Initialize cache storage with according capasity.
  Return 0 in success, 1 if cache is NULL / capasity is 0 /
  memory allocation error occured.
*/
int CreateCache(LRU_Cache_t* cache, size_t capacity, size_t buckets) {
  if (!cache || buckets == 0) {
    return EXIT_FAILURE;
  }

  cache->capacity = capacity;
  atomic_store(&cache->size, 0);

  cache->head = cache->tail = NULL;

  cache->buckets = buckets;
  cache->table = calloc(buckets, sizeof(cache_entry_t*));
  if (!cache->table) {
    return EXIT_FAILURE;
  }

  if (pthread_mutex_init(&cache->lock, NULL) != 0) {
    free(cache->table);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}


/*
  Destroying cache.
  Return 0 in success, 1 if cache is NULL.
*/
int DestroyCache(LRU_Cache_t* cache) {
  pthread_mutex_lock(&cache->lock);
  cache_entry_t* cur = cache->head;
  while (cur) {
    cache_entry_t* n = cur->next;
    pthread_mutex_destroy(&cur->lock);
    pthread_cond_destroy(&cur->cond);
    free(cur->key);
    free(cur->value);
    free(cur);
    cur = n;
  }
  free(cache->table);

  cache->table = NULL;
  cache->head = cache->tail = NULL;

  pthread_mutex_unlock(&cache->lock);
  pthread_mutex_destroy(&cache->lock);

  return EXIT_SUCCESS;
}


/*
  Search in cache by key.
  Return cache_entry_t* and created flag (0 if found in cache, 1 otherwise),
  NULL if error occured.
*/
cache_entry_t* CacheGetOrCreate(LRU_Cache_t* cache, const char* key,
                                int* created) {
  unsigned long h = hash_str(key) % cache->buckets;

  pthread_mutex_lock(&cache->lock);

  // search in hash table
  cache_entry_t* cur = cache->table[h];
  while (cur) {
    if (strcmp(cur->key, key) == 0) {
      if (atomic_load(&cur->complete)) {
        detach_node(cache, cur);
        insert_front(cache, cur);
      }
      *created = 0;
      pthread_mutex_unlock(&cache->lock);
      return cur;
    }
    cur = cur->hnext;
  }

  // not found, create new
  cache_entry_t* e = malloc(sizeof(cache_entry_t));
  if (!e) {
    pthread_mutex_unlock(&cache->lock);
    return NULL;
  }

  e->key = strdup(key);
  e->value = NULL;
  e->size = e->capacity = 0;
  atomic_store(&e->complete, 0);
  pthread_mutex_init(&e->lock, NULL);
  pthread_cond_init(&e->cond, NULL);
  e->hnext = cache->table[h];
  cache->table[h] = e;
  insert_front(cache, e);

  *created = 1;
  pthread_mutex_unlock(&cache->lock);
  return e;
}


/*
  Append data from buf to entry value, realloc value if needed.
  Return 0 in success, 1 if entry/buf/len invalid or realloc was not success.
*/
int CacheAppend(cache_entry_t* entry, const char* buf, size_t len) {
  if (!entry || !buf || len == 0)
    return EXIT_FAILURE;

  pthread_mutex_lock(&entry->lock);
  /* realloc if needed */
  if (entry->size + len > entry->capacity) {
    size_t nc = entry->capacity == 0 ? len * 2 : entry->capacity * 2;
    while (nc < entry->size + len)
      nc *= 2;
    char* p = realloc(entry->value, nc);
    if (!p) {
      pthread_mutex_unlock(&entry->lock);
      return EXIT_FAILURE;
    }
    entry->value = p;
    entry->capacity = nc;
  }

  memcpy(entry->value + entry->size, buf, len);
  entry->size += len;

  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->lock);

  return EXIT_SUCCESS;
}


/*
  Mark entry complete and start eviction mechanism.
  Return -1 if cache or entry invalid, else 0.
*/
int CacheFinish(LRU_Cache_t* cache, cache_entry_t* entry) {
  if (!cache || !entry)
    return -1;

  pthread_mutex_lock(&entry->lock);
  atomic_store(&entry->complete, 1);
  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->lock);

  atomic_fetch_add(&cache->size, entry->size);
  while (atomic_load(&cache->size) > cache->capacity) {
    if (cache->tail == cache->head)
      break;
    evict_tail(cache);
  }
  return 0;
}
