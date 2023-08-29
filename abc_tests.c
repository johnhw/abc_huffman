#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "huffman.h"
#include "huffman_tunes.h"

int main(int argc, char **argv)
{
    /* Read a huffman table from a file, and print it out */
    FILE *fp;
    char *buf;
    huffman_buffer *h_buffer;

    printf("Version " __DATE__ " " __TIME__ "\n");
    /* Read the file */
    fp = fopen(argv[1], "rb");
    if(!fp) {
        printf("Error: could not open file %s\n", argv[1]);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = malloc(file_size);
    fread(buf, 1, file_size, fp);
    fclose(fp);

    /* Read the huffman table */
    h_buffer = read_huffman(buf);

    /* Print out the size of the table, the number of bits in the compressed data, and the table itself */
    printf("Table size: %d\n", h_buffer->table->n_entries);
    printf("Compressed data size: %d\n", h_buffer->n_bits);
    uint32_t i;
    for(i=0; i<h_buffer->table->n_entries; i++) {
         printf("%s %d %d\n", h_buffer->table->entries[i]->token_string, h_buffer->table->entries[i]->n_bits, h_buffer->table->entries[i]->code);
    }

    /* Decode all of the symbols until we reach the end of the buffer */
    while(h_buffer->pos<h_buffer->n_bits) {
        uint32_t symbol = read_symbol(h_buffer);
        if(symbol==INVALID_CODE) {
            printf("Error: invalid code\n");
            break;
        }
        printf("%s ", h_buffer->table->entries[symbol]->token_string);
    }

    
    uint32_t *index = create_tune_index(h_buffer);

    /* Print out the index */
    printf("\nIndex:\n");
    uint32_t n_tunes = *index++;
    printf("Number of tunes: %d\n", n_tunes);
    for(i=0; i<n_tunes; i++) {
        printf("%d\n", *index++);
    }

    return 0;
}
