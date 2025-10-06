#pragma once

struct mythread_t {
  unsigned long tid;
  void*         stackaddr;
} typedef mythread_t;

int mythread_create(mythread_t* thread, void* (*start_routine)(void*), void* arg);