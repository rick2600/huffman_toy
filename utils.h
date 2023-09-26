#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void utils_fatal_error(const char* msg);
uint8_t* utils_hexify(const uint8_t* data, size_t size);

#endif