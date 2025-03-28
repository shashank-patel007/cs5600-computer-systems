#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to print the queue contents
void print_queue(queue_t *queue) {
    node_t *current = queue->front;
    if (current == NULL) {
        printf("[Empty]\n");
        return;
    }

    while (current != NULL) {
        process_t *process = (process_t *)current->data;
        printf("[id: %d, name: %s]", process->id, process->name);
        current = current->next;
        if (current != NULL) {
            printf(" -> ");
        }
    }
    printf("\n");
}

int main() {
    queue_t queue;
    init_queue(&queue);

    // Create and enqueue process_t elements
    process_t *p1 = (process_t *)malloc(sizeof(process_t));
    p1->id = 1;
    p1->name = strdup("A");

    process_t *p2 = (process_t *)malloc(sizeof(process_t));
    p2->id = 2;
    p2->name = strdup("B");

    process_t *p3 = (process_t *)malloc(sizeof(process_t));
    p3->id = 3;
    p3->name = strdup("C");

    printf("Enqueue: [id: %d, name: %s] is enqueued. ", p1->id, p1->name);
    enqueue(&queue, p1);
    print_queue(&queue);

    printf("Enqueue: [id: %d, name: %s] is enqueued. ", p2->id, p2->name);
    enqueue(&queue, p2);
    print_queue(&queue);

    printf("Enqueue: [id: %d, name: %s] is enqueued. ", p3->id, p3->name);
    enqueue(&queue, p3);
    print_queue(&queue);

    // Dequeue process_t elements
    process_t *deq = (process_t *)dequeue(&queue);
    printf("Dequeue: [id: %d, name: %s] is dequeued. ", deq->id, deq->name);
    free(deq->name);
    free(deq);
    print_queue(&queue);

    deq = (process_t *)dequeue(&queue);
    printf("Dequeue: [id: %d, name: %s] is dequeued. ", deq->id, deq->name);
    free(deq->name);
    free(deq);
    print_queue(&queue);

    deq = (process_t *)dequeue(&queue);
    printf("Dequeue: [id: %d, name: %s] is dequeued. ", deq->id, deq->name);
    free(deq->name);
    free(deq);
    print_queue(&queue);

    return 0;
}