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
  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd == -1) {
    printf("Can't open socket\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  sa.sin_port = htons(SERV_PORT);

  char buffer[BUFFER_SIZE];
  ssize_t len_receive;
  for (;;) {
    if (NULL == fgets(buffer, BUFFER_SIZE, stdin)) {
      printf("error when read line\n");
      close(sd);
      return EXIT_FAILURE;
    }
    sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&sa, sizeof(sa));
    len_receive = recvfrom(sd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (len_receive == -1) {
      printf("Error when receiving\n");
      close(sd);
      return EXIT_FAILURE;
    }
    printf("%s\n", buffer);
  }
  
  close(sd);
  return EXIT_SUCCESS;
}