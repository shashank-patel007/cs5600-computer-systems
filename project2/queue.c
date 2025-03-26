#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "queue.h"

static work_item *queue_head = NULL;
static work_item *queue_tail = NULL;
static int queued_requests = 0;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

static int shutdown_flag = 0;

void queue_init() {
    queue_head = queue_tail = NULL;
    queued_requests = 0;
    shutdown_flag = 0;
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
}

void queue_shutdown() {
    pthread_mutex_lock(&queue_mutex);
    shutdown_flag = 1;
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

void enqueue_work(int sock_fd) {
    work_item* item = malloc(sizeof(work_item));
    if (!item) {
        perror("malloc");
        close(sock_fd);
        return;
    }
    item->sock_fd = sock_fd;
    item->next = NULL;
    pthread_mutex_lock(&queue_mutex);
    if (queue_tail == NULL) {
        queue_head = queue_tail = item;
    } else {
        queue_tail->next = item;
        queue_tail = item;
    }
    queued_requests++;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int dequeue_work(void) {
    pthread_mutex_lock(&queue_mutex);
    while (queue_head == NULL && !shutdown_flag) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    if (shutdown_flag && queue_head == NULL) {
        pthread_mutex_unlock(&queue_mutex);
        return -1;
    }
    work_item *item = queue_head;
    queue_head = item->next;
    if (queue_head == NULL)
        queue_tail = NULL;
    queued_requests--;
    int sock_fd = item->sock_fd;
    free(item);
    pthread_mutex_unlock(&queue_mutex);
    return sock_fd;
}

int queue_length() {
    pthread_mutex_lock(&queue_mutex);
    int count = queued_requests;
    pthread_mutex_unlock(&queue_mutex);
    return count;
}

void queue_cleanup() {
    pthread_mutex_lock(&queue_mutex);
    while (queue_head) {
        work_item *temp = queue_head;
        queue_head = queue_head->next;
        close(temp->sock_fd);
        free(temp);
    }
    queue_tail = NULL;
    pthread_mutex_unlock(&queue_mutex);
}
