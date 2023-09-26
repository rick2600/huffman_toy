#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t* data;
    size_t size_in_bytes;
} bits_t;


bits_t* bits_create();
bits_t* bits_create_from_data(uint8_t* data, size_t size);
void bits_destroy(bits_t* bs);
void bits_trunc(bits_t* bs, size_t size);
void bits_trunc_to_bit_index(bits_t* bs, size_t bit_index);
void bits_set_at(bits_t* bs, size_t bit_index);
void bits_clear_at(bits_t* bs, size_t bit_index);
bool bits_read_bit_at(bits_t* bs, size_t bit_index);
uint8_t bits_read_byte_at(bits_t* bs, size_t bit_index);
void bits_write_byte_at(bits_t* bs, size_t bit_index, uint8_t value);
void bits_write_word_at(bits_t* bs, size_t bit_index, uint16_t value);
void bits_write_dword_at(bits_t* bs, size_t bit_index, uint32_t value);
uint32_t bits_count_bits_set_in_range(bits_t *bs, size_t start, size_t end);
#endif