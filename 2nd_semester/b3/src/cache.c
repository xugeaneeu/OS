#include "cache.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*------------place for statics------------*/

/*
  Simple str to hash function.
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
  cache_entry_t* e = NULL;

  pthread_mutex_lock(&c->list_lock);
  e = c->tail;
  while (e && !atomic_load(&e->complete))
    e = e->prev;
  if (!e) {
    pthread_mutex_unlock(&c->list_lock);
    return;
  }
  atomic_store(&e->deleted, true);
  detach_node(c, e);
  pthread_mutex_unlock(&c->list_lock);

  unsigned long h = hash_str(e->key) % c->buckets;
  pthread_mutex_lock(&c->bucket_locks[h]);
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
  pthread_mutex_unlock(&c->bucket_locks[h]);

  atomic_fetch_sub(&c->size, e->size);
  if (atomic_load(&e->refcount) == 0) {
    Destroy_entry(e);
  }
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

  cache->bucket_locks = calloc(buckets, sizeof(pthread_mutex_t));
  if (!cache->bucket_locks) {
    free(cache->table);
    return EXIT_FAILURE;
  }
  for (size_t i = 0; i < buckets; i++) {
    pthread_mutex_init(&cache->bucket_locks[i], NULL);
  }

  pthread_mutex_init(&cache->list_lock, NULL);

  return EXIT_SUCCESS;
}


/*
  Destroying cache.
  Return 0 in success, 1 if cache is NULL.
*/
int DestroyCache(LRU_Cache_t* cache) {
  if (!cache)
    return EXIT_SUCCESS;

  pthread_mutex_lock(&cache->list_lock);
  cache_entry_t* cur = cache->head;
  while (cur) {
    cache_entry_t* nxt = cur->next;
    pthread_rwlock_wrlock(&cur->rwlock);
    pthread_mutex_destroy(&cur->cond_mutex);
    pthread_cond_destroy(&cur->cond);
    pthread_rwlock_unlock(&cur->rwlock);
    pthread_rwlock_destroy(&cur->rwlock);
    free(cur->key);
    free(cur->value);
    free(cur);
    cur = nxt;
  }
  cache->head = cache->tail = NULL;
  pthread_mutex_unlock(&cache->list_lock);

  pthread_mutex_destroy(&cache->list_lock);

  free(cache->table);
  cache->table = NULL;

  for (size_t i = 0; i < cache->buckets; i++) {
    pthread_mutex_destroy(&cache->bucket_locks[i]);
  }
  free(cache->bucket_locks);
  cache->bucket_locks = NULL;

  return EXIT_SUCCESS;
}


/*
  Search in cache by key.
  Return cache_entry_t* and created flag (0 if found in cache, 1 otherwise),
  NULL if error occured.
*/
cache_entry_t* CacheGetOrCreate(LRU_Cache_t* cache, const char* key,
                                int* created) {
  unsigned long  h = hash_str(key) % cache->buckets;
  cache_entry_t* cur;

  pthread_mutex_lock(&cache->bucket_locks[h]);
  cur = cache->table[h];
  while (cur && strcmp(cur->key, key) != 0) {
    cur = cur->hnext;
  }
  if (cur) {
    bool done = atomic_load(&cur->complete);
    *created = 0;
    pthread_mutex_unlock(&cache->bucket_locks[h]);

    if (done) {
      pthread_mutex_lock(&cache->list_lock);
      detach_node(cache, cur);
      insert_front(cache, cur);
      pthread_mutex_unlock(&cache->list_lock);
    }
    return cur;
  }

  cache_entry_t* e = malloc(sizeof(*e));
  if (!e) {
    pthread_mutex_unlock(&cache->bucket_locks[h]);
    return NULL;
  }
  e->key = strdup(key);
  e->value = NULL;
  e->size = e->capacity = 0;
  atomic_store(&e->complete, false);
  atomic_store(&e->deleted, false);
  atomic_store(&e->refcount, 0);
  pthread_rwlock_init(&e->rwlock, NULL);
  pthread_mutex_init(&e->cond_mutex, NULL);
  pthread_cond_init(&e->cond, NULL);

  e->hnext = cache->table[h];
  cache->table[h] = e;
  pthread_mutex_unlock(&cache->bucket_locks[h]);

  pthread_mutex_lock(&cache->list_lock);
  insert_front(cache, e);
  pthread_mutex_unlock(&cache->list_lock);

  *created = 1;
  return e;
}


/*
  Append data from buf to entry value, realloc value if needed.
  Return 0 in success, 1 if entry/buf/len invalid or realloc was not
  success.
*/
int CacheAppend(cache_entry_t* entry, const char* buf, size_t len) {
  if (!entry || !buf || len == 0)
    return EXIT_FAILURE;

  pthread_rwlock_wrlock(&entry->rwlock);
  if (entry->size + len > entry->capacity) {
    size_t nc = entry->capacity == 0 ? len * 2 : entry->capacity * 2;
    while (nc < entry->size + len)
      nc *= 2;
    char* p = realloc(entry->value, nc);
    if (!p) {
      pthread_rwlock_unlock(&entry->rwlock);
      return EXIT_FAILURE;
    }
    entry->value = p;
    entry->capacity = nc;
  }
  memcpy(entry->value + entry->size, buf, len);
  entry->size += len;
  pthread_rwlock_unlock(&entry->rwlock);

  pthread_mutex_lock(&entry->cond_mutex);
  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->cond_mutex);

  return EXIT_SUCCESS;
}


/*
  Mark entry complete and start eviction mechanism.
  Return -1 if cache or entry invalid, else 0.
*/
int CacheFinish(LRU_Cache_t* cache, cache_entry_t* entry) {
  if (!cache || !entry)
    return -1;

  atomic_store(&entry->complete, 1);
  pthread_mutex_lock(&entry->cond_mutex);
  pthread_cond_broadcast(&entry->cond);
  pthread_mutex_unlock(&entry->cond_mutex);

  atomic_fetch_add(&cache->size, entry->size);
  while (atomic_load(&cache->size) > cache->capacity) {
    evict_tail(cache);
  }
  return 0;
}


void Destroy_entry(cache_entry_t* e) {
  pthread_rwlock_destroy(&e->rwlock);
  pthread_mutex_destroy(&e->cond_mutex);
  pthread_cond_destroy(&e->cond);
  free(e->value);
  free(e->key);
  free(e);
}
