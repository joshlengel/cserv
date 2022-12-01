#include"tools.h"

#include<stdlib.h>
#include<string.h>

void fdqueue_init(FDQueue *queue, size_t capacity)
{
    queue->capacity = capacity;
    queue->size = 0;
    queue->arr = calloc(capacity, sizeof(int));
}

int fdqueue_empty(const FDQueue *queue)
{
    return queue->size == 0;
}

int fdqueue_push(FDQueue *queue, int fd)
{
    if (queue->size >= queue->capacity)
        return -1;
    
    queue->arr[queue->size++] = fd;
    return 0;
}

int fdqueue_pop(FDQueue *queue)
{
    if (queue->size == 0)
        return -1;
    
    const int fd = queue->arr[0];
    memmove(queue->arr, queue->arr + 1, (queue->size - 1) * sizeof(int));
    --queue->size;
    return fd;
}

void fdqueue_free(FDQueue *queue)
{
    free(queue->arr);
}