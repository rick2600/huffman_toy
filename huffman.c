#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "huffman.h"
#include "bitstream.h"
#include "utils.h"


static bool is_leaf(huffman_node_t * node) {
    return (node->left == NULL && node->right == NULL);
}

static huffman_node_t* huffman_create_node_for_byte(uint8_t byte, uint32_t freq) {
    huffman_node_t* node = malloc(sizeof(huffman_node_t));
    if (node == NULL) utils_fatal_error("huffman_create_node_for_byte() failed");
    node->byte = byte;
    node->freq = freq;
    node->code = NULL;
    node->left  = NULL;
    node->right = NULL;
    return node;
}

static huffman_node_t* huffman_join_nodes(huffman_node_t* a, huffman_node_t* b) {
    huffman_node_t* node = huffman_create_node_for_byte(0, a->freq + b->freq);
    if (a->freq <= b->freq) {
        node->left = a;
        node->right = b;
    } else {
        node->left = b;
        node->right = a;
    }
    return node;
}

static uint32_t huffman_histogram(uint32_t* hist, uint8_t* data, size_t size) {
    uint32_t nodes_count = 0;
    memset(hist, 0, 256 * sizeof(uint32_t));
    for (size_t i = 0; i < size; i++) {
        if (hist[data[i]] == 0) nodes_count++;
        hist[data[i]]++;
    }
    return nodes_count;
}

static void huffman_create_nodes(huffman_node_t** nodes_array, uint32_t* hist) {
    uint32_t index = 0;
    for (uint32_t i = 0; i < 256; i++) {
        if (hist[i] == 0) continue;
        nodes_array[index++] = huffman_create_node_for_byte(i, hist[i]);
    }
}

static int huffman_cmp_freq(const void *a, const void *b) {
    if (a == NULL || b == NULL) utils_fatal_error("huffman_cmp_freq() failed");
    const huffman_node_t *node1 = *(const huffman_node_t **)a;
    const huffman_node_t *node2 = *(const huffman_node_t **)b;
    if (node1 == NULL || node2 == NULL) utils_fatal_error("huffman_cmp_freq() failed 2");
    if (node1->freq < node2->freq) return -1;
    if (node1->freq > node2->freq) return 1;
    return 0;
}

static void huffman_sort_nodes(huffman_node_t** nodes_array, uint32_t nodes_count) {
    qsort(nodes_array, nodes_count, sizeof(huffman_node_t *), huffman_cmp_freq);
}

static huffman_node_t** huffman_alloc_nodes_array(uint32_t nodes_count) {
    huffman_node_t** nodes_array = malloc(nodes_count * sizeof(huffman_node_t *));
    if (nodes_array == NULL) utils_fatal_error("huffman_compress() failed");
    return nodes_array;
}

static huffman_node_t* huffman_pop_node(huffman_node_t*** nodes_array) {
    huffman_node_t* node = **nodes_array;
    (*nodes_array)++;
    return node;
}

static void huffman_push_node(huffman_node_t*** nodes_array, huffman_node_t* node) {
    (*nodes_array)--;
    **nodes_array = node;
}

static huffman_node_t* huffman_build_tree(huffman_node_t** nodes_array, uint32_t nodes_count) {
    huffman_node_t** stack = nodes_array;
    while (nodes_count > 1) {
        huffman_sort_nodes(stack, nodes_count);
        huffman_node_t* node1 = huffman_pop_node(&stack);
        huffman_node_t* node2 = huffman_pop_node(&stack);
        huffman_node_t* new_node = huffman_join_nodes(node1, node2);
        huffman_push_node(&stack, new_node);
        nodes_count--;
    }
    return *stack;
}

static void huffman_show_tree(huffman_node_t* root, uint32_t level) {
    if (root == NULL) return;
    if (is_leaf(root)) {
        printf("%*s(%c: %d - %s)\n", level*4, "", root->byte, root->freq, root->code);
    }
    else {
        printf("%*s(%c: %d)\n", level*4, "", '*', root->freq);
    }
    huffman_show_tree(root->left, level+1);
    huffman_show_tree(root->right, level+1);
}

