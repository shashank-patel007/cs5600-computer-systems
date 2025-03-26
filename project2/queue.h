#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct work_item {
    int sock_fd;
    struct work_item *next;
} work_item;

void queue_init();
void enqueue_work(int sock_fd);
int dequeue_work();
void queue_shutdown();
int queue_length();
void queue_cleanup();

#endif
