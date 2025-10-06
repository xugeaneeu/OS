#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_OF_THREADS 5

int global_var = 69;

void *mythread(void *arg) {
  static int loc_st_var = 1488;
  int loc_var = 228;
  const int loc_const_var = 322;
	printf("mythread [%d %d tid:%d ptid:%lu]\n", getpid(), getppid(), gettid(), pthread_self());

  printf("variables in thread: global_var: %d, loc_st_var: %d, loc_var: %d, loc_const_var: %d\n", global_var, loc_st_var, loc_var, loc_const_var);

  global_var = 96;
  printf("global_var is changed\n");
  loc_var = 288;
  printf("loc_var is changed\n");

	return NULL;
}

int main() {
	pthread_t tid[NUM_OF_THREADS];
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  for (int i = 0; i < NUM_OF_THREADS; ++i) {
    err = pthread_create(&tid[i], NULL, mythread, NULL);
    if (err) {
      printf("main: pthread_create() (%d iteration) failed: %s\n", i, strerror(err));
      return -1;
    }
  }

  sleep(30);

  for (int i = 0; i < NUM_OF_THREADS; ++i) {
    err = pthread_join(tid[i], NULL);
    printf("tid from pthread_create(): %lu\n", tid[i]);
    if (err) {
      printf("main: pthread_join() failed: %s", strerror(err));
      return -1;
    }
  }

	return 0;
}
