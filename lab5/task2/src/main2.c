#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
  pid_t father_pid = fork();
  if (father_pid == -1) {
    printf("error when forking\n");
    return EXIT_FAILURE;
  }

  if (father_pid == 0) {
    pid_t child_pid = fork();
    if (child_pid == -1) {
      printf("error when forking\n");
      return EXIT_FAILURE;
    }

    if (child_pid == 0) {
      while (1) {
        sleep(20);
        printf("child\n");
      }
    } else {
      sleep(15);
      exit(5);
    }
  } else {
    while (1) {
      sleep(20);
      printf("grand: don't handle SIGCHLD\n");
    }
  }
}