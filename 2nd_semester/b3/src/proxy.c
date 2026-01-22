#include "proxy.h"
#include "HTTPParser.h"
#include "config.h"
#include "net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


// Для graceful shutdown
// Потоки проверяют флаг на завершение конекшена,
// когда хэндлер ловит сигнал, он выставляет флаг в 0/1
// потоки внутри завершают текущие запросы, после завершаются
static volatile int shutdown_flag = 0;


/*------------place for statics------------*/

/*
  SIGINT handler, raise shutdown_flag for start graceful shutdown.
*/
static void on_sigint(int sig) { shutdown_flag = 1; }


/*
  Build request from proxy server to origin.
*/
static char* build_origin_request(const char* host, const char* path) {
  const char tmpl[] = "GET %s HTTP/1.0\r\n"
                      "Host: %s\r\n"
                      "Connection: close\r\n"
                      "\r\n";

  size_t len = sizeof(tmpl) + strlen(host) + strlen(path);
  char*  req = malloc(len);
  if (!req)
    return NULL;

  snprintf(req, len, tmpl, path, host);
  return req;
}


typedef struct {
  LRU_Cache_t*   cache;
  cache_entry_t* entry;
  char*          host;
  char*          port;
  char*          path;
} loader_arg_t;


/*
  Network part of loading to cache.
*/
static void fetch_from_origin(LRU_Cache_t* cache, cache_entry_t* entry,
                              const char* host, const char* port,
                              const char* path) {
  struct addrinfo  hints = {.ai_socktype = SOCK_STREAM};
  struct addrinfo* res = NULL;

  if (getaddrinfo(host, port, &hints, &res) != 0) {
    CacheFinish(cache, entry);
    return;
  }

  int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    freeaddrinfo(res);
    CacheFinish(cache, entry);
    return;
  }

  if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
    close(sock);
    freeaddrinfo(res);
    CacheFinish(cache, entry);
    return;
  }

  freeaddrinfo(res);

  char* req = build_origin_request(host, path);
  if (req) {
    send(sock, req, strlen(req), 0);
    free(req);
  }

  char    buf[8192];
  ssize_t n;
  while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
    if (CacheAppend(entry, buf, (size_t)n) != 0)
      break;
  }

  close(sock);
}



/*
  Loader thread, starts at handle_connection if according cache entry is
  missing.
*/
static void* loader_thread(void* arg) {
  loader_arg_t*  la = arg;
  LRU_Cache_t*   cache = la->cache;
  cache_entry_t* e = la->entry;

  fetch_from_origin(cache, e, la->host, la->port, la->path);

  CacheFinish(cache, e);

  free(la->host);
  free(la->port);
  free(la->path);
  free(la);
  return NULL;
}


/*
  Read from fd until find "\r\n\r\n" or error.
  Returns total length on success, -1 on failure.
*/
static ssize_t read_request(int fd, char* buf, size_t buf_size) {
  ssize_t tot = 0;
  while (1) {
    ssize_t got = recv(fd, buf + tot, buf_size - 1 - tot, 0);
    if (got <= 0)
      return -1;
    tot += got;
    buf[tot] = '\0';
    if (strstr(buf, "\r\n\r\n"))
      return tot;
    if (tot >= (ssize_t)buf_size - 1)
      return -1;
  }
}


/*
  Build the cache lookup key "host:port/path"
  Caller must free() the returned string.
*/
static char* build_cache_key(const char* host, const char* port,
                             const char* path) {
  size_t L = strlen(host) + 1 + strlen(port) + strlen(path) + 1;
  char*  key = malloc(L);
  if (!key)
    return NULL;
  snprintf(key, L, "%s:%s%s", host, port, path);
  return key;
}


/*
  Lookup or create a cache entry.  If newly created,
  spawn a loader_thread().
*/
static cache_entry_t* get_or_create_entry(ProxyServer* serv, const char* key,
                                          const char* host, const char* port,
                                          const char* path) {
  int            created;
  cache_entry_t* e = CacheGetOrCreate(serv->cache, key, &created);
  if (created) {
    loader_arg_t* la = malloc(sizeof(*la));
    if (!la) {
      atomic_store(&e->complete, 1);
      return e;
    }
    la->cache = serv->cache;
    la->entry = e;
    la->host = strdup(host);
    la->port = strdup(port);
    la->path = strdup(path);

    pthread_t tid;
    pthread_create(&tid, NULL, loader_thread, la);
    pthread_detach(tid);
  }
  return e;
}


