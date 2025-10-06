#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

struct S {
  int n;
  char *s;
} typedef S;

void *thread_func(void *arg) {
  S *p = arg;
  printf("in thread: n=%d, s=%s\n", p->n, p->s);
  return NULL;
}

int main(void) {
  struct S st = { 7, "hello" };
  pthread_t tid;
  int err = pthread_create(&tid, NULL, thread_func, &st);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return EXIT_FAILURE;
	}
  
  err = pthread_join(tid, NULL);
  if (err) {
    printf("main: pthread_join() failed: %s", strerror(err));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}