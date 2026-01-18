#pragma once

#include "cache.h"
#include "config.h"

#include <stdatomic.h>
#include <stdint.h>


/*------------types-------------*/

struct ProxyServer;

typedef struct Connection {
  pthread_t           thread;
  int                 client_fd;
  atomic_int          busy;
  struct ProxyServer* serv;
} Connection;


typedef struct ProxyServer {
  LRU_Cache_t* cache;
  int          listener;
  atomic_int   conn_cnt;
  Connection   connections[MAX_CONNECTIONS];
} ProxyServer;

typedef struct ProxyConfig {
  char         cacheless;
  LRU_Cache_t* cache;

  uint16_t port;
} ProxyConfig;


/*-------------API--------------*/

int Proxy(ProxyServer* serv, ProxyConfig* cfg);
int Serve(ProxyServer* serv);
int Shutdown(ProxyServer* serv);
