#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "binary.h"

/* Write words to a file */ 

void write_bytes(FILE *f, void *buf, uint32_t n_bytes)
{
    /* Write n_bytes to the file */
    fwrite(buf, n_bytes, 1, f);
}

void write_u8(FILE *f, uint8_t val)
{
    /* Write a uint8_t to the file */
    write_bytes(f, &val, 1);
}

void write_u16(FILE *f, uint16_t val)
{
    /* Write a uint16_t to the file */
    write_bytes(f, &val, 2);
}

void write_u32(FILE *f, uint32_t val)
{
    /* Write a uint32_t to the file */
    write_bytes(f, &val, 4);
}

void write_u64(FILE *f, uint64_t val)
{
    /* Write a uint64_t to the file */
    write_bytes(f, &val, 8);
}


/* Read words from a memory buffer */
uint8_t readbuf_u8(uint8_t **buf)
{
    /* Read a uint8_t from the buffer */
    uint8_t val = **buf;
    *buf += 1;
    return val;
}

uint16_t readbuf_u16(uint8_t **buf)
{
    /* Read a uint16_t from the buffer */
    uint16_t val = *((uint16_t*)*buf);
    *buf += 2;
    return val;
}

uint32_t readbuf_u32(uint8_t **buf)
{
    /* Read a uint32_t from the buffer */
    uint32_t val = *((uint32_t*)*buf);
    *buf += 4;
    return val;
}

uint64_t readbuf_u64(uint8_t **buf)
{
    /* Read a uint64_t from the buffer */
    uint64_t val = *((uint64_t*)*buf);
    *buf += 8;
    return val;
}

char *readbuf_string(uint8_t **buf)
{
    /* Read a zero-terminated string from the buffer (note: NOT A COPY!) */
    char *str = (char*)*buf;
    *buf += strlen(str)+1;
    return str;
}

void readbuf_bytes(uint8_t **buf, uint8_t *dest, uint32_t n_bytes)
{
    /* Read n_bytes from the buffer into dest */
    /* No zero terminator is added */
    memcpy(dest, *buf, n_bytes);
    *buf += n_bytes;
}

