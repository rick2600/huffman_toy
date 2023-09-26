#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "huffman.h"
#include "bitstream.h"
#include "utils.h"


typedef struct {
    char *input_file;
    char *output_file;
    bool encode;
    bool decode;
    bool errors;
} program_opts_t;

program_opts_t parse_opts(int argc, char** argv);

size_t get_file_size(const char* path) {
    struct stat file_stat;
    if (stat(path, &file_stat) == -1) return 0;
    return file_stat.st_size;
}

uint8_t* read_file(const char *path, size_t* size) {
    size_t file_size = get_file_size(path);
    uint8_t* data = malloc(file_size);
    if (data == NULL) utils_fatal_error("Could not allocate memory for file content");
    int fd = open(path, O_RDONLY);
    if (fd < 0) utils_fatal_error("Could not open file for reading");
    ssize_t bytes_read = read(fd, data, file_size);
    if (bytes_read < file_size) utils_fatal_error("Could not read all content");
    *size = file_size;
    close(fd);
    return data;
}

void write_file(const char *path, uint8_t* data, size_t size) {
    int fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (fd < 0) utils_fatal_error("Could not open file for writing");
    ssize_t bytes_written = write(fd, data, size);
    if (bytes_written < size) utils_fatal_error("Could not read write all content");
    close(fd);
}


void huffman_compress_file(const char* input_file, const char* output_file) {
    size_t size = 0;
    uint8_t* data = read_file(input_file, &size);
    huffman_cdata_t* cdata = huffman_compress(data, size);
    write_file(output_file, cdata->data, cdata->size);
    printf("Original   size: %ld\n", size);
    printf("Compressed size: %ld\n", cdata->size);
    if (cdata->size < size) {
        float perc = ((float)cdata->size/(float)size)*100.0;
        printf("Output file is %.02f%% of input file (%.02f%% smaller)\n", perc, 100.0-perc);
    } else {
        float perc = ((float)size/(float)cdata->size)*100.0;
        printf("Output is %.02f%% BIGGER !!!!!!!!!!!\n", 100.0-perc);
    }
    free(cdata->data);
    free(cdata);
    free(data);
}


void huffman_decompress_file(const char* input_file, const char* output_file) {
    size_t size = 0;
    size_t orig_size = 0;
    uint8_t* data = read_file(input_file, &size);
    uint8_t* orig_data = huffman_decompress(data, &orig_size);
    write_file(output_file, orig_data, orig_size);
    printf("Original   size: %ld\n", orig_size);
    printf("Compressed size: %ld\n", size);
    free(data);
    free(orig_data);
}

int main(int argc, char** argv) {
    program_opts_t opts = parse_opts(argc, argv);
    if (opts.errors == false) {
        printf("Input file:  %s\n", opts.input_file);
        printf("Output file: %s\n\n", opts.output_file);

        if (opts.encode)
            huffman_compress_file(opts.input_file, opts.output_file);
        else
            huffman_decompress_file(opts.input_file, opts.output_file);
    }
    exit(EXIT_SUCCESS);
}


program_opts_t parse_opts(int argc, char** argv) {
    program_opts_t opts;
    opts.input_file = NULL;
    opts.output_file = NULL;
    opts.encode = false;
    opts.decode = false;
    opts.errors = false;
    int opt;

    struct option long_opts[] = {
        {"encode",  no_argument,         NULL, 'e'},
        {"decode",  no_argument,         NULL, 'd'},
        {"input",   required_argument,   NULL, 'i'},
        {"output",  required_argument,   NULL, 'o'},
        {NULL,                      0,   NULL,  0}
    };

    while ((opt = getopt_long(argc, argv, "edi:o:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'e': opts.encode = true;        break;
            case 'd': opts.decode = true;        break;
            case 'i': opts.input_file = optarg;  break;
            case 'o': opts.output_file = optarg; break;
            case '?':
                fprintf(stderr, "Usage: %s [-e | -d] -i <infile> -o <outfile>\n", argv[0]);
                opts.errors = true;
                return opts;
            default:
                fprintf(stderr, "Unknown option.\n");
                opts.errors = true;
                return opts;
        }
    }

    if ((opts.encode && opts.decode) || (!opts.encode && !opts.decode)) {
        fprintf(stderr, "Use either --encode or --decode, but not both.\n");
        opts.errors = true;
        return opts;
    }

    if (opts.input_file == NULL || opts.output_file == NULL) {
        fprintf(stderr, "Both --input and --output options are required.\n");
        opts.errors = true;
        return opts;
    }
    return opts;
}