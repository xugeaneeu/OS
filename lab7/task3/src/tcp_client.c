#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 12067
#define BUFFER_SIZE 1024

int main(void) {
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd == -1) {
    printf("Can't open socket\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  sa.sin_port = htons(SERV_PORT);

  if (connect(sd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    printf("Error when connecting\n");
    close(sd);
    return EXIT_FAILURE;
  }

  char buffer[BUFFER_SIZE];
  ssize_t len_receive;
  for (;;) {
    if (NULL == fgets(buffer, BUFFER_SIZE, stdin)) {
      printf("Error when read line\n");
      close(sd);
      return EXIT_FAILURE;
    }
    
    if (send(sd, buffer, BUFFER_SIZE, 0) == -1) {
      printf("Error when sending msg\n");
      close(sd);
      return EXIT_FAILURE;
    }

    len_receive = recv(sd, buffer, BUFFER_SIZE, 0);
    if (len_receive == -1) {
      printf("Error when receiving\n");
      close(sd);
      return EXIT_FAILURE;
    } else if (len_receive == 0) {
      printf("Server closed connection\n");
      break;
    } else {
      buffer[len_receive] = '\0';
      printf("Server response: %s\n", buffer);
    }
  }
  
  close(sd);
  return EXIT_SUCCESS;
}
