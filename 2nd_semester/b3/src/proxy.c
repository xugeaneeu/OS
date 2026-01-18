#include "proxy.h"
#include "config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdatomic.h>
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
  Use some lib to parse HTTP request. (Пока что это дипсик сгенерил xd)
*/
static int parse_request(const char* buf, char** out_host, char** out_port,
                         char** out_path) {
  char        method[16], url[1024], version[16];
  const char* p = buf;

  // 1) Разбор request-line
  if (sscanf(p, "%15s %1023s %15s", method, url, version) != 3)
    return -1;
  if (strcmp(method, "GET") != 0)
    return -1;
  // Разрешаем HTTP/1.0 и HTTP/1.1
  if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0)
    return -1;

  char *host = NULL, *port = NULL, *path = NULL;

  // 2) Абсолютный URI?
  if (strncmp(url, "http://", 7) == 0) {
    const char* hp = url + 7;
    // найти первый slash
    const char* slash = strchr(hp, '/');
    size_t      hlen = slash ? (size_t)(slash - hp) : strlen(hp);
    // скопировать host[:port]
    char* hostport = strndup(hp, hlen);
    if (!hostport)
      return -1;
    // разделить на host и port
    char* colon = strchr(hostport, ':');
    if (colon) {
      *colon = '\0';
      host = strdup(hostport);
      port = strdup(colon + 1);
    } else {
      host = strdup(hostport);
      port = strdup("80");
    }
    free(hostport);
    if (!host || !port) {
      free(host);
      free(port);
      return -1;
    }
    // путь
    if (slash)
      path = strdup(slash);
    else
      path = strdup("/");
    if (!path) {
      free(host);
      free(port);
      return -1;
    }
  } else {
    // 3) Относительный URI — ищем Host: header
    if (url[0] != '/')
      return -1;
    // Находим "\r\nHost:" или в начале после request-line — "Host:"
    char* hpos = strcasestr(buf, "\r\nHost:");
    if (!hpos) {
      // возможно без \r\n — на границе буфера
      hpos = strcasestr(buf, "\nHost:");
      if (!hpos)
        return -1;
    }
    // перепрыгнуть до текста значения
    hpos = strchr(hpos, ':');
    if (!hpos)
      return -1;
    hpos++;
    while (*hpos == ' ' || *hpos == '\t')
      hpos++;
    // читаем до \r или \n
    char* end = strpbrk(hpos, "\r\n");
    if (!end)
      return -1;
    char* hostport = strndup(hpos, (size_t)(end - hpos));
    if (!hostport)
      return -1;
    // разделяем host и port
    char* colon = strchr(hostport, ':');
    if (colon) {
      *colon = '\0';
      host = strdup(hostport);
      port = strdup(colon + 1);
    } else {
      host = strdup(hostport);
      port = strdup("80");
    }
    free(hostport);
    if (!host || !port) {
      free(host);
      free(port);
      return -1;
    }
    path = strdup(url);
    if (!path) {
      free(host);
      free(port);
      return -1;
    }
  }

  // 4) Успешно
  *out_host = host;
  *out_port = port;
  *out_path = path;
  return 0;
}


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
  Loader thread, starts at handle_connection if according cache entry is
  missing.
*/
static void* loader_thread(void* arg) {
  loader_arg_t*  la = arg;
  LRU_Cache_t*   cache = la->cache;
  cache_entry_t* e = la->entry;

  struct addrinfo  hints = {.ai_socktype = SOCK_STREAM};
  struct addrinfo* res;
  if (getaddrinfo(la->host, la->port, &hints, &res))
    goto CLEAN;

  int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (s < 0) {
    freeaddrinfo(res);
    goto CLEAN;
  }
  if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
    close(s);
    freeaddrinfo(res);
    goto CLEAN;
  }
  freeaddrinfo(res);

  char* req = build_origin_request(la->host, la->path);
  if (req) {
    send(s, req, strlen(req), 0);
    free(req);
  }

  char    buf[8192];
  ssize_t r;
  while ((r = recv(s, buf, sizeof(buf), 0)) > 0) {
    if (CacheAppend(e, buf, (size_t)r))
      break;
  }
  close(s);

  CacheFinish(cache, e);

