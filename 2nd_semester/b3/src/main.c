#include "proxy.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


int main(void) {
  ProxyConfig cfg = ProxyConfigDefault();
  ProxyServer serv;

  if (InitProxy(&serv, &cfg)) {
    fprintf(stderr, "error when creating server");
    return EXIT_FAILURE;
  }

  if (InitServerAndServe(&serv)) {
    Shutdown(&serv);
    return EXIT_FAILURE;
  }

  Shutdown(&serv);

  return EXIT_SUCCESS;
}
