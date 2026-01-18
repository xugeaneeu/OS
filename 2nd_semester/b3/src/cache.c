#include "cache.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*------------place for statics------------*/

/*
  Simple hash to str function.
*/
static unsigned long hash_str(const char* str) {
  unsigned long hash = 5381;
  int           c;
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
static void insert_at_front(LRU_Cache_t* cache, cache_entry_t* node) {
  node->prev = NULL;
  node->next = cache->head;
  if (cache->head)
    cache->head->prev = node;
  cache->head = node;
  if (!cache->tail)
    cache->tail = node;
}


/*
  Pop node from tail.
*/
static void evict_tail(LRU_Cache_t* cache) {
  if (!cache->tail)
    return;
  cache_entry_t* node = cache->tail;

  unsigned long  h = hash_str(node->key) % cache->buckets;
  cache_entry_t* cur = cache->table[h];
  cache_entry_t* prev = NULL;
  while (cur) {
    if (cur == node) {
      if (prev)
        prev->hnext = cur->hnext;
      else
        cache->table[h] = cur->hnext;
      break;
    }
    prev = cur;
    cur = cur->hnext;
  }

  detach_node(cache, node);

  if (cache->size >= node->size)
    cache->size -= node->size;
  else
    cache->size = 0;

  free(node->key);
  free(node->value);
  free(node);
}


/*-------------------API-------------------*/

/*
  Initialize cache storage with according capasity.
  Return 0 in success, 1 if cache is NULL / capasity is 0 /
  memory allocation error occured.
*/
int Cache(LRU_Cache_t* cache, size_t capacity, size_t buckets) {
  if (!cache || capacity == 0 || buckets == 0) {
    return EXIT_FAILURE;
  }

  cache->capacity = capacity;
  cache->size = 0;

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
  if (!cache) {
    return EXIT_FAILURE;
  }

  pthread_mutex_lock(&cache->lock);
  cache_entry_t* cur = cache->head;
  while (cur) {
    cache_entry_t* n = cur->next;
    free(cur->key);
    free(cur->value);
    free(cur);
    cur = n;
  }
  free(cache->table);

  cache->table = NULL;
  cache->head = cache->tail = NULL;
  cache->size = 0;
  cache->capacity = 0;

  pthread_mutex_unlock(&cache->lock);

  pthread_mutex_destroy(&cache->lock);
  return EXIT_SUCCESS;
}


/*

*/
int AddElem(LRU_Cache_t* cache, const char* key, const char* value,
            const size_t size) {
  if (!cache || !key || (!value && size > 0))
    return EXIT_FAILURE;

  pthread_mutex_lock(&cache->lock);

  unsigned long  h = hash_str(key) % cache->buckets;
  cache_entry_t* cur = cache->table[h];
  while (cur) {
    if (strcmp(cur->key, key) == 0) {
      assert(cache->size >= cur->size);
      cache->size -= cur->size;

      // here need to refactor, now firstly copy full response to cache, when
      // unlock mutex
      char* newval = NULL;
      if (size > 0) {
        newval = malloc(size);
        if (!newval) {
          cache->size += cur->size;
          pthread_mutex_unlock(&cache->lock);
          return EXIT_FAILURE;
        }
        memcpy(newval, value, size);
      }
      free(cur->value);
      cur->value = newval;
      cur->size = size;
      cache->size += size;

      detach_node(cache, cur);
      insert_at_front(cache, cur);

      while (cache->capacity > 0 && cache->size > cache->capacity) {
        evict_tail(cache);
      }

      pthread_mutex_unlock(&cache->lock);
      return EXIT_SUCCESS;
    }
    cur = cur->hnext;
  }

  cache_entry_t* node = malloc(sizeof(cache_entry_t));
  if (!node) {
    pthread_mutex_unlock(&cache->lock);
    return EXIT_FAILURE;
  }

  node->key = strdup(key);
  if (!node->key) {
    free(node);
    pthread_mutex_unlock(&cache->lock);
    return EXIT_FAILURE;
  }

  // here need to refactor, now firstly copy full response to cache, when unlock
  // mutex
  node->value = NULL;
  if (size > 0) {
    node->value = malloc(size);
    if (!node->value) {
      free(node->key);
      free(node);
      pthread_mutex_unlock(&cache->lock);
      return -1;
    }
    memcpy(node->value, value, size);
  }
  node->size = size;
  node->prev = node->next = NULL;
  node->hnext = NULL;

  node->hnext = cache->table[h];
  cache->table[h] = node;

  insert_at_front(cache, node);

  cache->size += size;

  while (cache->capacity > 0 && cache->size > cache->capacity) {
    evict_tail(cache);
  }

  pthread_mutex_unlock(&cache->lock);
  return EXIT_SUCCESS;
}


/*
  Find value by provided key.
  Return copy of finded value if success, NULL if cache/key is NULL /
  memory allocation error occured.
*/
cache_entry_t* FindElem(LRU_Cache_t* cache, const char* key) {
  if (!cache || !key) {
    return NULL;
  }
  pthread_mutex_lock(&cache->lock);

  unsigned long  h = hash_str(key) % cache->buckets;
  cache_entry_t* cur = cache->table[h];

  while (cur) {
    if (strcmp(cur->key, key) == 0) {
      detach_node(cache, cur);
      insert_at_front(cache, cur);

      cache_entry_t* ret = malloc(sizeof(cache_entry_t));
      if (!ret) {
        pthread_mutex_unlock(&cache->lock);
        return NULL;
      }

      ret->key = strdup(cur->key);
      ret->size = cur->size;
      if (cur->size > 0) {
        ret->value = malloc(cur->size);
        if (!ret->value) {
          free(ret->key);
          free(ret);
          pthread_mutex_unlock(&cache->lock);
          return NULL;
        }
        memcpy(ret->value, cur->value, cur->size);
      } else {
        ret->value = NULL;
      }
      ret->prev = ret->next = ret->hnext = NULL;

      pthread_mutex_unlock(&cache->lock);
      return ret;
    }

    cur = cur->hnext;
  }
  pthread_mutex_unlock(&cache->lock);
  return NULL;
}