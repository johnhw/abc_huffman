#ifndef HUFFMAN_TUNES_H
#define HUFFMAN_TUNES_H
#include <stdint.h>
#include <stdlib.h> 
#include "huffman.h"

#define BASE_NOTE 69 /* A440 */
#define A440 69
#define TUNE_TERMINATOR "\n"
#define STRING_TERMINATOR "*"
#define STRING_TOKENS 1
#define NORMAL_TOKENS 0
#define MAX_TITLE 256
#define BASE_DURATION 0.25 /* 1/4 bar */



/* Musical data (chords, keys, modes) */

typedef struct {
    char *note_name;
    int offset;
} note_offset;

typedef struct 
{
    char *chord_type;
    int chord[16];
} chord_type;

typedef struct 
{
    char *mode_name;
    int mode[7];
} mode_type;

/* Tune data */

typedef struct tune_metadata
{
    char title[MAX_TITLE];
    char rhythm[32];
    uint8_t meter_denominator;
    uint8_t meter_numerator;
    char key[5]; 
    char chord[7];
    chord_type *chord_type;   
    uint32_t bar_duration; /* Duration of a bar in microseconds */
} tune_metadata;

typedef struct parser_context
{
    int token_mode; /* Can be STRING_TOKENS or NORMAL_TOKENS */
    char *token_string; /* pointer to a string to write the next string tokens to */        
} parser_context;

/* The context inside a tune */
typedef struct tune_context
{
    /* All timings in microseconds */    
    tune_metadata *meta;
    parser_context *parser;

    uint32_t current_duration; /* Note duration in microseconds */
    uint8_t current_note; /* MIDI note number */    
    uint8_t note_on; /* 1 if a note is currently on, 0 if not */
    uint32_t bar_count;
    uint32_t bar_start_time; /* the time at the start of the current bar */
    uint32_t bar_end_time; /* the time at the end of the current bar */
    uint32_t note_start_time; /* the time at the start of the current note */
    uint32_t note_end_time; /* the time at the end of the current note */    
    uint32_t time; /* the current time */
} tune_context;

void seek_forward_one_tune(huffman_buffer *buffer);
void reset_context(tune_context *context);
void trigger_note(tune_context *context, int rest);
void string_token(tune_context *context, char *target);
void decode_token(char *token, tune_context *context);
uint32_t *create_tune_index(huffman_buffer *buffer);


#endif