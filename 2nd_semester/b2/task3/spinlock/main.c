#include "list.h"

#define STORAGE_CAPACITY 10


void *count_monitor(void *arg) {
  Storage *s = arg;
  while (1) {
    Node *n = s->first;
    int total_swap = 0, total_asc = 0, total_dsc = 0, total_eq = 0;
    while (n) {
      pthread_spin_lock(&n->sync);
      printf("%s (swap=%d asc=%d dsc=%d eq=%d)\n", n->value, n->counter_swap, n->counter_asc, n->counter_dsc, n->counter_eq);
      total_swap += n->counter_swap;
      total_asc += n->counter_asc;
      total_dsc += n->counter_dsc;
      total_eq += n->counter_eq;
      pthread_spin_unlock(&n->sync);
      n = n->next;
    }

    printf("TOTAL: swap=%d asc=%d dsc=%d eq=%d\n\n", total_swap, total_asc, total_dsc, total_eq);
    sleep(1);
  }

  return NULL;
}

void update_counter(Node *curr1, Node *curr2, int type) {
  int pair_count = strlen(curr1->value) - strlen(curr2->value);
  switch (type) {
  case EQ:
    if(pair_count == 0) ++curr1->counter_eq;
    break;
  case ASC:
    if(pair_count < 0) ++curr1->counter_asc;
    break;
  case DESC:
    if(pair_count > 0) ++curr1->counter_dsc;
    break;
  }
}

void *compare_length_thread(void *data) {
  ThreadData *thread_data = (ThreadData *)data;
  Storage *storage = thread_data->storage;
  int type = thread_data->type;

  while (1) {
    Node *curr1;
    // для получения 1 элемента
    pthread_spin_lock(&storage->sync);

    if((curr1 = storage->first) == NULL) {
      printf("compare_length_thread(): curr1 is NULL\n");
      pthread_spin_unlock(&storage->sync);
      break;
    }

    pthread_spin_lock(&curr1->sync);

    pthread_spin_unlock(&storage->sync);

    Node *curr2 = curr1->next;
    while (curr2 != NULL) {
      pthread_spin_lock(&curr2->sync);
      // оба потока на момент сравнения имеют захваченные мьютексы
      // это логика из условия: - необходимо блокировать все записи с данными которых производится работа
      update_counter(curr1, curr2, type);

      pthread_spin_unlock(&curr1->sync);
      curr1 = curr2;
      curr2 = curr1->next;
    }

    pthread_spin_unlock(&curr1->sync);
    
  }
  
  return NULL;
}

/*
было: ... -> *curr1_next -> curr2 -> curr3 -> ...
стало: ... -> *curr1_next -> curr3 -> curr2 -> ...
*/
void swap_nodes(Node **curr1_next, Node *curr2, Node *curr3) {
  ++curr2->counter_swap;

  *curr1_next = curr3;
  curr2->next = curr3->next;
  curr3->next = curr2;
}

// из условия - при перестановке записей списка, необходимо блокировать три записи.
void *swap_thread(void *data) {
  Storage *storage = (Storage *)data;

  while (1) {
    Node *curr1, *curr2, *curr3;

    pthread_spin_lock(&storage->sync);

    if((curr1 = storage->first) == NULL) {
      printf("swap_thread(): curr1 is NULL\n");
      pthread_spin_unlock(&storage->sync);
      break;
    }

    pthread_spin_lock(&curr1->sync);

    if((curr2 = curr1->next) == NULL) {
      printf("swap_thread(): curr2 is NULL\n");
      pthread_spin_unlock(&curr1->sync);
      pthread_spin_unlock(&storage->sync);
      break;
    }

    pthread_spin_lock(&curr2->sync);

    if ((rand() % 2) == 0) {
      swap_nodes(&storage->first, curr1, curr2);
      curr1 = storage->first;
      curr2 = curr1->next;
    }

    pthread_spin_unlock(&storage->sync);
    // до конца списка делаю 50% свап узлов 
    curr3 = curr2->next;
    while (curr3 != NULL) {
      pthread_spin_lock(&curr3->sync);

      if ((rand() % 2) == 0) {
        swap_nodes(&curr1->next, curr2, curr3);
      }
      // ... -> *cur1->next -> curr3 -> curr2 -> ...
      curr3 = curr1->next;
      pthread_spin_unlock(&curr1->sync);
      curr1 = curr3;
      curr2 = curr1->next;
      curr3 = curr2->next;
    }

    pthread_spin_unlock(&curr1->sync);
    pthread_spin_unlock(&curr2->sync);
  }

  return NULL;
}

int main() {
  srand(time(NULL));

  Storage *storage = init_storage(STORAGE_CAPACITY);
  fill_storage(storage);

  pthread_t monitor, compare_asc_tid, compare_desc_tid, compare_eq_tid, swap_tid1, swap_tid2, swap_tid3;

  ThreadData compare_asc_data = {storage, ASC};
  ThreadData compare_desc_data = {storage, DESC};
  ThreadData compare_eq_data = {storage, EQ};

  pthread_create(&monitor, NULL, count_monitor, storage);
  pthread_create(&compare_asc_tid, NULL, compare_length_thread, &compare_asc_data);
  pthread_create(&compare_desc_tid, NULL, compare_length_thread, &compare_desc_data);
  pthread_create(&compare_eq_tid, NULL, compare_length_thread, &compare_eq_data);
  pthread_create(&swap_tid1, NULL, swap_thread, storage);
  pthread_create(&swap_tid2, NULL, swap_thread, storage);
  pthread_create(&swap_tid3, NULL, swap_thread, storage);

  pthread_join(compare_asc_tid, NULL);
  pthread_join(compare_desc_tid, NULL);
  pthread_join(compare_eq_tid, NULL);
  pthread_join(swap_tid1, NULL);
  pthread_join(swap_tid2, NULL);
  pthread_join(swap_tid3, NULL);
  pthread_join(monitor, NULL);

  return 0;
}