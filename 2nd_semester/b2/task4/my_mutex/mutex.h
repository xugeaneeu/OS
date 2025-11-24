#pragma once
#include <stdatomic.h>

typedef struct {
  atomic_int lock;  // статусы мьютекса: 0 - свободен, 1 - занят
  int owner;        // tid владельца
  int cnt;          // счётчик захватов лока
} custom_mutex_t;

void custom_mutex_init(custom_mutex_t *s);
void custom_mutex_lock(custom_mutex_t *s);
void custom_mutex_unlock(custom_mutex_t *s);