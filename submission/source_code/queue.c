#include "queue.h"

#include <stdint.h>
#include <stdlib.h>

bool queue_init(Queue *queue, size_t capacity)
{
    if (queue == NULL || capacity == 0 ||
        capacity > SIZE_MAX / sizeof(*queue->items)) {
        return false;
    }

    queue->items = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;

    queue->items = malloc(capacity * sizeof(*queue->items));
    if (queue->items == NULL) {
        return false;
    }

    queue->capacity = capacity;
    return true;
}

bool queue_enqueue(Queue *queue, int value)
{
    if (queue == NULL || queue->items == NULL || queue_is_full(queue)) {
        return false;
    }

    queue->items[queue->rear] = value;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;
    return true;
}

bool queue_dequeue(Queue *queue, int *value)
{
    if (queue == NULL || value == NULL || queue->items == NULL ||
        queue_is_empty(queue)) {
        return false;
    }

    *value = queue->items[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;
    return true;
}

bool queue_is_empty(const Queue *queue)
{
    return queue == NULL || queue->count == 0;
}

bool queue_is_full(const Queue *queue)
{
    return queue != NULL && queue->capacity > 0 &&
           queue->count == queue->capacity;
}

void queue_destroy(Queue *queue)
{
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    queue->items = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
}