/*
  Stream the contents of cache_entry 'e' to socket 'fd'.
  Blocks until entry->complete is set and all data sent.
*/
static void stream_entry_to_fd(int fd, cache_entry_t* e) {
  size_t offset = 0;

  atomic_fetch_add(&e->refcount, 1);

  for (;;) {
    pthread_mutex_lock(&e->cond_mutex);
    while (offset == e->size && !atomic_load(&e->complete)) {
      pthread_cond_wait(&e->cond, &e->cond_mutex);
    }
    pthread_mutex_unlock(&e->cond_mutex);

    pthread_rwlock_rdlock(&e->rwlock);
    size_t cur_size = e->size;
    bool   done = atomic_load(&e->complete) && (offset == cur_size);
    pthread_rwlock_unlock(&e->rwlock);

    if (offset < cur_size) {
      size_t      tosend = cur_size - offset;
      const char* p = e->value + offset;
      while (tosend) {
        ssize_t w = send(fd, p, tosend, 0);
        if (w <= 0) {
          goto finish;
        }
        p += w;
        tosend -= w;
      }
      offset = cur_size;
      continue;
    }

    if (done) {
      break;
    }
  }

finish:
  if (atomic_fetch_sub(&e->refcount, 1) == 1 && atomic_load(&e->deleted)) {
    Destroy_entry(e);
  }
}


/*
  Run loader thread to fetch response from dest server
  to cache, if response is missing in cache.
  Fetch from cache response and send to client.
*/
static void* handle_connection(void* arg) {
  Connection*  c = arg;
  ProxyServer* serv = c->serv;
  int          fd = c->client_fd;

  char *         host = NULL, *port = NULL, *path = NULL, *key = NULL;
  cache_entry_t* e = NULL;
  char           hdr[8192];

  if (read_request(fd, hdr, sizeof(hdr)) <= 0)
    goto DONE;

  if (Parse_request(hdr, &host, &port, &path) != 0)
    goto CLEANUP_PARTS;

  key = build_cache_key(host, port, path);
  if (!key)
    goto CLEANUP_PARTS;

  e = get_or_create_entry(serv, key, host, port, path);

  stream_entry_to_fd(fd, e);

CLEANUP_PARTS:
  free(host);
  free(port);
  free(path);
  free(key);
DONE:
  close(fd);
  atomic_store(&c->busy, 0);
  atomic_fetch_sub(&serv->conn_cnt, 1);
  sem_post(&serv->slot_sem);
  return NULL;
}


static int serve(ProxyServer* serv) {
  while (!shutdown_flag) {
    int client_fd = NetListener_accept(serv->listener);
    if (client_fd < 0) {
      if (errno == EINTR && shutdown_flag)
        break;
      continue;
    }

    sem_wait(&serv->slot_sem);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      int expected = 0;
      if (atomic_compare_exchange_strong(&serv->connections[i].busy, &expected,
                                         1)) {
        serv->connections[i].client_fd = client_fd;
        atomic_fetch_add(&serv->conn_cnt, 1);
        pthread_create(&serv->connections[i].thread, NULL, handle_connection,
                       &serv->connections[i]);
        break;
      }
    }
  }

  return 0;
}

/*-------------------API-------------------*/

/*
  Initialize proxy struct and create cache
  (interface to use stored cache exists, but not implemented).
*/
int InitProxy(ProxyServer* serv, ProxyConfig* cfg) {
  if (!serv || !cfg)
    return -1;

  if (cfg->cacheless) {
    serv->cache = NULL;
  } else if (cfg->cache) {
    serv->cache = cfg->cache;
  } else {
    serv->cache = malloc(sizeof(*serv->cache));
    if (!serv->cache)
      return -1;

    if (CreateCache(serv->cache, LRU_CACHE_EVICT_NUM, LRU_CACHE_BUCKETS)) {
      free(serv->cache);
      return -1;
    }
  }

  atomic_store(&serv->conn_cnt, 0);
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    atomic_store(&serv->connections[i].busy, 0);
    serv->connections[i].serv = serv;
  }

  sem_init(&serv->slot_sem, 0, MAX_CONNECTIONS);

  return 0;
}


/*
  Main endless proxy loop, gracefully shutdown when handle SIGINT.
*/
int InitServerAndServe(ProxyServer* serv) {
  if (!serv)
    return -1;

  struct sigaction saction = {.sa_handler = on_sigint};
  sigemptyset(&saction.sa_mask);
  saction.sa_flags = 0;
  sigaction(SIGINT, &saction, NULL);

  if (NetListener_create(&(serv->listener), PORT, MAX_CONNECTIONS))
    return -1;

  return serve(serv);
}


/*
  Gracefully shutdown all connections.
*/
int Shutdown(ProxyServer* serv) {
  shutdown_flag = 1;

  NetListener_destroy(serv->listener);

  sem_destroy(&serv->slot_sem);

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (atomic_load(&serv->connections[i].busy)) {
      pthread_join(serv->connections[i].thread, NULL);
    }
  }
  if (serv->cache) {
    DestroyCache(serv->cache);
    free(serv->cache);
    serv->cache = NULL;
  }
  return 0;
}

/*
  Return default config.
*/
ProxyConfig ProxyConfigDefault(void) {
  ProxyConfig cfg;
  cfg.port = PORT;
  cfg.cache = NULL;
  cfg.cacheless = 0;
  return cfg;
}