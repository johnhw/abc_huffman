#ifndef _BINARY_H_
#define _BINARY_H_

void write_bytes(FILE *f, void *buf, uint32_t n_bytes);
void write_u8(FILE *f, uint8_t val);
void write_u16(FILE *f, uint16_t val);
void write_u32(FILE *f, uint32_t val);
void write_u64(FILE *f, uint64_t val);
uint8_t readbuf_u8(uint8_t **buf);
uint16_t readbuf_u16(uint8_t **buf);
uint32_t readbuf_u32(uint8_t **buf);
uint64_t readbuf_u64(uint8_t **buf);
char *readbuf_string(uint8_t **buf);
void readbuf_bytes(uint8_t **buf, uint8_t *dest, uint32_t n_bytes);

#endif