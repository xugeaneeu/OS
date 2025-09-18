#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

int main(int argc, char** argv) {
  int fd;
  char buffer[BUFFER_SIZE];
  int ret;

  if (argc != 2) {
    printf("Usage: ./%s [file-name]\n", argv[0]);
    return EXIT_FAILURE;
  }

  printf("uid: %d\ngid: %d\neuid: %d\negid: %d\n", getuid(), getgid(), geteuid(), getegid());

  FILE* f = fopen(argv[1], "r");
  if (!f && errno == EACCES) {
    printf("permission denied\n");
    return EXIT_FAILURE;
  }

  do {
    ret = fread(buffer, sizeof(char), BUFFER_SIZE, f);
    printf("%s", buffer);
  } while (ret == BUFFER_SIZE);

  printf("\n");

  return EXIT_SUCCESS;
}