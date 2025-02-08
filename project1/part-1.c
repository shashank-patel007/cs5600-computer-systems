/*
 * file:        part-1.c
 * description: Part 1, CS5600 Project 1, 2025 SP
 */

/* THE ONLY INCLUDE FILE */
#include "sysdefs.h"

#define BUFFER_SIZE 200

/* Function prototypes */
int read(int fd, void *ptr, int len);
int write(int fd, void *ptr, int len);
void exit(int err);
void readline(char *buffer, int len);
void print(char *input);

/* ---------- */

/* System call wrappers */
int read(int fd, void *ptr, int len) {
    return syscall(__NR_read, fd, ptr, len);
}

int write(int fd, void *ptr, int len) {
    return syscall(__NR_write, fd, ptr, len);
}

void exit(int err) {
    syscall(__NR_exit, err);
}

/* ---------- */

/* Reads one line from stdin (file descriptor 0) into a buffer */
void readline(char *buffer, int len) {
    int i = 0;
    char ch;
    long input;

    while (i < len - 1) {
        input = read(0, &ch, 1);

        if (input == -1) {
            write(1, "Error reading input\n", 19);
            break;
        }
        if (input == 0) {
            break;
        }
        if (ch == '\n') {
            break;
        }

        buffer[i] = ch;
        i++;
    }
    buffer[i] = '\0';

    while (ch != '\n' && read(0, &ch, 1) > 0)
        ;
}

/* Prints a string to stdout (file descriptor 1) */
void print(char *input) {
    int len = 0;

    while (input[len] != '\0') {
        len++;
    }

    char buffer[len];

    for (int i = 0; i < len; i++) {
        buffer[i] = input[i];
    }

    write(1, buffer, len);
}

/* ---------- */

/* Main function */
int main(void) {
    char buffer[BUFFER_SIZE];

    while (1) {
        print("Enter String: ");
        readline(buffer, BUFFER_SIZE);

        if (buffer[0] == 'q' && buffer[1] == 'u' && buffer[2] == 'i' &&
            buffer[3] == 't' && buffer[4] == '\0') {
            exit(0);
        }

        print("Output: ");
        print(buffer);
        print("\n");
    }
}
