#define _GNU_SOURCE
#include <errno.h>
#include "mythread_create.h"
#include <signal.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE (1<<20)

int threadFunc(void* args) {
	void* (*start_routine)(void*) = ((void**)(args))[0];
	void* routineArg = ((void**)(args))[1];

	start_routine(routineArg);

	free((void**)args);
	return EXIT_SUCCESS;
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg) {
  if (!thread || !start_routine) {
    errno = EINVAL;
    return EXIT_FAILURE;
  }

  void* stackaddr = malloc(STACK_SIZE);
  if (!stackaddr) {
    perror("failed malloc()");
    return EXIT_FAILURE;
  }

  void** funcArgs = malloc(2 * sizeof(void*));
  if (!funcArgs) {
    perror("failed mallo()");
    free(stackaddr);
    return EXIT_FAILURE;
  }

  funcArgs[0] = start_routine;
  funcArgs[1] = arg;
  int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;

  thread->tid = clone(threadFunc, (char*)stackaddr + STACK_SIZE, flags | SIGCHLD, funcArgs);

  if (thread->tid == -1) {
    perror("failed clone()");
    free(stackaddr);
    free(funcArgs);
    return EXIT_FAILURE;
  }

  thread->stackaddr = stackaddr;

  return EXIT_SUCCESS;
}
