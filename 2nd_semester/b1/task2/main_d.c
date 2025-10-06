#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void* mythread(void *arg) {
  printf("my tid: %lu\n", pthread_self());
  return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  pthread_attr_t attr;
  if (pthread_attr_init(&attr)) {
    printf("initialization of attr didn't success");
    return EXIT_FAILURE;
  }
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
    printf("pthread_attr_setdetachstate() didn't success");
    return EXIT_FAILURE;
  }

  for (;;) {
    err = pthread_create(&tid, &attr, mythread, NULL);
    if (err) {
      printf("main: pthread_create() failed: %s\n", strerror(err));
      return EXIT_FAILURE;
    }
  }

  pthread_attr_destroy(&attr);
	return EXIT_SUCCESS;
}
