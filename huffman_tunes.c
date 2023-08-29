#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "huffman.h"
#include "huffman_tunes.h"


/* Tune related functions */

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
    reset_buffer(buffer);
    

    /* Count the number of tunes in the buffer */
    while(peek_symbol(buffer)!=nl) {
        /* Read the next tune */        
        seek_forward_one_tune(buffer);        
        n_tunes++;
    }
    
    /* Allocate space for the index */
    uint32_t *index = malloc(sizeof(uint32_t)*(n_tunes+1));
    uint32_t *p = index;
    *p++ = n_tunes;

    /* Reset the buffer */
    reset_buffer(buffer);
    
    while(peek_symbol(buffer)!=nl) {
        /* Read the next tune */
        *p++ = buffer->pos;
        seek_forward_one_tune(buffer);
    }
    return index;
}

void seek_forward_one_tune(huffman_buffer *buffer)
{
    /* Seek forward one tune in the buffer */
    uint32_t nl = lookup_symbol_index(TUNE_TERMINATOR, buffer->table); 
    while(peek_symbol(buffer)!=nl) {
        read_symbol(buffer);
    }
    read_symbol(buffer);
}

void seek_to_tune(uint32_t ix, uint32_t *tune_index, huffman_buffer *buffer)
{
    /* Seek to the start of the tune at index ix */    
    if(ix>=tune_index[0]) {
        printf("Error: tune index out of range\n");
        return;
    }
    buffer->pos = tune_index[ix+1];    
}

tune_context *new_context()
{
    /* Create a new tune context */
    tune_context *context = malloc(sizeof(tune_context));
    context->meta = malloc(sizeof(tune_metadata));
    context->parser = malloc(sizeof(parser_context));
    /* Allocate space for the chord type and full chord name */
    context->meta->chord_type = malloc(sizeof(chord_type));    
    reset_context(context);
    return context;
}

void free_context(tune_context *context)
{
    /* Free the memory associated with a tune context */    
    free(context->meta->chord_type);
    free(context->meta);
    free(context->parser);
    free(context);
}

/* Reset the context to a new, blank tune, at the start */
void reset_context(tune_context *context)
{    
    strcpy(context->meta->title, "Untitled");
    strcpy(context->meta->key, "cmaj");
    strcpy(context->meta->chord, "");
    context->meta->meter_denominator = 4;
    context->meta->meter_numerator = 4;
    context->meta->bar_duration = 0;
    context->parser->token_mode = NORMAL_TOKENS;
    context->parser->token_string = "";
    
    context->current_duration = 0;
    context->current_note = BASE_NOTE;
    context->bar_count = 0;
    context->bar_start_time = 0;
    context->bar_end_time = 0;
    context->time = 0;
    context->note_start_time = 0;
    context->note_end_time = 0;
    context->note_on = 0;
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

/* Take a huffman token and append it to the current target, building
up a string. */
void string_token(tune_context *context, char *target)
{
    context->parser->token_mode = STRING_TOKENS;
    *target = '\0';
    context->parser->token_string = target;     
}

void decode_token(tune_context *context, char *token)
{

    char leading = token[0];
    char *p = token+1;
    char dup[16];
    
    printf("Token `%s`, token mode %d\n", token, context->parser->token_mode);

    /* In STRING_TOKENS mode, we just append the token to the target string */
    if(context->parser->token_mode == STRING_TOKENS) {
        /* end of tokens? */
        if(!strcmp(token,STRING_TERMINATOR))
            context->parser->token_mode = NORMAL_TOKENS;
        else        
            strcat(context->parser->token_string, token);                                    
        return;
    }

    /* otherwise we are in NORMAL_TOKENS mode */
    switch(leading) {
        /* Metadata fields.
        These tokens update context->meta.
        */
        case '*':
            /* Text field 
            In this case, we need to switch to string tokens
            until we find an end of string token marker.
            */        
            if(!strcmp(p, "title")) {
                   string_token(context, context->meta->title);        
            }
            else if(!strcmp(p, "rhythm")) {
                string_token(context, context->meta->rhythm);                        
            }            
            break;
        case '&':
            /* Key */
            strcpy(context->meta->key, p);
            break;
        case '#':
            /* Chord */
            strcpy(context->meta->chord, p);
            break;
        case '^':
            /* Bar duration */
            context->meta->bar_duration = atoi(p);
            context->current_duration = context->meta->bar_duration * BASE_DURATION;            
            break;
        case '%':
            /* Meter */            
            strcpy(dup, p);
            context->meta->meter_numerator = atoi(strtok(dup, "/"));
            context->meta->meter_denominator = atoi(strtok(NULL, "/"));
            
            break;
        case '|':
            /* Bar */
            context->bar_count++;
            context->bar_start_time = context->time;
            context->bar_end_time = context->time + context->meta->bar_duration;
            break;        
        /* These indicate that a note should be played */        
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
            break;
        /* Relative change in duration */
        case '/':
            int num, den;
            strcpy(dup, p);
            num = atoi(strtok(dup, "/"));
            den = atoi(strtok(NULL, "/"));                       
            context->current_duration = (num/den) * BASE_DURATION;
            break;
        default:
            printf("Error: unknown token type %c\n", leading);
            break;
    }
}

// void *decode_chord(char *name, chord_type *out)
// {
//     /* Chords are in the form <base><type>, where
//     base is a note name (like A, B, Ds, Cb) and
//     type is a type name (like maj, min, 7)
//     For example, Amaj or Dsmin7. Write the full 
//     chord data to out, where each offset is a note increment
//     from C. -1 means no note to be played.   
//     */
//     /* Search the note names for the base */
//     note_offset *note = note_offsets;
//     chord_type *chord = chord_types;
//     int i;
//     while(note->note_name) {
//         if(!strncmp(note->note_name, name, strlen(note->note_name))) {
//             /* Found the base note */
//             break;
//         }
//         note++;
//     }
//     if(note->note_name == NULL) {
//         printf("Error: unknown note name %s\n", name);
//         return NULL;
//     }
//     /* Search the chord types for the type */
    
//     while(chord->chord_type) {
//         if(!strncmp(chord->chord_type, name+strlen(note->note_name), strlen(chord->chord_type))) {
//             /* Found the chord type */
//             break;
//         }
//         chord++;
//     }
//     if(chord->chord_type == NULL) {
//         printf("Error: unknown chord type %s\n", name);
//         return NULL;
//     }
//     /* Write new chord to out */
//     strcpy(out->chord_type, name);    
//     for(i=0; i<5; i++) {
//         if(chord->chord[i]==-1)
//             break;
//         out->chord[i] = note->offset + chord->chord[i];
//     }
// }
