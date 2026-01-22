#pragma once
#include <stdint.h>

typedef struct NetListener NetListener;

/*
  Create a listening socket on port port with a backlog queue.
  On success, return 0 and write the result to *out.
  On error, return -1.
*/
int NetListener_create(NetListener** out, uint16_t port, int backlog);

/*
  Close the listening socket and free the structure.
*/
void NetListener_destroy(NetListener* lst);

/*
  Accept a new connection. Blocks until accept() is called.
  Returns client fd ≥0, or -1 on error.
*/
int NetListener_accept(NetListener* lst);