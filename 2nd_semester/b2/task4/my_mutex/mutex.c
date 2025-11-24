#define _GNU_SOURCE
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>

#include "mutex.h"

// если значение в uaddr равно val, поток заснет
int futex_wait(atomic_int *uaddr, int val) {
  return syscall(SYS_futex, uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}
// разбудить до n потоков, которые ждут на адресе uaddr
int futex_wake(atomic_int *uaddr, int n) {
  return syscall(SYS_futex, uaddr, FUTEX_WAKE, n, NULL, NULL, 0);
}

void custom_mutex_init(custom_mutex_t *s) {
  atomic_init(&s->lock, 0);
  s->owner = 0;
  s->cnt = 0;
}

void custom_mutex_lock(custom_mutex_t *s) {
  pid_t me = gettid();
  // если текущий поток владелец
  if (s->owner == me) {
    s->cnt++;
    return;
  }

  // попытка захвата мьютекса 
  while (1) {
    int expected = 0;
    if (atomic_compare_exchange_strong(&s->lock, &expected, 1)) {
      s->owner = me;
      s->cnt = 1;
      return;
    }
    // засыпаем и ждем пока другой поток не освободит лок
    futex_wait(&s->lock, 1);
  }
}

void custom_mutex_unlock(custom_mutex_t *s) {
  pid_t me = gettid();
  
  // если мьютекс был захвачен другим потоком
  if (s->owner != me) {
    return;
  }
  
  // если поток захватывал мьютекс больше одного раза, то пока рано
  if (--s->cnt > 0) {
    return;
  }

  s->owner = 0; // после освобождения мьютекса у него нет владельца
  // сначала сбрасываю мьютекс атомарно, потом пробуждаю 1 спящий
  atomic_store(&s->lock, 0);
  futex_wake(&s->lock, 1);
}