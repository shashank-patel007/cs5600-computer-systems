#ifndef QUEUE_H
#define QUEUE_H

typedef struct process {
    int id;
    char *name;
} process_t;

typedef struct node {
    void *data;
    struct node *next;
} node_t;

typedef struct queue {
    node_t *front;
    node_t *rear;
} queue_t;

// Queue operations
void init_queue(queue_t *queue);
void enqueue(queue_t *queue, void *element);
void *dequeue(queue_t *queue);
int is_empty(queue_t *queue);

#endif