static void huffman_destroy_tree(huffman_node_t* root) {
    if (root == NULL) return;
    huffman_destroy_tree(root->left);
    huffman_destroy_tree(root->right);
    free(root->code);
    root->code = NULL;
    free(root);
}

static void do_construct_code(huffman_node_t* root, char* path, char** codes, size_t size) {
    if (root == NULL) return;
    if (is_leaf(root)) {
        if (path[0] == 0) {
            root->code = strdup("0");
            codes[root->byte] = root->code;
            return;
        }
        path[size] = 0;
        root->code = strdup(path);
        codes[root->byte] = root->code;
    }
    path[size] = '0';
    do_construct_code(root->left, path, codes, size+1);
    path[size] = '1';
    do_construct_code(root->right, path, codes, size+1);
}

static void huffman_construct_code(huffman_node_t* root, char** codes) {
    char path[256];
    memset(path, 0, sizeof(path));
    memset(codes, 0, 256*sizeof(char *));
    do_construct_code(root, path, codes, 0);
}

static void huffman_encode_data(huffman_header_t* header, uint8_t* data, char** codes) {
    for (size_t i = 0; i < header->orig_size; i++) {
        char* code = codes[data[i]];
        size_t len = strlen(code);
        for (size_t j = 0; j < len; j++) {
            if (code[j] == '1') bits_set_at(header->bs, header->bit_index);
            else                bits_clear_at(header->bs, header->bit_index);
            header->bit_index++;
        }
    }
}

static uint32_t count_bits(uint32_t value) {
    if (value == 0) return 1;
    uint32_t mask = 0x80000000;
    uint32_t count = 1;
    while ((value & mask) == 0) mask >>= 1;
    while (value >> 1) {
        value >>= 1;
        count++;
    }
    return count;
}

static void huffman_fill_header_for_encode(huffman_header_t* header, uint32_t* hist) {
    uint32_t max = 0;
    header->nodes_count = 0;
    for (uint32_t i = 0; i < 256; i++) {
        if (hist[i] ==  0) continue;
        if (hist[i] > max) max = hist[i];
        header->nodes_count++;
    }
    header->freq_max_bits = count_bits(max);
    header->bitmap = (header->nodes_count >= 32);
    if      (header->orig_size > 0xffff) header->orig_size_max_bytes = 4;
    else if (header->orig_size > 0x00ff) header->orig_size_max_bytes = 2;
    else                                 header->orig_size_max_bytes = 1;
}

static void huffman_encode_guide(huffman_header_t* header) {
    uint8_t guide = (header->bitmap << 7) | header->orig_size_max_bytes;
    bits_write_byte_at(header->bs, header->bit_index, guide);
    header->bit_index += 8;
}

static void huffman_encode_orig_size(huffman_header_t* header) {
    switch (header->orig_size_max_bytes) {
        case 1: bits_write_byte_at(header->bs, header->bit_index, header->orig_size); break;
        case 2: bits_write_word_at(header->bs, header->bit_index, header->orig_size); break;
        case 4: bits_write_dword_at(header->bs, header->bit_index, header->orig_size); break;
    }
    header->bit_index += (8 * header->orig_size_max_bytes);
}

static void huffman_encode_freq_max_bits(huffman_header_t* header) {
    bits_write_byte_at(header->bs, header->bit_index, header->freq_max_bits);
    header->bit_index += 8;
}

static void huffman_encode_bitmap(huffman_header_t* header, uint32_t* hist) {
    for (uint32_t i = 0; i < 256; i++) {
        if (hist[i] > 0) bits_set_at(header->bs, header->bit_index);
        else             bits_clear_at(header->bs, header->bit_index);
        header->bit_index++;
    }
}

static void huffman_encode_nodes_count(huffman_header_t* header) {
    bits_write_byte_at(header->bs, header->bit_index, header->nodes_count);
    header->bit_index += 8;
}

static void huffman_encode_nodes(huffman_header_t* header, uint32_t* hist) {
    for (uint32_t i = 0; i < 256; i++) {
        if (hist[i] == 0) continue;
        bits_write_byte_at(header->bs, header->bit_index, i);
        header->bit_index += 8;
    }
}

