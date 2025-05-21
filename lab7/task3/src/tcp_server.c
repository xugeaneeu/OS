#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

#define SERV_PORT 12067
#define BUFFER_SIZE 1024
#define LISTEN_BACKLOG 5
#define MAX_CLIENTS 100

int main(void) {
  int serv_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    printf("Can't open socket\n");
    return EXIT_FAILURE;
  }

  int opt = 1;
  setsockopt(serv_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port =  htons(SERV_PORT);

  if (bind(serv_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    printf("Error when binding name to socket\n");
    close(serv_socket);
    return EXIT_FAILURE;
  }

  if (listen(serv_socket, LISTEN_BACKLOG) == -1) {
    printf("Some error in listen\n");
    close(serv_socket);
    return EXIT_FAILURE;
  }

  struct pollfd fds[MAX_CLIENTS+1];
  int nfds = 1;
  fds[0].fd = serv_socket;
  fds[0].events = POLLIN;
  for (size_t i = 1; i <= MAX_CLIENTS; ++i) {
    fds[i].fd = -1;
  }

  int client_socket;
  struct sockaddr_in ca;
  socklen_t client_len;
  for (;;) {
    int ready = poll(fds, nfds, -1);
    if (ready == -1) {
      printf("Incorrect poll call\n");
      close(serv_socket);
      return EXIT_FAILURE;
    }

    if (fds[0].revents & POLLIN) {
      client_len = sizeof(ca);
      client_socket = accept(serv_socket, (struct sockaddr *)&ca, &client_len);
      if (client_socket == -1) {
        printf("Error when accepting\n");
        close(serv_socket);
        return EXIT_FAILURE;
      }

      size_t i;
      for (i = 1; i <= MAX_CLIENTS; ++i) {
        if (fds[i].fd < 0) {
          fds[i].fd = client_socket;
          fds[i].events = POLLIN;
          break;
        }
      }
      if (i > MAX_CLIENTS) {
        printf("Too many clients\n");
        close(client_socket);
        continue;
      }

      if (i+1 > nfds) {
        nfds = i+1;
      }

      char addr[INET_ADDRSTRLEN];
      if (NULL == inet_ntop(AF_INET, &ca.sin_addr, addr, sizeof(addr))) {
        printf("Error when try to convert addr\n");
        close(client_socket);
        close(serv_socket);
        return EXIT_FAILURE;
      }

      printf("New connection: %s:%d\n", addr, htons(ca.sin_port));

      if (--ready == 0) continue;
    }

    char buffer[BUFFER_SIZE];
    ssize_t len_receive;
    for (size_t i = 1; (i < nfds) && (ready > 0); ++i) {
      if (fds[i].fd < 0) continue;
      if (fds[i].revents & POLLIN) {
        if ((len_receive = recv(fds[i].fd, buffer, BUFFER_SIZE, 0)) == -1) {
          printf("Error when receiving\n");
          close(fds[i].fd);
          fds[i].fd = -1;
        } else {
          send(fds[i].fd, buffer, len_receive, 0);
        }
        --ready;
      }
    }
  }

  close(serv_socket);
  return EXIT_SUCCESS;
}
