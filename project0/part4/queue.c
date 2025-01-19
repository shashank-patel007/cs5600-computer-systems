#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// Initialize the queue
void init_queue(queue_t *queue) {
    queue->front = NULL;
    queue->rear = NULL;
}

// Enqueue operation
void enqueue(queue_t *queue, void *element) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (!new_node) {
        perror("Failed to allocate memory for new node");
        exit(EXIT_FAILURE);
    }
    new_node->data = element;
    new_node->next = NULL;

    if (queue->rear == NULL) { // If queue is empty
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
}

// Dequeue operation
void *dequeue(queue_t *queue) {
    if (queue->front == NULL) { // If queue is empty
        return NULL;
    }

    node_t *temp = queue->front;
    void *data = temp->data;

    queue->front = queue->front->next;
    if (queue->front == NULL) { // If the queue becomes empty
        queue->rear = NULL;
    }

    free(temp);
    return data;
}

// Check if the queue is empty
int is_empty(queue_t *queue) {
    return queue->front == NULL;
}