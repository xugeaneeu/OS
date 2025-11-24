#include "list.h"

Storage *init_storage(int capacity) {
  Storage *storage = (Storage*)malloc(sizeof(Storage));
  if (!storage) {
    printf("malloc() failed to allocate memory for a queue\n");
    exit(EXIT_FAILURE);
  }

  pthread_rwlock_init(&(storage->sync), NULL);
  storage->capacity = capacity;
  storage->first = NULL;

  return storage;
}

Node *create_node(const char *value) {
  Node *new_node = (Node *) calloc(1, sizeof(*new_node));
  if (!new_node) {
    printf("malloc() failed to allocate memory for a new node\n");
    exit(EXIT_FAILURE);
  }

  strcpy(new_node->value, value);
  pthread_rwlock_init(&(new_node->sync), NULL);

  return new_node;
}

void add_node(Storage *storage, const char *value) {
  Node *new_node = create_node(value);

  if (storage->first != NULL) {
    Node *node = storage->first;
    while (node->next != NULL) {
      node = node->next;
    }
    node->next = new_node;
  } else {
    storage->first = new_node;
  }
}

void fill_storage(Storage *storage) {
  char buff[24];
  for (int i = 1; i <= storage->capacity; ++i) {
    sprintf(buff, "%d", i % (storage->capacity + 1));
    add_node(storage, buff);
  }
}