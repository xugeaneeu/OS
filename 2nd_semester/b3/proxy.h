#pragma once

#include "cache.h"

#include <stdint.h>


/*------------types-------------*/

typedef struct ProxyServer {
  LRU_Cache_t *cache;
  int listener;
} ProxyServer;

typedef struct ProxyConfig {
  char cacheless;
  LRU_Cache_t* cache;

  uint16_t port;
} ProxyConfig;


/*-------------API--------------*/

int Proxy(ProxyServer* serv, ProxyConfig* cfg);
int Serve(ProxyServer* serv);
int Shutdown(ProxyServer* serv);