static void huffman_encode_freqs(huffman_header_t* header, uint32_t* hist) {
    uint8_t nbits = header->freq_max_bits;
    for (uint32_t i = 0; i < 256; i++) {
        if (hist[i] == 0) continue;
        for (uint32_t j = 0, mask = 1<<(nbits-1); j < nbits; j++, mask >>= 1) {
            if (mask & hist[i]) bits_set_at(header->bs, header->bit_index);
            else                bits_clear_at(header->bs, header->bit_index);
            header->bit_index++;
        }
    }
}

static void huffman_encode_header(huffman_header_t* header, uint32_t* hist) {
    huffman_fill_header_for_encode(header, hist);
    huffman_encode_guide(header);
    huffman_encode_orig_size(header);
    huffman_encode_freq_max_bits(header);
    if (header->bitmap) {
        huffman_encode_bitmap(header, hist);
    }
    else {
        huffman_encode_nodes_count(header);
        huffman_encode_nodes(header, hist);
    }
    huffman_encode_freqs(header, hist);
    //uint8_t* x = utils_hexify(header->bs->data, 80);
    //printf("x: %s\n", x);
}

static void huffman_print_header_info(huffman_header_t* header) {
    printf("header->bitmap:              %10d\n", header->bitmap);
    printf("header->orig_size_max_bytes: %10d\n", header->orig_size_max_bytes);
    printf("header->orig_size:           %10ld\n", header->orig_size);
    printf("header->freq_max_bits:       %10d\n", header->freq_max_bits);
    printf("header->nodes_count:         %10d\n\n", header->nodes_count);
}

huffman_cdata_t* huffman_compress(uint8_t* data, size_t size) {
    uint32_t hist[256];
    char* codes[256];
    huffman_header_t header;
    memset(&header, 0, sizeof(header));
    header.orig_size = size;
    header.nodes_count = huffman_histogram(hist, data, header.orig_size);
    huffman_node_t** nodes_array = huffman_alloc_nodes_array(header.nodes_count);
    huffman_create_nodes(nodes_array, hist);
    huffman_node_t* root = huffman_build_tree(nodes_array, header.nodes_count);
    huffman_construct_code(root, codes);
    //huffman_show_tree(root, 0);
    header.bs = bits_create();
    header.bit_index = 0;
    huffman_encode_header(&header, hist);
    printf("Header on compress\n");
    huffman_print_header_info(&header);
    huffman_encode_data(&header, data, codes);
    bits_trunc_to_bit_index(header.bs, header.bit_index);
    huffman_cdata_t* cdata = malloc(sizeof(huffman_cdata_t));
    if (cdata == NULL) utils_fatal_error("huffman_compress() failed");
    cdata->size = header.bs->size_in_bytes;
    cdata->data = header.bs->data;
    header.bs->data = NULL;
    bits_destroy(header.bs);
    free(nodes_array);
    huffman_destroy_tree(root);
    return cdata;
}

static void huffman_get_header_info(huffman_header_t* header, uint8_t* cdata) {
    uint8_t guide = cdata[0];
    header->bitmap = (guide >> 7);
    header->orig_size_max_bytes = (guide & 0b111);
    switch (header->orig_size_max_bytes) {
        case 1:
            header->orig_size = cdata[1];
            break;
        case 2:
            header->orig_size = (cdata[1]<<8)|(cdata[2]);
            break;
        case 4:
            header->orig_size  = ((cdata[1]<<8)|(cdata[2])) << 16;
            header->orig_size |= ((cdata[3]<<8)|(cdata[4]));
            break;
    }
    header->freq_max_bits = cdata[1 + header->orig_size_max_bytes];
}

static void huffman_rec_hist_with_bitmap(huffman_header_t* header, uint8_t* cdata, uint32_t* hist) {
    uint8_t* bitmap_start = cdata + 1 + header->orig_size_max_bytes + 1;
    bits_t* bs_bitmap = bits_create_from_data(bitmap_start, 256/8);
    header->nodes_count = bits_count_bits_set_in_range(bs_bitmap, 0, 255);
    uint8_t* freq_start = bitmap_start + (256/8);
    bits_t* bs_freq = bits_create_from_data(freq_start, 256/8);
    for (uint32_t i = 0; i < 256; i++) {
        if (!bits_read_bit_at(bs_bitmap, i)) continue;
        // how many nodes before me?
        size_t nodes_before = bits_count_bits_set_in_range(bs_bitmap, 0, i);
        if (nodes_before > 0) nodes_before--;
        size_t bit_index = nodes_before * header->freq_max_bits;
        uint32_t freq = 0;
        for (uint32_t j = 0; j < header->freq_max_bits; j++) {
            uint32_t b = bits_read_bit_at(bs_freq, bit_index++);
            freq |= b;
            freq <<= 1;
        }
        freq >>= 1;
        hist[i] = freq;
    }
    bs_bitmap->data = NULL;
    bits_destroy(bs_bitmap);
    bs_freq->data = NULL;
    bits_destroy(bs_freq);
}

