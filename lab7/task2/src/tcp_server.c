#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 12067
#define BUFFER_SIZE 1024
#define LISTEN_BACKLOG 3

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  int bytes_received;

  while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
    buffer[bytes_received] = '\0';
    printf("\nMessage: %s", buffer);
    send(client_socket, buffer, bytes_received, 0);
  }

  close(client_socket);
  _exit(0);
}

int main(void) {
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd == -1) {
    printf("Can't open socket\n");
    return EXIT_FAILURE;
  }

  int opt = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port =  htons(SERV_PORT);

  if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    printf("Error when binding name to socket\n");
    close(sd);
    return EXIT_FAILURE;
  }

  if (listen(sd, LISTEN_BACKLOG) == -1) {
    printf("Some error in listen\n");
    close(sd);
    return EXIT_FAILURE;
  }

  int client_socket;
  struct sockaddr_in client_addr;
  socklen_t client_len;
  for (;;) {
    client_len = sizeof(client_addr);
    if ((client_socket = accept(sd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
      printf("Error in accept\n");
      close(sd);
      return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
      printf("Error when forking\n");
      close(sd);
      close(client_socket);
      return EXIT_FAILURE;
    } else if (child == 0) {
      close(sd);
      handle_client(client_socket);
    } else {
      close(client_socket);
    }
  }  

  close(sd);
  return EXIT_SUCCESS;
}
