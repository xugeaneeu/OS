#pragma once

typedef unsigned long mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);
void mythread_exit(void *retval);
mythread_t mythread_self(void);
int mythread_equal(mythread_t t1, mythread_t t2);
int mythread_join(mythread_t thread, void **retval);
int mythread_detach(mythread_t thread);
int mythread_cancel(mythread_t thread);
void mythread_testcancel(void);
void mythread_cleanup_push(void (*rtn)(void *), void *arg);
void mythread_cleanup_pop(int execute);