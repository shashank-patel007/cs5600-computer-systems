#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "database.h"

#define INVALID 0
#define BUSY 1
#define VALID 2

struct db_record db_table[MAX_KEYS];

int db_write(char *name, char *data, int len);
int db_read(char *name, char *buf);
int db_delete(char *name);
int find_key(char *key);
int free_index();
int count_valid_objects();
void db_cleanup(void);

int find_key(char *key) {
    for (int i=0; i<MAX_KEYS; i++) {
        if (db_table[i].status == VALID) {
            if (strcmp(db_table[i].record_name,key) == 0) {
                return i;
            }
        }
    }
    return -1;
}

int free_index() {
    for (int i=0; i<MAX_KEYS; i++) {
        if (db_table[i].status == INVALID) {
            return i;
        } 
    }
    return -1;
}

int db_write(char *name, char *data, int len) {
    int index = find_key(name);
    if (index == -1) {
        index = free_index();
        if (index == -1) {
            return -1;
        }
    }
    db_table[index].status = BUSY;
    char filename[32];
    sprintf(filename,"/tmp/data.%d",index);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777); 
    if (fd < 0)  {
        db_table[index].status = INVALID;
        perror("file opening error");
        return -1;
    }
    int write_done = write(fd, data, len); 
    close(fd);
    if(write_done != len) {
        db_table[index].status = INVALID;
        perror("write failed: invalid length");
        return -1;
    }
    strncpy(db_table[index].record_name, name, sizeof(db_table[index].record_name));
    db_table[index].status = VALID;
    return 0;
}

int db_read(char *name, char *buf) {
    int index = find_key(name);
    if (index == -1) {
        perror("no such record");
        return -1;
    }
    char filename[32];
    sprintf(filename,"/tmp/data.%d",index);
    int fd = open(filename, O_RDONLY); 
    if (fd < 0) {
        perror("file opening error");
        return -1;
    }
    int size = read(fd, buf, 4096); 
    close(fd);
    return size;
}

int db_delete(char *name) {
    int index = find_key(name);
    if (index == -1) {
        perror("no such record");
        return -1;
    }
    if (db_table[index].status == BUSY) {
        return -1;
    }
    char filename[32];
    sprintf(filename, "/tmp/data.%d", index);
    if (unlink(filename) == 0){
        db_table[index].status = INVALID;
        return 0;
    }
    return -1;
}

int count_valid_objects() {
    int count = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
        if (db_table[i].status == VALID) {
            count++;
        }
    }
    return count;
}

void db_cleanup(void) {
    system("rm -f /tmp/data.*");
}