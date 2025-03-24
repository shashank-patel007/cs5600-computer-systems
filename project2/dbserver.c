#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include "proj2.h"  // contains the struct request definition

#define MAX_KEYS 200
#define MAX_VALUE_SIZE 4096

// Database entry states
#define STATE_INVALID 0
#define STATE_BUSY    1
#define STATE_VALID   2

typedef struct {
    char key[31];   // key (max 30 characters, null-padded)
    int state;      // STATE_INVALID, STATE_BUSY, STATE_VALID
} db_entry;

db_entry db_table[MAX_KEYS];
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

// Statistics counters
int stat_reads = 0;
int stat_writes = 0;
int stat_deletes = 0;
int stat_failed = 0;
int stat_objects = 0; // current number of valid keys
pthread_mutex_t stat_mutex = PTHREAD_MUTEX_INITIALIZER;

// Work queue item
typedef struct work_item {
    int sock_fd;
    struct work_item *next;
} work_item;

work_item *queue_head = NULL;
work_item *queue_tail = NULL;
int queued_requests = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

int shutdown_flag = 0;
int listen_fd = -1;

// Forward declarations
void enqueue_work(int sock_fd);
int dequeue_work();
void* listener_thread(void *arg);
void* worker_thread(void *arg);
void handle_work(int sock_fd);
void print_stats(void);
void cleanup(void);

// ----- Database helper functions -----

// Search for a key; return its index or -1 if not found.
int find_entry(const char* key) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if ((db_table[i].state == STATE_VALID || db_table[i].state == STATE_BUSY) &&
            strncmp(db_table[i].key, key, 30) == 0) {
            return i;
        }
    }
    return -1;
}

// Find a free (invalid) entry; return its index or -1 if full.
int find_free_entry(void) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (db_table[i].state == STATE_INVALID)
            return i;
    }
    return -1;
}

// Format a filename based on entry index.
void get_filename(int index, char* filename, size_t size) {
    snprintf(filename, size, "/tmp/data.%d", index);
}

// ----- Work queue functions -----

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

// ----- Listener thread -----

void* listener_thread(void *arg) {
    int port = *(int*)arg;
    struct sockaddr_in addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(listen_fd, 2) < 0) {
        perror("listen");
        exit(1);
    }
    
    while (!shutdown_flag) {
        int sock_fd = accept(listen_fd, NULL, NULL);
        if (sock_fd < 0) {
            if (shutdown_flag)
                break;
            perror("accept");
            continue;
        }
        enqueue_work(sock_fd);
    }
    return NULL;
}

// ----- Worker thread -----

void* worker_thread(void *arg) {
    (void)arg;  // unused
    while (1) {
        int sock_fd = dequeue_work();
        if (sock_fd == -1)
            break;
        handle_work(sock_fd);
    }
    return NULL;
}

// ----- Request handling -----

