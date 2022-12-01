#pragma once

#include<stddef.h>

typedef struct
{
    size_t capacity, size;
    int *arr;
} FDQueue;

void fdqueue_init(FDQueue *queue, size_t capacity);
int fdqueue_empty(const FDQueue *queue);
int fdqueue_push(FDQueue *queue, int fd);
int fdqueue_pop(FDQueue *queue);
void fdqueue_free(FDQueue *queue);