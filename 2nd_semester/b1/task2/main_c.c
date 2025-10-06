#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void* mythread(void *arg) {
  char* x = "Hello world";
	return x;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	err = pthread_create(&tid, NULL, mythread, NULL);
	if (err) {
	  printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

  void* retval;
  err = pthread_join(tid, &retval);
  if (err) {
    printf("main: pthread_join() failed: %s", strerror(err));
    return -1;
  }
  printf("%s\n", (char*)retval);

	return 0;
}
