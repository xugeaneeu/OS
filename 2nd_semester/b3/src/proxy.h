#pragma once

#include "cache.h"
#include "config.h"
#include "net.h"

#include <semaphore.h>
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
  NetListener* listener;
  atomic_int   conn_cnt;
  Connection   connections[MAX_CONNECTIONS];
  sem_t        slot_sem;
} ProxyServer;

typedef struct ProxyConfig {
  char         cacheless;
  LRU_Cache_t* cache;

  uint16_t port;
} ProxyConfig;


/*-------------API--------------*/

int         InitProxy(ProxyServer* serv, ProxyConfig* cfg);
int         InitServerAndServe(ProxyServer* serv);
int         Shutdown(ProxyServer* serv);
ProxyConfig ProxyConfigDefault(void);