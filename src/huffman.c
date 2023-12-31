#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "huffman.h"
#include "binary.h"

/* extract the bit at bit index pos from the byte array x */
#define BIT_AT(x, pos) ((x[pos>>3]>>(pos&7))&1)


uint8_t *read_one_entry(uint8_t *buf, huffman_entry *entry)
{
    /* Read one huffman entry from buf, returning the advanced buf pointer.
    entry should already be allocated. 
    */
    uint8_t *p = buf;
    int i;
    
    entry->token_string_len = readbuf_u8(&p);
    entry->n_bits = readbuf_u8(&p);    
    entry->token_string = malloc(entry->token_string_len+1);
    readbuf_bytes(&p, entry->token_string, entry->token_string_len);    
    entry->token_string[entry->token_string_len] = '\0';

    /* Read n bits into code */
    entry->code = 0;
    for (i=0; i<entry->n_bits; i++) {
        entry->code = (entry->code<<1) | BIT_AT(p, i);
    }    
    p += (entry->n_bits+7)>>3;    
    return p;    
}

uint8_t *read_huffman_table(uint8_t *buf, huffman_table *table)
{
    /* Read a huffman table from buf, returning the advanced buf pointer. */
    /* Table should already be allocated. */
    
    uint32_t i;
    
    huffman_entry *entry;        
    /* read a uint32_t from buf */
    table->n_entries = readbuf_u32(&buf);    
    /* allocate space for the entries */
    table->entries = malloc(sizeof(huffman_entry*)*(table->n_entries));    
    /* read each entry */
    for (i=0; i<table->n_entries; i++) {
        entry = malloc(sizeof(huffman_entry));        
        buf = read_one_entry(buf, entry);
        table->entries[i] = entry;
    }
    return buf;
}

void free_huffman_table(huffman_table *table)
{
    /* Free the memory associated with a huffman table */
    uint32_t i;
    for(i=0; i<table->n_entries; i++) {
        free(table->entries[i]->token_string);
        free(table->entries[i]);
    }
    free(table->entries);
    free(table);
}

huffman_buffer *read_huffman(uint8_t *buf)     
{
    huffman_table *table;
    /* Read the tune data from buf. */
    /* Check the header begins 'ABCM' */

    if (buf[0] != 'H' || buf[1] != 'U' || buf[2] != 'F' || buf[3] != 'M') {
        printf("Error: not an HUFM file\n");
        return NULL;
    }
    buf += 4;
    table = malloc(sizeof(huffman_table));    
    buf = read_huffman_table(buf, table);

    /* Now buf points to the compressed data. 
    Create a huffman_buffer structure and return it */
    huffman_buffer *buffer = malloc(sizeof(huffman_buffer));    
    buffer->n_bits = readbuf_u32(&buf);
    buffer->table = table;
    buffer->buf = buf;
    buffer->pos = 0;
    return buffer;
}

void reset_buffer(huffman_buffer *buffer)
{
    /* Reset the buffer to the start */
    buffer->pos = 0;
}

uint32_t read_symbol(huffman_buffer *buffer)
{
    /* Read up a huffman symbol from the buffer at bit index pos. 
    Update pos to the end of the symbol, and return the index of the symbol. */
    int init_pos = buffer->pos;
    uint32_t code = 0;
    uint8_t found_code = 0; /* flag to indicate we've found a complete code */
    uint8_t b; 
    uint32_t i;
    while(!found_code)
    {
        if(buffer->pos >= buffer->n_bits) {
            buffer->pos = init_pos; /* reset the buffer to before we started */
            printf("Error: buffer overrun\n");
            return INVALID_CODE;
        }
        b = BIT_AT(buffer->buf, buffer->pos);
        code = (code<<1) | b;
        for(i=0; i<buffer->table->n_entries; i++) {
            if (buffer->table->entries[i]->n_bits == buffer->pos-init_pos+1) {
                /* Length matches */
                if (buffer->table->entries[i]->code == code) {
                    /* Code matches */
                    found_code = 1;
                    break;
                }
            }
        }       
        /* Codes are at most 32 bits long */
        if(buffer->pos-init_pos+1 > 32) {
            printf("Error: no matching code found\n");
            return INVALID_CODE;
        }         
        (buffer->pos)++;
    }    
    return i;
}

uint32_t peek_symbol(huffman_buffer *buffer)
{
    /* Peek at the next symbol in the buffer, without advancing pos. */
    int init_pos = buffer->pos;
    uint32_t symbol = read_symbol(buffer);
    buffer->pos = init_pos;
    return symbol;
}

void seek_symbol(uint32_t symbol, huffman_buffer *buffer)
{
    /* Advance pos to the position immediately following symbol.*/
    uint32_t found_symbol=INVALID_CODE;
    while(found_symbol!=symbol) {
        found_symbol = read_symbol(buffer);
    }    
}

uint32_t lookup_symbol_index(char *text, huffman_table *table)
{
    /* Find the symbol index that matches text, or 
    return INVALID_CODE if no match is found. */
    uint32_t i;
    for(i=0; i<table->n_entries; i++) {
        if(strcmp(text, table->entries[i]->token_string)==0) {
            return i;
        }
    }
    return INVALID_CODE;
}