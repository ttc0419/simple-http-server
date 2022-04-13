/**
 * Copyright (c) 2022, William TANG <galaxyking0419@gmail.com>
 * Generic implementation of queue data structure in C
 */
#ifndef SIMPLE_HTTP_SERVER_QUEUE_H
#define SIMPLE_HTTP_SERVER_QUEUE_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Queue struct */
typedef struct {
	size_t item_size;
	size_t step_size;
	size_t shrink_size;
	unsigned char *restrict head;
	unsigned char *restrict tail;
	unsigned char *restrict front;
	unsigned char *restrict rear;
} queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize a queue
 *
 * Parameters:
 * queue: pointer to a queue
 * item_size: the size of each element in the queue
 *
 * Return:
 * None
 */
extern void queue_init(queue_t *restrict queue, size_t item_size, size_t init_queue_size, size_t step_size, size_t shrink_size);

/* Initialize a queue with some data
 *
 * Parameters:
 * queue: pointer to a queue
 * item_size: the size of each element in the queue
 * data: pointer to the data
 * data_size: the size of the data
 *
 * Return:
 * None
 */
extern void queue_populate_init(queue_t *restrict queue, size_t item_size, void *data, size_t data_size, size_t step_size, size_t shrink_size);

/* Get the length of a queue
 *
 * Parameters:
 * queue: pointer to a queue
 *
 * Return:
 * None
 */
static inline size_t queue_len(queue_t *restrict queue)
{
	return (size_t)((queue->rear - queue->front) / queue->item_size);
}

/* Get an element based on index
 *
 * Parameters:
 * queue: pointer to a queue
 * index: the index of the element
 *
 * Return:
 * The pointer to the element,
 * NULL if the item does not exist
 */
static inline void *queue_get_item(queue_t *restrict queue, size_t index)
{
	if (index >= queue_len(queue))
		return NULL;

	return (void *)(queue->front + index * queue->item_size);
}

/* Get an element from the front of the queue
 *
 * Parameters:
 * queue: pointer to a queue
 *
 * Return:
 * The pointer to the element,
 * NULL if the item does not exist
 */
static inline void *queue_front(queue_t *restrict queue)
{
	if (queue->front == queue->rear)
		return NULL;

	return (void *)(queue->front);
}

/* Get an element at the back of the queue
 *
 * Parameters:
 * queue: pointer to a queue
 *
 * Return:
 * The pointer to the element,
 * NULL if the item does not exist
 */
static inline void *queue_back(queue_t *restrict queue)
{
	if (queue->front == queue->rear)
		return NULL;

	return (void *)(queue->rear - queue->item_size);
}

/* Find the first position of an element in a queue
 *
 * Parameters:
 * queue: pointer to a queue
 * item: the pointer to the item
 *
 * Return:
 * The pointer to the element in the queue,
 * NULL if the item does not exist
 */
extern void *queue_find_the_first_of(queue_t *restrict queue, void *item);

/* Append an element to the end of a queue
 *
 * Parameters:
 * queue: pointer to a queue
 * item: pointer to the item
 *
 * Return:
 * None
 */
extern void enqueue(queue_t *restrict queue, void *item);

/* Pop an element at the front of a queue
 *
 * Parameters:
 * queue: pointer to a queue
 *
 * Return:
 * The pointer to the element
 *
 * Note: User need to manually convert the void
 *	   pointer to appropriate data type
 *	   Return NULL if there is nothing to
 *	   in the queue
 */
extern void *dequeue(queue_t *restrict queue);

/* Destroy a queue
 *
 * Parameters:
 * queue: pointer to a queue
 *
 * Return:
 * None
 *
 * Note: ALWAYS call it to prevent memory leak
 */
static inline void queue_destroy(queue_t *restrict queue)
{
	free(queue->head);
}

#ifdef __cplusplus
}
#endif

#endif