#include <stdio.h>
#include "caesar.h"

int main() {
    char message[] = "HelloWorld";
    int key = 3;

    printf("Original message: %s\n", message);
    encode(message, key);
    printf("Encoded message: %s\n", message);
    decode(message, key);
    printf("Decoded message: %s\n", message);

    return 0;
}