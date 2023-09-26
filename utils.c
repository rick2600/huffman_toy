#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "utils.h"


void utils_fatal_error(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

uint8_t* utils_hexify(const uint8_t* data, size_t size) {
    uint8_t* hex = malloc(size * 2 + 1);
    if (hex == NULL) utils_fatal_error("utils_hexify() failed");
    for (size_t i = 0; i < size; i++) {
        sprintf(hex + i * 2, "%02x", data[i]);
    }
    hex[size * 2] = 0;
    return hex;
}
#endif
