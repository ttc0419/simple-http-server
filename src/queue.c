/**
 * Copyright (c) 2022, William TANG <galaxyking0419@gmail.com>
 * Generic implementation of queue data structure in C
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

void queue_init(queue_t *restrict queue, size_t item_size, size_t init_queue_size, size_t step_size, size_t shrink_size)
{
	if ((queue->head = (unsigned char *)malloc(item_size * init_queue_size)) == NULL) {
		fputs("Queue->FATAL: Could not allocate memory!\n", stderr);
		exit(1);
	}

	queue->tail  = queue->head + init_queue_size * item_size;
	queue->front = queue->head;
	queue->rear  = queue->head;

	queue->item_size   = item_size;
	queue->step_size   = step_size;
	queue->shrink_size = shrink_size;
}

void queue_populate_init(queue_t *restrict queue, size_t item_size, void *data, size_t data_size, size_t step_size, size_t shrink_size)
{
	if (data_size % item_size) {
		fputs("Queue->FATAL: data_size is not aligned properly!\n", stderr);
		exit(-1);
	}

	if ((queue->head = (unsigned char *)malloc(data_size + step_size * item_size)) == NULL) {
		fputs("Queue->FATAL: Could not allocate memory!\n", stderr);
		exit(1);
	}

	memcpy(queue->head, data, data_size);

	queue->front	 = queue->head;
	queue->rear	     = queue->head + data_size;
	queue->tail	     = queue->rear + step_size * item_size;

	queue->item_size   = item_size;
	queue->step_size   = step_size;
	queue->shrink_size = shrink_size;
}

void *queue_find_the_first_of(queue_t *restrict queue, void *item)
{
	for (unsigned char *ptr = queue->front; ptr != queue->rear; ptr += queue->item_size) {
		if (memcmp(ptr, item, queue->item_size) == 0)
			return ptr;
	}
	return NULL;
}

void enqueue(queue_t *restrict queue, void *item)
{
	/* If the rear reaches the end of allocated memory, reallocate for more */
	if (queue->rear == queue->tail) {
		if (queue->front == queue->head) {
			unsigned char *prev = queue->head;

			if ((queue->head = (unsigned char *)realloc(queue->head, queue->tail - queue->head + queue->step_size * queue->item_size)) == NULL) {
				fputs("Queue->FATAL: Could not allocate more memory!", stderr);
				exit(1);
			}

			/* if the pointer did not change, don't update front and rear pointer */
			if (queue->head != prev) {
				queue->rear += (ptrdiff_t)(queue->head - prev);
				queue->front = queue->head;
			}

			/* Always update tail */
			queue->tail = queue->rear + queue->step_size * queue->item_size;
		} else {
			memcpy(queue->head, queue->front, queue->rear - queue->front);
			queue->rear  = queue->head + (queue->rear - queue->front);
			queue->front = queue->head;
		}
	}
	/* copy the item to the end of the queue */
	memcpy(queue->rear, item, queue->item_size);
	queue->rear += queue->item_size;
}

void *dequeue(queue_t *restrict queue)
{
	/* Return NULL if there is nothing in the queue */
	if (queue->front == queue->rear)
		return NULL;

	/* Shrink the queue size if there is too much empty space */
	if (((queue->front - queue->head) + (queue->tail - queue->rear)) / queue->item_size >= queue->shrink_size) {
		if (queue->front != queue->head) {
			memcpy(queue->head, queue->front, queue->rear - queue->front);
			queue->rear  = queue->head + (queue->rear - queue->front);
			queue->front = queue->head;
		}

		unsigned char *prev = queue->head;
		/* if the pointer did not change, don't update front and rear pointer */
		if ((queue->head = (unsigned char *)realloc(queue->head, queue->rear - queue->front + queue->step_size * queue->item_size)) != prev) {
			queue->rear += (ptrdiff_t)(queue->head - prev);
			queue->front = queue->head;
		}

		/* Always update tail */
		queue->tail = queue->rear + queue->step_size * queue->item_size;
	}

	queue->front += queue->item_size;

	return (void *)(queue->front - queue->item_size);
}