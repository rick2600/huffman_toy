#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stdbool.h>
#include "bitstream.h"


typedef struct _huffman_node {
    uint32_t freq;
    uint8_t byte;
    struct _huffman_node* left;
    struct _huffman_node* right;
    char* code;
} huffman_node_t;

typedef struct {
    size_t size;
    uint8_t* data;
} huffman_cdata_t;

typedef struct {
    bool bitmap;
    uint8_t orig_size_max_bytes;
    size_t orig_size;
    uint8_t freq_max_bits;
    uint16_t nodes_count;

    bits_t* bs;
    size_t bit_index;
} huffman_header_t;

huffman_cdata_t* huffman_compress(uint8_t* data, size_t size);
uint8_t* huffman_decompress(uint8_t* cdata, size_t* size);
#endif