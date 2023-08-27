#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "huffman.h"
#define BASE_NOTE 69 /* A440 */
#define A440 69
#define TUNE_TERMINATOR "\n"
#define STRING_TERMINATOR "\t"
#define STRING_TOKENS 1
#define NORMAL_TOKENS 0

/* Tune related functions */

/* The context inside a tune */
typedef struct tune_context
{
    /* All timings in microseconds */
    uint32_t bar_duration;
    char title[256];
    char rhythm[32];
    int token_mode; /* Can be STRING_TOKENS or NORMAL_TOKENS */
    char *token_string; /* pointer to a string to write the next string tokens to */
    uint32_t current_duration; /* Note duration in microseconds */
    uint8_t current_note; /* MIDI note number */
    uint8_t meter_denominator;
    uint8_t meter_numerator;
    char key[5];
    char chord[7];
    uint32_t bar_count;
    uint32_t bar_start_time; /* the time at the start of the current bar */
    uint32_t bar_end_time; /* the time at the end of the current bar */
    uint32_t note_start_time; /* the time at the start of the current note */
    uint32_t note_end_time; /* the time at the start of the current note */
    

    uint8_t note_on; /* 1 = if not currently on */
    uint32_t time; /* the current time */

} tune_context;

/* The current tune */
tune_context context;

/* Tune index; element 0 is count of tunes, subsequent elements are bit offsets */
uint32_t *tune_index;

uint32_t midi_to_hz(uint8_t note)
{
    /* Convert a MIDI note number to a frequency in Hz */
    return (uint32_t)(440.0*pow(2, (note-A440)/12.0));
}

uint32_t *create_tune_index(huffman_buffer *buffer)
{
    /* Create a table of tune indexes (bit offsets) from the buffer. */
    /* First value in the index is the number of tunes, subsequent values are the bit offsets */

    int n_tunes = 0;    
    uint32_t nl = lookup_symbol_index(TUNE_TERMINATOR, buffer->table); 

    /* Count the number of tunes in the buffer */
    while(peek_symbol(buffer)!=nl) {
        /* Read the next tune */        
        seek_forward_one_tune(buffer);        
        n_tunes++;
    }
    
    /* Allocate space for the index */
    uint32_t *index = malloc(sizeof(uint32_t)*(n_tunes+1));
    *index++ = n_tunes;

    /* Reset the buffer */
    reset_buffer(buffer);
    while(peek_symbol(buffer)!=nl) {
        /* Read the next tune */
        *index++ = buffer->pos;
        seek_forward_one_tune(buffer);
    }
}

void seek_to_tune(uint32_t ix, uint32_t *tune_index, huffman_buffer *buffer)
{
    /* Seek to the start of the tune at index ix */    
    if(ix>=*tune_index) {
        printf("Error: tune index out of range\n");
        return;
    }
    buffer->pos = tune_index[ix+1];    
}

/* Reset the context to a new, blank tune, at the start */
void reset_context(tune_context *context)
{    
    strcpy(context->title, "Untitled");
    strcpy(context->key, "cmaj");
    strcpy(context->chord, "");
    context->bar_duration = 0;
    context->current_duration = 0;
    context->current_note = BASE_NOTE;
    context->meter_denominator = 4;
    context->meter_numerator = 4;
    context->bar_count = 0;
    context->bar_start_time = 0;
    context->bar_end_time = 0;
    context->token_mode = NORMAL_TOKENS;
    context->token_string = "";
    context->note_on = 0;
    context->time = 0;
    context->note_start_time = 0;
    context->note_end_time = 0;
}

/* Update the context to trigger notes */
void trigger_note(tune_context *context, int rest)
{
    if(!rest)
        context->note_on = 1;
    else
        context->note_on = 0;
    context->note_start_time = context->time;
    context->note_end_time = context->time + context->current_duration;
}

void string_token(tune_context *context, char *target)
{
    context->token_mode = STRING_TOKENS;
    *target = '\0';
    context->token_string = target;     
}

void decode_token(char *token, tune_context *context)
{

    char leading = token[0];
    char *p = token+1;
    
    /* In STRING_TOKENS mode, we just append the token to the target string */
    if(context->token_mode == STRING_TOKENS) {
        /* end of tokens? */
        if(*token==STRING_TERMINATOR)
            context->token_mode = NORMAL_TOKENS;
        else        
            strcat(context->token_string, token);                                    
        return;
    }

    /* otherwise we are in NORMAL_TOKENS mode */
    switch(leading) {
        case '*':
            /* Text field 
            In this case, we need to switch to string tokens
            until we find an end of string token marker.
            */        
            if(!strcmp(token, "title")) {
                   string_token(context, context->title);        
            }
            else if(!strcmp(token, "rhythm")) {
                string_token(context, context->rhythm);                        
            }            
            break;
        case '&':
            /* Key */
            strcpy(context->key, p);
            break;
        case '#':
            /* Chord */
            strcpy(context->chord, p);
            break;
        case '^':
            /* Bar duration */
            context->bar_duration = atoi(p);
            break;
        case '%':
            /* Meter */            
            context->meter_numerator = atoi(strtok(p, '/'));
            context->meter_denominator = atoi(strtok(NULL, '/'));
            break;
        case '|':
            /* Bar */
            context-> bar_count++;
            context->bar_start_time = context->time;
            context->bar_end_time = context->time + context->bar_duration;
            break;
        /* These indicate that a note should be played */
        case '.':
            /* Same note as last */
            trigger_note(context, 0);
            break;
        case '+':
            /* Note, increasing pitch */
            context->current_note += atoi(p);
            trigger_note(context, 0);
            break;
        case '-':
            /* Note, decreasing pitch */
            context->current_note -= atoi(p);
            trigger_note(context, 0);
            break;
        case '~':
            /* Rest */
            trigger_note(context, 1);
        /* Relative change in duration */
        case '/':
            context->current_duration *= atoi(strtok(p, '/'));
            context->current_duration /= atoi(strtok(NULL, '/'));
            break;
        default:
            printf("Error: unknown token type %c\n", leading);
            break;
    }
}