static void huffman_rec_hist(huffman_header_t* header, uint8_t* cdata, uint32_t* hist) {
    memset(hist, 0, 256 * sizeof(uint32_t));
    if (header->bitmap) {
        huffman_rec_hist_with_bitmap(header, cdata, hist);
    }
    else {
        header->nodes_count = cdata[1 + header->orig_size_max_bytes + 1];
        uint8_t* symbols_start = cdata + 1 + header->orig_size_max_bytes + 1 + 1;
        uint8_t* freq_start = symbols_start + header->nodes_count * sizeof(uint8_t);
        size_t total_bits = header->nodes_count * header->freq_max_bits;
        size_t round_up = (((total_bits + 7) / 8) * 8) / 8;
        size_t bit_index = 0;
        bits_t* bs = bits_create_from_data(freq_start, round_up);
        for (uint32_t i = 0; i < header->nodes_count; i++) {
            uint32_t freq = 0;
            for (uint32_t j = 0; j < header->freq_max_bits; j++) {
                uint32_t b = bits_read_bit_at(bs, bit_index++);
                freq |= b;
                freq <<= 1;
            }
            freq >>= 1;
            hist[symbols_start[i]] = freq;
        }
        bs->data = NULL;
        bits_destroy(bs);
    }
}

static uint8_t* huffman_decompress_data(huffman_header_t* header, huffman_node_t* root, uint8_t* cdata, size_t* write_size) {
    uint8_t* freq_start = NULL;
    if (header->bitmap) {
        uint8_t* bitmap_start = cdata + 1 + header->orig_size_max_bytes + 1;
        freq_start = bitmap_start + (256/8);
    }
    else {
        uint8_t* symbols_start = cdata + 1 + header->orig_size_max_bytes + 1 + 1;
        freq_start = symbols_start + header->nodes_count * sizeof(uint8_t);
    }
    bits_t* bs = bits_create_from_data(freq_start, header->orig_size);
    size_t bit_index = header->nodes_count * header->freq_max_bits;;
    uint8_t* data = malloc(header->orig_size);
    if (data == NULL) utils_fatal_error("huffman_decompress_data() failed");
    uint32_t decoded = 0;
    while (decoded < header->orig_size) {
        huffman_node_t* node = root;
        while (node != NULL && !is_leaf(node)) {
            if (bits_read_bit_at(bs, bit_index++))
                node = node->right;
            else
                node = node->left;
        }
        data[decoded++] = node->byte;
    }
    bs->data = NULL;
    bits_destroy(bs);
    *write_size = decoded;
    return data;
}

uint8_t* huffman_decompress(uint8_t* cdata, size_t cdata_size, size_t* write_size) {
    uint32_t hist[256];
    char* codes[256];
    huffman_header_t header;
    memset(&header, 0, sizeof(huffman_header_t));
    huffman_get_header_info(&header, cdata);
    huffman_rec_hist(&header, cdata, hist);
    printf("Header on decompress\n");
    huffman_print_header_info(&header);

    if (header.orig_size > (4 * cdata_size))
        utils_fatal_error("huffman_decompress() failed - unreal");

    huffman_node_t** nodes_array = huffman_alloc_nodes_array(header.nodes_count);
    huffman_create_nodes(nodes_array, hist);
    huffman_node_t* root = huffman_build_tree(nodes_array, header.nodes_count);
    huffman_construct_code(root, codes);
    //huffman_show_tree(root, 0);
    uint8_t* data = huffman_decompress_data(&header, root, cdata, write_size);
    free(nodes_array);
    huffman_destroy_tree(root);
    return data;
}
