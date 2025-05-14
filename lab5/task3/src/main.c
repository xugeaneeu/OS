#define _GNU_SOURCE
#include <stdint.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define STACK_SIZE (2<<20)

static int recurse_depth = 10;

void recursion(int cur_depth) {
  if (cur_depth == 0) {
    return;
  }
  char str[] = "hello world\n";
  recursion(cur_depth - 1);
}

int child_fn(void *arg) {
  recursion(recurse_depth);
  _exit(0);
}

int main() {
  int fd_stack = open("resources/stack.bin",  O_RDWR|O_CREAT, 0600);
  if (fd_stack == -1) {
    printf("Can't open file resources/stack.bin\n");
    return EXIT_FAILURE;
  }

  if (ftruncate(fd_stack, STACK_SIZE) == -1) {
    printf("error when truncate\n");
    return EXIT_FAILURE;
  }

  void *stack = mmap(NULL, STACK_SIZE, PROT_READ|PROT_WRITE,MAP_SHARED, fd_stack, 0);
  if (stack == MAP_FAILED) {
    printf("mmap is failed\n");
    close(fd_stack);
    return EXIT_FAILURE;
  }

  void *child_sp = (char*)stack + STACK_SIZE;
  child_sp = (void*)(((uintptr_t)child_sp & ~0xF) - 8);

  pid_t child_pid = clone(child_fn, child_sp, SIGCHLD, NULL);
  if (child_pid == -1) {
    close(fd_stack);
    return EXIT_FAILURE;
  }

  wait(NULL);
  munmap(stack, STACK_SIZE);
  close(fd_stack);
  return 0;
}