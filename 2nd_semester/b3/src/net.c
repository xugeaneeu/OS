#include "net.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct NetListener {
  int sockfd;
};

int NetListener_create(NetListener** out, uint16_t port, int backlog) {
  int ls = socket(AF_INET, SOCK_STREAM, 0);
  if (ls < 0)
    return -1;

  int opt = 1;
  if (setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(ls);
    return -1;
  }

  struct sockaddr_in sa = {.sin_family = AF_INET,
                           .sin_addr.s_addr = INADDR_ANY,
                           .sin_port = htons(port)};

  if (bind(ls, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
    close(ls);
    return -1;
  }
  if (listen(ls, backlog) < 0) {
    close(ls);
    return -1;
  }

  NetListener* lst = malloc(sizeof(*lst));
  if (!lst) {
    close(ls);
    return -1;
  }
  lst->sockfd = ls;
  *out = lst;
  return 0;
}

void NetListener_destroy(NetListener* lst) {
  if (!lst)
    return;
  close(lst->sockfd);
  free(lst);
}

int NetListener_accept(NetListener* lst) {
  if (!lst)
    return -1;
  return accept(lst->sockfd, NULL, NULL);
}