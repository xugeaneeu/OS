#include "proxy.h"

#include <stdlib.h>

/*------------place for statics------------*/

/*
  Loader thread, starts at handle_connection.
  Load to cache or response to client from cache.
*/
static void loader_thread(void) {

}


/*
  Run loader thread to fetch response from dest server
  to cache, if response is missing in cache.
  Fetch from cache response and send to client.
*/
static void handle_connection(void) {

}

/*-------------------API-------------------*/

/*
  Initialize proxy struct and create cache
  (interface to use stored cache exists, but not implemented).
*/
int Proxy(ProxyServer* serv, ProxyConfig* cfg) {


  return EXIT_SUCCESS;
}


/*
  Main endless proxy loop, gracefully shutdown when handle SIGINT.
*/
int Serve(ProxyServer* serv) {


  return EXIT_SUCCESS;
}


/*
  Gracefully shutdown all connections.
*/
int Shutdown(ProxyServer* serv) {
  

  return EXIT_SUCCESS;
}
