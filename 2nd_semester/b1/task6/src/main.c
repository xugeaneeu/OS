#include "./../lib/mythread_create.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* routine(void* args) {
  char* str = args;
  printf("mythread hello\n");
  printf("%s\n", str);
  return NULL;
}

int main(void) {
  mythread_t tid;
  int err;

  char* args = "args";
  if ((err = mythread_create(&tid, routine, args))) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
		return EXIT_FAILURE;
  }

  sleep(3);

  return EXIT_SUCCESS;
}