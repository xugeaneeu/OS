#include "../lib/uthread_create.h"

#include <stdint.h>
#include <stdio.h>

void *thread_func(void *arg) {
  int id = *(int*)arg;
  for (int i = 0; i < 5; ++i) {
    printf("Thread %d: iteration %d\n", id, i);
    uthread_yield();
  }
  return (void *)(intptr_t)(id);
}

int main(void) {
    if (uthreads_init() != 0) {
      fprintf(stderr, "uthread_system_init failed\n");
      return 1;
    }

    uthread_t t1, t2;
    int tid_1 = 1, tid_2 = 2;

    if (uthread_create(&t1, thread_func, (void *)(&tid_1)) != 0) {
      fprintf(stderr, "create t1 failed\n");
      return 1;
    }
    if (uthread_create(&t2, thread_func, (void *)(&tid_2)) != 0) {
      fprintf(stderr, "create t2 failed\n");
      return 1;
    }

    printf("Start t1 with uthread_run (will run until first yield or finish)\n");
    uthread_run(&t1);
    printf("Back in main after uthread_run(&t1)\n");

    printf("Now join t1 (this will resume it until completion) and get retval\n");
    void *r1 = uthread_join(&t1);
    printf("t1 returned %d\n", (int)(intptr_t)r1);

    printf("Join t2 directly (will run it to completion)\n");
    void *r2 = uthread_join(&t2);
    printf("t2 returned %d\n", (int)(intptr_t)r2);

    uthread_system_shutdown();
    return 0;
}