#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

static void sigint_handler(int sig) {
  if (sig == SIGINT) {
		printf("Thread 2: catch SIGINT\n");
	}
}

void *blocker_thread(void *arg) {
  sigset_t all;
  sigfillset(&all);
  pthread_sigmask(SIG_BLOCK, &all, NULL);
  printf("blocker: all signals blocked in this thread (inherited mask + explicit block)\n");
  
  for (;;) {
    pause();
  }
  return NULL;
}

void *handler_thread(void *arg) {
  sigset_t s;
  sigemptyset(&s);
  sigaddset(&s, SIGINT);
  pthread_sigmask(SIG_UNBLOCK, &s, NULL);

  printf("handler: SIGINT unblocked in this thread; waiting for SIGINT (ctrl-C)\n");

  for (;;) {
    pause();
  }
  return NULL;
}

void *sigwait_thread(void *arg) {
  sigset_t s;
  sigemptyset(&s);
  sigaddset(&s, SIGQUIT);

  printf("sigwait: SIGQUIT is blocked in this thread; calling sigwait (ctrl-\\)\n");

  int sig;
  if (sigwait(&s, &sig) == 0) {
    printf("sigwait: caught signal %d\n", sig);
  } else {
    perror("sigwait");
  }
  return NULL;
}

int main(void) {
  pthread_t t_blocker, t_handler, t_sigwait;
  int err;

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGQUIT);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    perror("pthread_sigmask");
    return EXIT_FAILURE;
  }

  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    return EXIT_FAILURE;
  }

  if ((err = pthread_create(&t_blocker, NULL, blocker_thread, NULL))) {
    fprintf(stderr, "pthread_create(blocker): %s\n", strerror(err));
    return EXIT_FAILURE;
  }
  if ((err = pthread_create(&t_handler, NULL, handler_thread, NULL))) {
    fprintf(stderr, "pthread_create(handler): %s\n", strerror(err));
    return EXIT_FAILURE;
  }
  if ((err = pthread_create(&t_sigwait, NULL, sigwait_thread, NULL))) {
    fprintf(stderr, "pthread_create(sigwait): %s\n", strerror(err));
    return EXIT_FAILURE;
  }
  void *res;
  pthread_join(t_sigwait, &res);
  printf("main: sigwait thread finished - canceling other threads\n");

  pthread_cancel(t_handler);
  pthread_cancel(t_blocker);

  pthread_join(t_handler, NULL);
  pthread_join(t_blocker, NULL);

  printf("main: done\n");
  return EXIT_SUCCESS;
}