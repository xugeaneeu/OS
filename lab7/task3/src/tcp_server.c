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






// #define PORT 8080 
// #define MAX_CLIENTS 5
// #define BUFFER_SIZE 4096

// int main() {
//     int server_fd, client_fds[MAX_CLIENTS], max_clients = 0;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);
//     fd_set readfds;
//     char buffer[BUFFER_SIZE];

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//         perror("setsockopt");
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, MAX_CLIENTS) < 0) {
//         perror("listen");
//         exit(EXIT_FAILURE);
//     }

//     printf("Server listening on port %d\n", PORT);

//     for (int i = 0; i < MAX_CLIENTS; ++i) {
//         client_fds[i] = 0;
//     }

//     while (1) {
//         FD_ZERO(&readfds); // Clear set
//         FD_SET(server_fd, &readfds); // Add server socket to set

//         // Add all active client sockets to set
//         for (int i = 0; i < MAX_CLIENTS; ++i) {
//             if (client_fds[i] > 0) {
//                 FD_SET(client_fds[i], &readfds);
//             }
//         }

//         // Find the highest file descriptor
//         int max_fd = server_fd;
//         for (int i = 0; i < MAX_CLIENTS; ++i) {
//             if (client_fds[i] > max_fd) {
//                 max_fd = client_fds[i];
//             }
//         }

//         // Wait for activity on sockets using select()
//         int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
//         if ((activity < 0) && (errno != EINTR)) {
//             perror("select error");
//             exit(EXIT_FAILURE);
//         }

//         // If server socket is ready, there is a new connection
//         if (FD_ISSET(server_fd, &readfds)) {
//             int new_client_fd;

//             if ((new_client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//                 perror("accept error");
//                 exit(EXIT_FAILURE);
//             }

//             printf("New connection, socket fd is %d, IP is : %s, port : %d\n", new_client_fd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

//             // Add new socket to array of sockets
//             for (int i = 0; i < MAX_CLIENTS; ++i) {
//                 if (client_fds[i] == 0) {
//                     client_fds[i] = new_client_fd;
//                     if (i >= max_clients) {
//                         max_clients = i + 1;
//                     }
//                     break;
//                 }
//             }
//         }

//         // Check all client sockets for activity
//         for (int i = 0; i < max_clients; ++i) {
//             int client_socket = client_fds[i];
//             if (FD_ISSET(client_socket, &readfds)) {
//                 int valread;
//                 if ((valread = recv(client_socket, buffer, BUFFER_SIZE, 0)) == 0) {
//                     printf("Client disconnected, socket fd is %d\n", client_socket);
//                     close(client_socket);
//                     client_fds[i] = 0;
//                 } else {
//                     if (strncmp(buffer, "GET", 3) == 0) {
//                         // Send HTTP response with animated circles
//                         send_html_response(client_socket);
//                         close(client_socket);
//                         client_fds[i] = 0;
//                     } else {
//                         // Echo back to client
//                         buffer[valread] = '\0';
//                         printf("\nMessage: %sFrom socket_fd: %d\nTime: %s\n", buffer, client_socket, get_current_time());
//                         send(client_socket, buffer, valread, 0);
//                     }
//                 }
//             }
//         }
//     }

//     return 0;
// }