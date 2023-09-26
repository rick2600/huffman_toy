#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "huffman.h"
#include "bitstream.h"
#include "utils.h"


static void bits_adjust_size(bits_t* bs, size_t size) {
    bs->data = realloc(bs->data, size);
    if (bs->data == NULL) utils_fatal_error("bits_adjust() failed");
    bs->size_in_bytes = size;
}

static void bits_ensure_size(bits_t* bs, size_t bit_index) {
    size_t byte_index = bit_index / 8;
    size_t bit_offset = bit_index % 8;
    if (bs->data == NULL || bit_index >= bs->size_in_bytes * 8) {
        bs->size_in_bytes = (byte_index + 1) * 2;
        bits_adjust_size(bs, bs->size_in_bytes);
    }
}

bits_t* bits_create() {
    bits_t* bs = malloc(sizeof(bits_t));
    if (bs == NULL) utils_fatal_error("bits_create() failed");
    bs->data = NULL;
    bs->size_in_bytes = 0;
    return bs;
}

bits_t* bits_create_from_data(uint8_t* data, size_t size) {
    bits_t* bs = malloc(sizeof(bits_t));
    if (bs == NULL) utils_fatal_error("bits_create_from_data() failed");
    bs->data = data;
    bs->size_in_bytes = size;
    return bs;
}

void bits_destroy(bits_t* bs) {
    bs->size_in_bytes = 0;
    free(bs->data);
    bs->data = NULL;
    free(bs);
}

void bits_trunc(bits_t* bs, size_t size) {
    if (size < bs->size_in_bytes)
        bits_adjust_size(bs, size);
}

void bits_trunc_to_bit_index(bits_t* bs, size_t bit_index) {
    size_t new_size_in_bytes = (((bit_index + 7) / 8) * 8) / 8;
    bits_trunc(bs, new_size_in_bytes);
}

void bits_set_at(bits_t* bs, size_t bit_index) {
    if (bs == NULL) return;
    bits_ensure_size(bs, bit_index);
    size_t byte_index = bit_index / 8;
    size_t bit_offset = bit_index % 8;
    bs->data[byte_index] |= (1 << (7 - bit_offset));
}

void bits_clear_at(bits_t* bs, size_t bit_index) {
    if (bs == NULL) return;
    bits_ensure_size(bs, bit_index);
    size_t byte_index = bit_index / 8;
    size_t bit_offset = bit_index % 8;
    bs->data[byte_index] &= ~(1 << (7 - bit_offset));
}

bool bits_read_bit_at(bits_t* bs, size_t bit_index) {
    uint32_t byte_pos = bit_index / 8;
    uint32_t bit_offset =  7 - (bit_index % 8);
    return (bs->data[byte_pos] >> bit_offset) & 1;
}

uint8_t bits_read_byte_at(bits_t* bs, size_t bit_index) {
    uint8_t byte = 0;
    for (uint32_t i = 0; i < 8; i++) {
        byte |= (bits_read_bit_at(bs, bit_index+i) << (7-i));
    }
    return byte;
}

void bits_write_byte_at(bits_t* bs, size_t bit_index, uint8_t value) {
    for (uint32_t i = 0, mask = 0x80; i < 8; i++, mask >>= 1) {
        if (mask & value) bits_set_at(bs, bit_index+i);
        else              bits_clear_at(bs, bit_index+i);
    }
}

void bits_write_word_at(bits_t* bs, size_t bit_index, uint16_t value) {
    for (uint32_t i = 0, mask = 0x8000; i < 16; i++, mask >>= 1) {
        if (mask & value) bits_set_at(bs, bit_index+i);
        else              bits_clear_at(bs, bit_index+i);
    }
}

void bits_write_dword_at(bits_t* bs, size_t bit_index, uint32_t value) {
    for (uint32_t i = 0, mask = 0x80000000; i < 32; i++, mask >>= 1) {
        if (mask & value) bits_set_at(bs, bit_index+i);
        else              bits_clear_at(bs, bit_index+i);
    }
}

uint32_t bits_count_bits_set_in_range(bits_t *bs, size_t start, size_t end) {
    uint32_t count = 0;
    for (uint32_t i = start; i <= end; i++)
        if (bits_read_bit_at(bs, i)) count++;
    return count;
}