CLEAN:
  free(la->host);
  free(la->port);
  free(la->path);
  free(la);
  return NULL;
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

  char    hdr[8192];
  ssize_t got, tot = 0;
  while (!shutdown_flag) {
    got = recv(fd, hdr + tot, sizeof(hdr) - 1 - tot, 0);
    if (got <= 0)
      goto FIN;
    tot += got;
    hdr[tot] = '\0';
    if (strstr(hdr, "\r\n\r\n"))
      break;
    if (tot >= (ssize_t)sizeof(hdr) - 1)
      goto FIN;
  }

  char *host = NULL, *port = NULL, *path = NULL;
  if (parse_request(hdr, &host, &port, &path)) {
    goto FREE_HDR;
  }

  size_t L = strlen(host) + 1 + strlen(port) + strlen(path) + 1;
  char*  key = malloc(L);
  snprintf(key, L, "%s:%s%s", host, port, path);

  int            created;
  cache_entry_t* e = CacheGetOrCreate(serv->cache, key, &created);

  if (created) {
    loader_arg_t* la = malloc(sizeof(*la));
    la->cache = serv->cache;
    la->entry = e;
    la->host = strdup(host);
    la->port = strdup(port);
    la->path = strdup(path);
    pthread_t tid;
    pthread_create(&tid, NULL, loader_thread, la);
    pthread_detach(tid);
  }

  size_t offset = 0;
  for (;;) {
    pthread_mutex_lock(&e->lock);
    while (offset == e->size && !atomic_load(&e->complete)) {
      pthread_cond_wait(&e->cond, &e->lock);
    }
    size_t new_bytes = e->size - offset;
    pthread_mutex_unlock(&e->lock);

    if (atomic_load(&e->complete) && new_bytes == 0)
      break;
    if (new_bytes > 0) {
      char*  p = e->value + offset;
      size_t tosend = new_bytes;
      while (tosend) {
        ssize_t w = send(fd, p, tosend, 0);
        if (w <= 0)
          break;
        p += w;
        tosend -= w;
      }
      offset += new_bytes;
    }
  }

FREE_HDR:
  free(host);
  free(port);
  free(path);
  free(key);
FIN:
  close(fd);
  atomic_store(&c->busy, 0);
  atomic_fetch_sub(&serv->conn_cnt, 1);
  return NULL;
}

/*-------------------API-------------------*/

/*
  Initialize proxy struct and create cache
  (interface to use stored cache exists, but not implemented).
*/
int Proxy(ProxyServer* serv, ProxyConfig* cfg) {
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

    if (Cache(serv->cache, LRU_CACHE_CAP, LRU_CACHE_BUCKETS) != 0) {
      free(serv->cache);
      return -1;
    }
  }

  atomic_store(&serv->conn_cnt, 0);
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    atomic_store(&serv->connections[i].busy, 0);
    serv->connections[i].serv = serv;
  }

  return 0;
}


/*
  Main endless proxy loop, gracefully shutdown when handle SIGINT.
*/
int Serve(ProxyServer* serv) {
  if (!serv)
    return -1;

  struct sigaction saction = {.sa_handler = on_sigint};
  sigemptyset(&saction.sa_mask);
  saction.sa_flags = 0;
  sigaction(SIGINT, &saction, NULL);

  int ls = socket(AF_INET, SOCK_STREAM, 0);
  if (ls < 0)
    return -1;

  int opt = 1;
  setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in sa = {.sin_family = AF_INET,
                           .sin_addr.s_addr = INADDR_ANY,
                           .sin_port = htons(PORT)};

  if (bind(ls, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
    close(ls);
    return -1;
  }

  if (listen(ls, MAX_CONNECTIONS) < 0) {
    close(ls);
    return -1;
  }

  serv->listener = ls;

  while (!shutdown_flag) {
    int client_fd = accept(ls, NULL, NULL);
    if (client_fd < 0) {
      if (errno == EINTR && shutdown_flag)
        break;
      continue;
    }

    if (atomic_load(&serv->conn_cnt) >= MAX_CONNECTIONS) {
      const char* err = "HTTP/1.0 503 Service Unavailable\r\n"
                        "Connection: close\r\n\r\n";
      send(client_fd, err, strlen(err), 0);
      close(client_fd);
      continue;
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      int exp = 0;
      if (atomic_compare_exchange_strong(&serv->connections[i].busy, &exp, 1)) {
        serv->connections[i].client_fd = client_fd;
        atomic_fetch_add(&serv->conn_cnt, 1);
        pthread_create(&serv->connections[i].thread, NULL, handle_connection,
                       &serv->connections[i]);
        break;
      }
    }
  }

  return Shutdown(serv);
}


/*
  Gracefully shutdown all connections.
*/
int Shutdown(ProxyServer* serv) {
  shutdown_flag = 1;
  if (serv->listener >= 0)
    close(serv->listener);

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