void handle_work(int sock_fd) {
    struct request req;
    ssize_t bytes = read(sock_fd, &req, sizeof(req));
    if (bytes != sizeof(req)) {
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_failed++;
        pthread_mutex_unlock(&stat_mutex);
        return;
    }
    
    // Check for network "quit" command: if op is 'Q', exit immediately.
    if (req.op_status == 'Q') {
        close(sock_fd);
        shutdown_flag = 1;
        close(listen_fd);
        pthread_cond_broadcast(&queue_cond);
        exit(0);
    }
    
    // Extract key from the request header.
    char key[31];
    strncpy(key, req.name, 30);
    key[30] = '\0';
    
    // Convert len field to integer.
    int data_len = atoi(req.len);
    
    if (req.op_status == 'W') {
        // ----- WRITE -----
        if (data_len > MAX_VALUE_SIZE) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        char buffer[MAX_VALUE_SIZE];
        int total = 0;
        ssize_t r;
        while (total < data_len) {
            r = read(sock_fd, buffer + total, data_len - total);
            if (r <= 0) break;
            total += r;
        }
        if (total != data_len) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        
        // Look up the key or allocate a new entry.
        pthread_mutex_lock(&db_mutex);
        int idx = find_entry(key);
        if (idx != -1) {
            if (db_table[idx].state == STATE_BUSY) {
                pthread_mutex_unlock(&db_mutex);
                req.op_status = 'X';
                write(sock_fd, &req, sizeof(req));
                close(sock_fd);
                pthread_mutex_lock(&stat_mutex);
                stat_failed++;
                pthread_mutex_unlock(&stat_mutex);
                return;
            }
            db_table[idx].state = STATE_BUSY; // mark for update
        } else {
            idx = find_free_entry();
            if (idx == -1) {
                pthread_mutex_unlock(&db_mutex);
                req.op_status = 'X';
                write(sock_fd, &req, sizeof(req));
                close(sock_fd);
                pthread_mutex_lock(&stat_mutex);
                stat_failed++;
                pthread_mutex_unlock(&stat_mutex);
                return;
            }
            db_table[idx].state = STATE_BUSY;
            strncpy(db_table[idx].key, key, 30);
            db_table[idx].key[30] = '\0';
            stat_objects++;
        }
        pthread_mutex_unlock(&db_mutex);
        
        // Sleep randomly between 0 and 10 ms to trigger race conditions if any.
        usleep(random() % 10000);
        
        // Write the data to file.
        char filename[64];
        get_filename(idx, filename, sizeof(filename));
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&db_mutex);
            db_table[idx].state = STATE_VALID;  // revert state
            pthread_mutex_unlock(&db_mutex);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        if (write(fd, buffer, data_len) != data_len) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(fd);
            close(sock_fd);
            pthread_mutex_lock(&db_mutex);
            db_table[idx].state = STATE_VALID;
            pthread_mutex_unlock(&db_mutex);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        close(fd);
        
        // Send success response.
        req.op_status = 'K';
        write(sock_fd, &req, sizeof(req));
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_writes++;
        pthread_mutex_unlock(&stat_mutex);
        
        // Mark entry valid.
        pthread_mutex_lock(&db_mutex);
        db_table[idx].state = STATE_VALID;
        pthread_mutex_unlock(&db_mutex);
    }
    else if (req.op_status == 'R') {
        // ----- READ -----
        pthread_mutex_lock(&db_mutex);
        int idx = find_entry(key);
        if (idx == -1 || db_table[idx].state == STATE_BUSY) {
            pthread_mutex_unlock(&db_mutex);
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        pthread_mutex_unlock(&db_mutex);
        
        char filename[64];
        get_filename(idx, filename, sizeof(filename));
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        char buffer[MAX_VALUE_SIZE];
        int n = read(fd, buffer, MAX_VALUE_SIZE);
        close(fd);
        if (n < 0) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        req.op_status = 'K';
        snprintf(req.len, sizeof(req.len), "%d", n);
        if (write(sock_fd, &req, sizeof(req)) != sizeof(req)) {
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        if (write(sock_fd, buffer, n) != n) {
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_reads++;
        pthread_mutex_unlock(&stat_mutex);
    }
    else if (req.op_status == 'D') {
        // ----- DELETE -----
        pthread_mutex_lock(&db_mutex);
        int idx = find_entry(key);
        if (idx == -1 || db_table[idx].state == STATE_BUSY) {
            pthread_mutex_unlock(&db_mutex);
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        db_table[idx].state = STATE_BUSY;
        pthread_mutex_unlock(&db_mutex);
        
        char filename[64];
        get_filename(idx, filename, sizeof(filename));
        if (unlink(filename) < 0) {
            req.op_status = 'X';
            write(sock_fd, &req, sizeof(req));
            close(sock_fd);
            pthread_mutex_lock(&db_mutex);
            db_table[idx].state = STATE_VALID;
            pthread_mutex_unlock(&db_mutex);
            pthread_mutex_lock(&stat_mutex);
            stat_failed++;
            pthread_mutex_unlock(&stat_mutex);
            return;
        }
        req.op_status = 'K';
        write(sock_fd, &req, sizeof(req));
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_deletes++;
        stat_objects--;
        pthread_mutex_unlock(&stat_mutex);
        
        pthread_mutex_lock(&db_mutex);
        db_table[idx].state = STATE_INVALID;
        pthread_mutex_unlock(&db_mutex);
    }
    else {
        // Unknown operation: reply failure.
        req.op_status = 'X';
        write(sock_fd, &req, sizeof(req));
        close(sock_fd);
        pthread_mutex_lock(&stat_mutex);
        stat_failed++;
        pthread_mutex_unlock(&stat_mutex);
    }
}

// ----- Statistics and cleanup -----

void print_stats(void) {
    pthread_mutex_lock(&stat_mutex);
    printf("Database objects: %d\n", stat_objects);
    printf("Read requests: %d\n", stat_reads);
    printf("Write requests: %d\n", stat_writes);
    printf("Delete requests: %d\n", stat_deletes);
    printf("Failed requests: %d\n", stat_failed);
    pthread_mutex_unlock(&stat_mutex);
    
    pthread_mutex_lock(&queue_mutex);
    printf("Requests in queue: %d\n", queued_requests);
    pthread_mutex_unlock(&queue_mutex);
}

void cleanup(void) {
    // Remove all data files.
    for (int i = 0; i < MAX_KEYS; i++) {
        if (db_table[i].state == STATE_VALID || db_table[i].state == STATE_BUSY) {
            char filename[64];
            get_filename(i, filename, sizeof(filename));
            unlink(filename);
        }
    }
}

// ----- Main function -----

int main(int argc, char *argv[]) {
    int port = 5000;
    int num_workers = 4;
    pthread_t listener_tid;
    pthread_t workers[num_workers];

    // Spawn the listener thread.
    if (pthread_create(&listener_tid, NULL, listener_thread, &port) != 0) {
        perror("pthread_create listener");
        exit(1);
    }
    // Spawn worker threads.
    for (int i = 0; i < num_workers; i++) {
        if (pthread_create(&workers[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create worker");
            exit(1);
        }
    }
    
    // Main thread: listen for console commands.
    char line[128];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strncmp(line, "stats", 5) == 0) {
            print_stats();
        } else if (strncmp(line, "quit", 4) == 0) {
            shutdown_flag = 1;
            close(listen_fd);
            pthread_cond_broadcast(&queue_cond);
            break;
        }
    }
    
    // Join threads.
    pthread_join(listener_tid, NULL);
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }
    
    cleanup();
    return 0;
}