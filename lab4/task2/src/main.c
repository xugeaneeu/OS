#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define BUFFER_SIZE 4096

void sigsegv_handler(int sign) {
  printf("Signal: %d (%s)\n", sign, strsignal(sign));
  exit(1);
}

void recursion_stack_allocation(void) {
  char buffer[BUFFER_SIZE];
  usleep(10000);
  recursion_stack_allocation();
}

void heap_allocation(void) {
  size_t size = 0;
  int block_size = 4096 * 1000;

  while(1) {
    malloc(block_size);
    size += block_size;
    sleep(1);
  }
}

void mmap_allocation(void) {
  void* addr;
  if (MAP_FAILED == (addr = mmap(NULL, 10*getpagesize(), PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))) {
    printf("shit happens\n");
  }
  sleep(10);
  if (mprotect(addr, 10*getpagesize(), PROT_READ) == -1) {
    printf("Error code: %d\n", errno);
  }
  sleep(5);

  if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
    printf("Error\n");
    return;
  }

  //*(char*)addr = 3;

  munmap((void*)((char*)addr + 4*getpagesize()), 3*getpagesize());
  sleep(5);

  return;
}

int main(int argc, char** argv) {
  printf("PID: %d\n\n", getpid());
  sleep(1);

  #ifdef A
  
  execv(argv[0], argv);
  printf("helllllo wrld\n");

  #endif

  #ifdef C1

  sleep(10);
  recursion_stack_allocation();

  #endif

  #ifdef C2

  sleep(10);
  heap_allocation();

  #endif

  #ifdef C3

  sleep(10);
  mmap_allocation();

  #endif

  return EXIT_SUCCESS;
}