#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int *items;
    size_t capacity;
    size_t front;
    size_t rear;
    size_t count;
} Queue;

/* Initialize an empty queue with storage for capacity values. */
bool queue_init(Queue *queue, size_t capacity);
bool queue_enqueue(Queue *queue, int value);
bool queue_dequeue(Queue *queue, int *value);
bool queue_is_empty(const Queue *queue);
bool queue_is_full(const Queue *queue);
void queue_destroy(Queue *queue);

#endif
