#include "uthreads.h"

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <errno.h>

#define STACK_SIZE   (1 << 20)
#define MAX_UTHREADS (1 << 7)

static ucontext_t mainContext, exitContext;
static void *exit_stack = NULL;

static uthread_t *curThread = NULL;

static uthread_t *runqueue[MAX_UTHREADS];
static size_t rq_head = 0, rq_tail = 0;
static size_t threadNum = 0;

static int uthreads_initialized = 0;

static void thread_trampoline(void) {
  if (curThread && curThread->start_routine) {
    curThread->retval = curThread->start_routine(curThread->arg);
  } else {
    curThread->retval = NULL;
  }
}

static void exit_trampoline(void) {
  if (curThread) {
    if (curThread->stack) {
      free(curThread->stack);
      curThread->stack = NULL;
    }
    curThread->finished = 1;
  }
  setcontext(&mainContext);
}

int uthreads_init(void) {
  if (uthreads_initialized) {
    return EXIT_SUCCESS;
  }

  if (getcontext(&exitContext) == -1) {
    return EXIT_FAILURE;
  }
  
  exit_stack = malloc(STACK_SIZE);
  if (!exit_stack) {
    return EXIT_FAILURE;
  }

  exitContext.uc_stack.ss_sp = exit_stack;
  exitContext.uc_stack.ss_size = STACK_SIZE;
  exitContext.uc_stack.ss_flags = 0;
  exitContext.uc_link = &mainContext;
  makecontext(&exitContext, (void (*)(void))exit_trampoline, 0);

  rq_head = rq_tail = threadNum = 0;
  curThread = NULL;
  uthreads_initialized = 1;

  return EXIT_SUCCESS;
}

void uthread_system_shutdown(void) {
  if (!uthreads_initialized) return;
    
  if (exit_stack) {
    free(exit_stack);
    exit_stack = NULL;
  }

  uthreads_initialized = 0;
}

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg) {
  if (!uthreads_initialized) {
    errno = EINVAL;
    return EXIT_FAILURE;
  }

  if (!thread || !start_routine) {
    errno = EINVAL;
    return EXIT_FAILURE;
  }

  if (threadNum >= MAX_UTHREADS) {
    errno = EAGAIN;
    return EXIT_FAILURE;
  }

  if (getcontext(&thread->context) == -1) {
    return EXIT_FAILURE;
  }

  thread->stack = malloc(STACK_SIZE);
  if (!thread->stack) {
    return EXIT_FAILURE;
  }

  thread->context.uc_stack.ss_sp = thread->stack;
  thread->context.uc_stack.ss_size = STACK_SIZE;
  thread->context.uc_stack.ss_flags = 0;
  thread->context.uc_link = &exitContext;

  thread->start_routine = start_routine;
  thread->arg = arg;
  thread->finished = 0;
  thread->retval = NULL;

  makecontext(&thread->context, (void (*)(void))thread_trampoline, 0);
  
  runqueue[rq_tail] = thread;
  rq_tail = (rq_tail + 1) % MAX_UTHREADS;
  threadNum++;

  return EXIT_SUCCESS;
}

void uthread_run(void) {
  if (!uthreads_initialized) return;

  while (threadNum > 0) {
    uthread_t *next = runqueue[rq_head];
    rq_head = (rq_head + 1) % MAX_UTHREADS;
    threadNum--;

    if (next->finished) continue;

    curThread = next;
    swapcontext(&mainContext, &curThread->context);
  }

  curThread = NULL;
}

void uthread_yield(void) {
  if (curThread && !curThread->finished) {
    runqueue[rq_tail] = curThread;
    rq_tail = (rq_tail + 1) % MAX_UTHREADS;
    threadNum++;
    swapcontext(&curThread->context, &mainContext);
  }
}

void uthread_exit(void *retval) {
  if (!curThread) {
    setcontext(&mainContext);
    return;
  }
  curThread->retval = retval;
  setcontext(&exitContext);
}

void *uthread_join(uthread_t *thread) {
  if (!thread) return NULL;

  while (!thread->finished) {
    uthread_yield();
  }

  return thread->retval;
}
