#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "huffman.h"
#include "huffman_tunes.h"
void wav_callback(tune_context *ctx, uint32_t event_code);

void dump_tune(huffman_buffer *h_buffer)
{

    uint32_t nl = lookup_symbol_index(TUNE_TERMINATOR, h_buffer->table);
    printf("Buffer position %d\n", h_buffer->pos);
    while(peek_symbol(h_buffer)!=nl) {
        uint32_t symbol = read_symbol(h_buffer);
        if(symbol==INVALID_CODE) {
            printf("Error: invalid code\n");
            break;
        }
        printf("%s ", h_buffer->table->entries[symbol]->token_string);
    }
}



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
    // while(h_buffer->pos<h_buffer->n_bits) {
    //     uint32_t symbol = read_symbol(h_buffer);
    //     if(symbol==INVALID_CODE) {
    //         printf("Error: invalid code\n");
    //         break;
    //     }
    //     printf("%s ", h_buffer->table->entries[symbol]->token_string);
    // }

    
    uint32_t *index = create_tune_index(h_buffer);
    uint32_t *p = index;
    /* Print out the index */
    printf("\nIndex:\n");
    uint32_t n_tunes = *p++;
    printf("Number of tunes: %d\n", n_tunes);
    for(i=0; i<n_tunes; i++) {
        printf("%d\n", *p++);
    }

    
    /* Test seeking to a tune */
    seek_to_tune(0, index, h_buffer);
    printf("\nSeeking to tune 0\n");
    dump_tune(h_buffer);
    printf("\nSeeking to tune 5\n");
    seek_to_tune(5, index, h_buffer);
    dump_tune(h_buffer);
    printf("\nSeeking to tune 50\n");
    seek_to_tune(50, index, h_buffer);
    dump_tune(h_buffer);
    printf("\nSeeking to last tune\n");
    seek_to_tune(index[0]-1, index, h_buffer);
    dump_tune(h_buffer);

    /* Test seeking to a tune */
    seek_to_tune(0, index, h_buffer);
    printf("\nSeeking to tune 0\n");
    //parse_tune(h_buffer, NULL);
    seek_to_tune(5, index, h_buffer);
    parse_tune(h_buffer, wav_callback);
    

    

    return 0;
}
