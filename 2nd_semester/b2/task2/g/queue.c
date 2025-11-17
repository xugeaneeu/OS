#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "queue.h"

void *qmonitor(void *arg) {
	queue_t *q = (queue_t *)arg;

	printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		queue_print_stats(q);
		sleep(1);
	}

	return NULL;
}

queue_t* queue_init(int max_count) {
	int err;

	queue_t *q = malloc(sizeof(queue_t));
	if (!q) {
		printf("Cannot allocate memory for a queue\n");
		abort();
	}

	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

  if (sem_init(&q->slots, 0, max_count) != 0) {
    perror("sem_init(slots)");
    abort();
  }
  if (sem_init(&q->items, 0, 0) != 0) {
    perror("sem_init(items)");
    abort();
  }
  if (sem_init(&q->mutex, 0, 1) != 0) {
    perror("sem_init(mutex)");
    abort();
  }

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}

	return q;
}

void queue_destroy(queue_t *q) {
  int err;
  err = pthread_cancel(q->qmonitor_tid);
  if (err) {
    fprintf(stderr, "queue_destroy: pthread_cancel() failed: %s\n", strerror(err));
  }
  
  err = pthread_join(q->qmonitor_tid, NULL);
  if (err) {
    fprintf(stderr, "queue_destroy: pthread_join() failed: %s\n", strerror(err));
  }
  
  sem_wait(&q->mutex);
  qnode_t *cur = q->first;
  while (cur) {
    qnode_t *next = cur->next;
    free(cur);
    cur = next;
  }
  sem_post(&q->mutex);
  
  sem_destroy(&q->slots);
  sem_destroy(&q->items);
  sem_destroy(&q->mutex);
  free(q);
}

int queue_add(queue_t *q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

  sem_wait(&q->slots);
  sem_wait(&q->mutex);

	qnode_t *new = malloc(sizeof(qnode_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}

	new->val = val;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

  sem_post(&q->mutex);
  sem_post(&q->items);

	return 1;
}

int queue_get(queue_t *q, int *val) {
	q->get_attempts++;

	assert(q->count >= 0);

  sem_wait(&q->items);
  sem_wait(&q->mutex);

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

  sem_post(&q->mutex);
  sem_post(&q->slots);

	return 1;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
          q->count,
          q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
          q->add_count, q->get_count, q->add_count - q->get_count);
}
