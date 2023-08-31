#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>

/* Holds one entry in the huffman table */
typedef struct huffman_entry {
    uint8_t n_bits;
    uint32_t code;
    char *token_string;
    uint8_t token_string_len;
} huffman_entry;

/* An entire table of huffman entries */
typedef struct huffman_table
{
    huffman_entry **entries;
    uint32_t n_entries;    
} huffman_table;


/* A pointer to a buffer, and the current bit index */
typedef struct huffman_buffer
{
    huffman_table *table;
    uint32_t pos;
    char *buf;
    uint32_t n_bits;
} huffman_buffer;

/*
    Huffman table format:
    -- 'ABCM' [n_huffman_codes:u32] [huffman table] [n_bits_compressed_data:u32] [compressed data]    
    Huffman table:
        [N byte len of string:u8] [K bit width of code:u8] [string:u8*N] [code padded to byte width:u8*|`K/8`|]
*/

char *read_one_entry(char *buf, huffman_entry *entry);
char *read_huffman_table(char *buf, huffman_table *table);
huffman_buffer *read_huffman(char *buf);
void reset_buffer(huffman_buffer *buffer);
uint32_t read_symbol(huffman_buffer *buffer);
uint32_t peek_symbol(huffman_buffer *buffer);
void seek_symbol(uint32_t symbol, huffman_buffer *buffer);
uint32_t lookup_symbol_index(char *text, huffman_table *table);
void free_huffman_table(huffman_table *table);

#define INVALID_CODE 0xFFFFFFFF

#endif