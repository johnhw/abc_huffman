#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "huffman_tunes.h"


void note_callback(tune_context *ctx, uint32_t event_code)
{
    /* This is the callback that is called by the parser when it encounters an event */
    /* The context is a pointer to the parser context */
    switch(event_code) {
        case EVENT_NOTE:            
            if(ctx->callback_context!=ctx->meta->title)
            {
                printf("%s\n", ctx->callback_context);
                ctx->callback_context = ctx->meta->title;
            }
            printf("%d, %d\n", ctx->current_note, (ctx->current_duration/1000));
            
            break;
        case EVENT_REST:
            printf("0, %d\n", (ctx->current_duration/1000));            
            break;        
        
    }
}