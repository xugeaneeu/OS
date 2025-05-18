#include <arpa/inet.h>
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

  struct sockaddr_in sa, ca;
  socklen_t client_len;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port =  htons(SERV_PORT);

  if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    printf("Error when binding name to socket\n");
    return EXIT_FAILURE;
  }

  char buffer[BUFFER_SIZE];
  ssize_t len_receive;
  while (1) {
    client_len = sizeof(ca);
    len_receive = recvfrom(sd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&ca, &client_len);
    if (len_receive == -1) {
      printf("Error when receiving\n");
      close(sd);
      return EXIT_FAILURE;
    }
    buffer[len_receive] = '\0';
    char client_ip[INET_ADDRSTRLEN];
    if (NULL == inet_ntop(AF_INET, &ca.sin_addr, client_ip, sizeof(client_ip))) {
      printf("And some errors more\n");
      close(sd);
      return EXIT_FAILURE;
    }
    printf("Receive %zd bytes from %s:%d\n", len_receive, client_ip, htons(ca.sin_port));
    sendto(sd, buffer, len_receive, 0, (struct sockaddr *)&ca, client_len);
  }

  close(sd);
  return EXIT_SUCCESS;
}