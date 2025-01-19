#include "caesar.h"
#include <ctype.h>
#include <stdio.h>

void encode(char *plaintext, int key) {
    for (int i = 0; plaintext[i] != '\0'; i++) {
        char ch = toupper(plaintext[i]);
        if (ch >= 'A' && ch <= 'Z') {
            plaintext[i] = ((ch - 'A' + key) % 26) + 'A';
        }
    }
}

void decode(char *ciphertext, int key) {
    for (int i = 0; ciphertext[i] != '\0'; i++) {
        char ch = toupper(ciphertext[i]);
        if (ch >= 'A' && ch <= 'Z') {
            ciphertext[i] = ((ch - 'A' - key + 26) % 26) + 'A';
        }
    }
}