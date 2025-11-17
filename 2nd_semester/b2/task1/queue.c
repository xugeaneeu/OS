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
  
  qnode_t *cur = q->first;
  while (cur) {
    qnode_t *next = cur->next;
    free(cur);
    cur = next;
  }
  
  free(q);
}

int queue_add(queue_t *q, int val) {
	q->add_attempts++;

	assert(q->count <= q->max_count);

	if (q->count == q->max_count) return 0;

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

	return 1;
}

/*
  Просто ужасный момент, я осознавал это наверное минут 20, мне пришлось первый раз 
  с первого курса взять в руки бумагу чтобы понять, что тут мать его не так, но я понял

  Prerequarities: (q->first = q->last) != NULL, соответственно q->first->next = NULL
  Итак, сначала мы берем в queue_get указатель на q->first, потом даже можем взять из него значение
  разницы не будет, потом переключение на queue_add происходит, мы создаем новую ноду и назначаем ее в q->last
  Тем временем в queue_get изменяем q->first = NULL, here we are. 
  Дальше освобождаем tmp и тд. В следующей итерации мы забываем про q->last, которую добавил предыдущий queue_add
  потому что затерли q->first, таким образом мы пропускаем ту самую единицу, на которую отличаются значения.

  Или все может пойти по иному сценарию, ситуация такая же, только в 103 строке мы успеваем назначить новую ноду
  в q->last, а сначала освобождаем tmp и только потом пытаемся сделать ->next у уже освобожденной структуры q->last
  здесь ловим сегфолт.
*/

int queue_get(queue_t *q, int *val) {
	q->get_attempts++;

	assert(q->count >= 0);

	if (q->count == 0) return 0;

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	return 1;
}

void queue_print_stats(queue_t *q) {
	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
}
