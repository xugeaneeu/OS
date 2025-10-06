#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

void *my_thread(void *arg) {
  for (;;) {
    printf("thread: printing line\n");
    sleep(1);
  }
  return NULL;
}

int main(void) {
  pthread_t t;

  int err;
  if ((err = pthread_create(&t, NULL, my_thread, NULL))) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
		return EXIT_FAILURE;
  }

  sleep(3);

  if ((err = pthread_cancel(t))) {
    printf("main: pthread_cancel() failed: %s\n", strerror(err));
		return EXIT_FAILURE;
  }

  if ((err = pthread_join(t, NULL))) {
    printf("main: pthread_join() failed: %s", strerror(err));
    return -1;
  }

  return 0;
}