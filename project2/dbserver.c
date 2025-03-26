#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include "proj2.h"
#include "database.h"
#include "queue.h"

#define PORT 5000
#define WORKERS 4

void handle_work(int sock_fd);

int stat_reads = 0;
int stat_writes = 0;
int stat_deletes = 0;
int stat_failed = 0;
int stat_objects = 0; 
pthread_mutex_t stat_mutex = PTHREAD_MUTEX_INITIALIZER;

int shutdown_flag = 0;
int listener_sock_fd = -1;
int server_port = PORT;

void* listener_thread(void *arg) {
    int port = *((int *)arg);
    listener_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if(listener_sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = 0};
    setsockopt(listener_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(listener_sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("can't bind");
        exit(1);
    }
    if (listen(listener_sock_fd,5) < 0){
        perror("listen failed");
        exit(1);
    }
    printf("Listener thread running on port %d\n",port);
    while(!shutdown_flag){
        int fd = accept(listener_sock_fd,NULL,NULL);
        if (fd < 0) {
            if (shutdown_flag) {
                break;  
            }
            perror("Accept failed");
            continue;
        }
        enqueue_work(fd);
    }
    close(listener_sock_fd);
    printf("Exiting\n");
    return NULL;
}

void* worker_thread(void *arg) {
    while (1) {
        int fd = dequeue_work();
        if (fd == -1){
            break;
        }
        usleep(random() % 10000);
        handle_work(fd);
        close(fd);
    }
    return NULL;
}

void handle_work(int sock_fd) {
    struct request req;
    struct request response;
    char buf_write[4096];
    char buf_read[4096];
    int len;
    int status;

    if (read(sock_fd, &req, sizeof(req)) <=0) {
        perror("Failed to read request");
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_failed++;
        pthread_mutex_unlock(&stat_mutex);
        return;
    }

    if (req.op_status == 'Q') {
        shutdown_flag = 1;
        close(sock_fd);
        queue_shutdown();
        queue_cleanup();
        db_cleanup();
        exit(0);
    }

    switch (req.op_status) {
        case 'W':
            len = atoi(req.len);
            if (len > 4096) {
                len = 4096;
            }
            if (read(sock_fd, buf_write, len) != len) {
                perror("Failed to read provided data");
                response.op_status = 'X';
                break;
            }
            status = db_write(req.name, buf_write, len);
            if (status == 0) {
                response.op_status = 'K';
            } else {
                response.op_status = 'X';
            }
            snprintf(response.len, sizeof(response.len), "%d", 0);
            break;
        case 'R':
            len = db_read(req.name, buf_read);
            if (len > 0) {
                response.op_status = 'K';
                snprintf(response.len, sizeof(response.len), "%d", len);
            } else {
                response.op_status = 'X';
                snprintf(response.len, sizeof(response.len), "%d", 0);
            }
            break;
        case 'D':
            status = db_delete(req.name);
            if (status == 0) {
                response.op_status = 'K';
                snprintf(response.len, sizeof(response.len), "%d", 0);
                break;
            } 
            response.op_status = 'X';
            snprintf(response.len, sizeof(response.len), "%d", 0);
            break;
        default:
            perror("invalid operation");
            response.op_status = 'X';
            snprintf(response.len, sizeof(response.len), "%d", 0);
            break;
    }

    memset(response.name, 0 , sizeof(response.name));
    write(sock_fd, &response, sizeof(response));
    if (req.op_status == 'R' && response.op_status == 'K') {
        write(sock_fd, buf_read, len);
    }

    pthread_mutex_lock(&stat_mutex);
    if (req.op_status == 'R') stat_reads++;
    if (req.op_status == 'W') stat_writes++;
    if (req.op_status == 'D') stat_deletes++;
    if (response.op_status == 'X') stat_failed++;
    pthread_mutex_unlock(&stat_mutex);
}

void print_stats(void) {
    pthread_mutex_lock(&stat_mutex);
    printf("Database objects: %d\n", count_valid_objects());
    printf("Read requests: %d\n", stat_reads);
    printf("Write requests: %d\n", stat_writes);
    printf("Delete requests: %d\n", stat_deletes);
    printf("Failed requests: %d\n", stat_failed);
    pthread_mutex_unlock(&stat_mutex);
    
    printf("Requests in queue: %d\n", queue_length());
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        server_port = atoi(argv[1]);
    }
    queue_init();

    pthread_t listener_tid;
    pthread_t worker_tids[WORKERS];

    if (pthread_create(&listener_tid, NULL, listener_thread, &server_port) != 0) {
        perror("pthread_create listener");
        exit(1);
    }
    for (int i = 0; i < WORKERS; i++) {
        if (pthread_create(&worker_tids[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create worker");
            exit(1);
        }
    }
    char line[128];
    while (!shutdown_flag && fgets(line, sizeof(line), stdin) != NULL) {
        if (strncmp(line, "stats", 5) == 0) {
            print_stats();
        } else if (strncmp(line, "quit", 4) == 0) {
            shutdown_flag = 1;
            close(listener_sock_fd);
            queue_shutdown();
            queue_cleanup();
            db_cleanup();
            break;
        } else {
            printf("Invalid command, Supported commands - stats, quit\n");
        }
    }
    pthread_join(listener_tid, NULL);
    for (int i = 0; i < WORKERS; i++) {
        pthread_join(worker_tids[i], NULL);
    }
    
    return 0;
}