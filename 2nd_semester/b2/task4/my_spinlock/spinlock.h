#pragma once

#include <stdatomic.h>

typedef struct {
  atomic_int lock;    // статусы лока: 0 - свободен, 1 - занят
  int owner;          // tid того кто владелец
  int cnt;            // счётчик захватов лока
} custom_spinlock_t;

void custom_spinlock_init(custom_spinlock_t *s);
void custom_spinlock_lock(custom_spinlock_t *s);
void custom_spinlock_unlock(custom_spinlock_t *s);