#pragma once

#include <sys/ucontext.h>

typedef struct uthread {
    ucontext_t context;
    void *stack;
    int finished;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
} uthread_t;

int uthreads_init(void);
void uthread_system_shutdown(void);

int uthread_create(uthread_t *thread, void *(*start_routine)(void *), void *arg);
void uthread_run(uthread_t *thread);
void uthread_yield(void);
void uthread_exit(void *retval);
void *uthread_join(uthread_t *thread);