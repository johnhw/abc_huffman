#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "huffman.h"
#include "huffman_tunes.h"
#include "music_data.h"


/* Tune related functions */

/* The current tune */
tune_context context;

/* Tune index; element 0 is count of tunes, subsequent elements are bit offsets */
uint32_t *tune_index;


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

void debug_callback(tune_context *ctx, uint32_t event_code)
{
    /* A debug callback function */
    switch(event_code) {
        case EVENT_NOTE:
            printf("Note %d, duration %d\n", ctx->current_note, ctx->current_duration);
            break;
        case EVENT_REST:
            printf("Rest, duration %d\n", ctx->current_duration);
            break;
        case EVENT_BAR:
            printf("Bar %d\n", ctx->bar_count);
            break;
        case EVENT_CHORD:
            printf("Chord %s\n", ctx->meta->chord);
            break;
        case EVENT_KEY:
            printf("Key %s\n", ctx->meta->key);
            break;
        case EVENT_TUNE_START:
            printf("Tune start\n");
            break;
        case EVENT_TUNE_END:
            printf("Tune end\n");
            break;
        default:
            printf("Unknown event code %d\n", event_code);
            break;
    }
}

tune_context *new_context()
{
    /* Create a new tune context */
    tune_context *context = malloc(sizeof(tune_context));
    context->meta = malloc(sizeof(tune_metadata));
    context->parser = malloc(sizeof(parser_context));
    context->event_callback = debug_callback;
    context->callback_context = NULL;
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
    
    context->current_duration = 1000000;
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
    if(!rest)
    {
        EVENT(context, EVENT_NOTE);        
    }
    else
    {        
        EVENT(context, EVENT_REST);
    }
}

/* Take a huffman token and append it to the current target, building
up a string. */
void string_token(tune_context *context, char *target)
{
    context->parser->token_mode = STRING_TOKENS;
    *target = '\0';
    context->parser->token_string = target;     
}


void parse_tune(huffman_buffer *h_buffer, event_callback_type callback)
{
    char *token;
    uint32_t nl = lookup_symbol_index(TUNE_TERMINATOR, h_buffer->table);
    tune_context *ctx = new_context();
    if(callback!=NULL)
        ctx->event_callback = callback;    
    else 
        ctx->event_callback = debug_callback;

    EVENT(ctx, EVENT_TUNE_START);
    while(peek_symbol(h_buffer)!=nl) {
        uint32_t symbol = read_symbol(h_buffer);
        if(symbol==INVALID_CODE) {
            printf("Error: invalid code\n");
            break;
        }
        token = h_buffer->table->entries[symbol]->token_string;
        decode_token(ctx, token);
    }
    EVENT(ctx, EVENT_TUNE_END);    
}

void decode_token(tune_context *context, char *token)
{

    char leading = token[0];
    char *p = token+1;
    char dup[MAX_TOKEN]; /* tokens are never more than MAX_TOKEN long */
    int num, den;
#ifdef DEBUG
    printf("Token `%s`, token mode %d\n", token, context->parser->token_mode);
#endif 

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
            EVENT(context, EVENT_KEY);
            break;
        case '#':
            /* Chord */
            strcpy(context->meta->chord, p);            
            EVENT(context, EVENT_CHORD);
            break;
        case '^':
            /* Bar duration */
            context->meta->bar_duration = atoi(p);
            context->current_duration = context->meta->bar_duration * BASE_DURATION;      
            EVENT(context, EVENT_BAR_DURATION);                
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
            EVENT(context, EVENT_BAR);
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
            strcpy(dup, p);
            num = atoi(strtok(dup, "/"));
            den = atoi(strtok(NULL, "/"));                       
            context->current_duration *= num;
            context->current_duration /= den;                        
            break;
        case '\n':
            EVENT(context, EVENT_TUNE_END);
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
