#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

struct S {
  int n;
  char *s;
} typedef S;

void *thread_func(void *arg) {
  S *p = arg;
  printf("in thread: n=%d, s=%s\n", p->n, p->s);
  free(p);
  return NULL;
}

int main(void) {
  struct S* p = malloc(sizeof(S));
  if (!p) {
    return EXIT_FAILURE;
  }
  p->n = 42;
  p->s = strdup("detached");

  pthread_t tid;
  int err = pthread_create(&tid, NULL, thread_func, p);
  if (err) {
    printf("main: pthread_create() failed: %s\n", strerror(err));
    return EXIT_FAILURE;
	}

  if ((err = pthread_detach(tid))) {
    printf("main: pthread_detach() failed: %s\n", strerror(err));
    free(p);
    return EXIT_FAILURE;
  }

  sleep(1);

  return EXIT_SUCCESS;
}