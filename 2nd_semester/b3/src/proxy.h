#pragma once

#include "cache.h"
#include "config.h"

#include <stdatomic.h>
#include <stdint.h>


/*------------types-------------*/

typedef struct Connection {

} Connection;


typedef struct ProxyServer {
  LRU_Cache_t *cache;
  int listener;

  //Для graceful shutdown
  //Потоки постоянно проверяют флаг на завершение конекшена,
  //когда хэндлер ловит сигнал, он выставляет флаг в 0/1
  //потоки внутри завершают текущие запросы, после завершаются
  //Возможно имеет смысл посмотреть в строну testcancel

  atomic_int conn_cnt;
  Connection connections[MAX_CONNECTIONS];
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
