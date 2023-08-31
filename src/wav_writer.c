/* Implements a simple callback that writes a WAV file 
with plain square wave oscillators for notes */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "huffman_tunes.h"

typedef struct wav_context
{
    FILE *f;
    uint32_t sample_rate;
    uint32_t n_channels;
    uint32_t bits_per_sample;
    uint32_t n_samples;
} wav_context;

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

void write_header(wav_context *ctx)
{
    /* Write the WAV header to the file */
    fseek(ctx->f, 0, SEEK_SET);
    write_bytes(ctx->f, "RIFF", 4);
    write_u32(ctx->f, ctx->n_samples*ctx->n_channels*ctx->bits_per_sample/8+36);
    write_bytes(ctx->f, "WAVE", 4);
    write_bytes(ctx->f, "fmt ", 4);
    write_u32(ctx->f, 16); /* format chunk size */
    write_u16(ctx->f, 1); /* PCM */
    write_u16(ctx->f, ctx->n_channels); /* n channels */
    write_u32(ctx->f, ctx->sample_rate); /* sample rate */
    write_u32(ctx->f, ctx->sample_rate*ctx->n_channels*ctx->bits_per_sample/8); /* byte rate */
    write_u16(ctx->f, ctx->n_channels*ctx->bits_per_sample/8); /* block align */
    write_u16(ctx->f, ctx->bits_per_sample); /* bits per sample */
    write_bytes(ctx->f, "data", 4);
    write_u32(ctx->f, ctx->n_samples*ctx->n_channels*ctx->bits_per_sample/8); /* data size */    
}

wav_context *open_wav(char *fname, uint32_t sample_rate, uint32_t n_channels, uint32_t bits_per_sample)
{
    /* Open a WAV file for writing */
    wav_context *ctx = malloc(sizeof(wav_context));
    ctx->f = fopen(fname, "wb");
    ctx->sample_rate = sample_rate;
    ctx->n_channels = n_channels;
    ctx->bits_per_sample = bits_per_sample;
    ctx->n_samples = 0;
    write_header(ctx); /* DUMMY HEADER */
    return ctx;
}

void finalise_wav(wav_context *ctx)
{
    /* Finalise the WAV file */
    write_header(ctx); /* Real header */
    fclose(ctx->f);
    free(ctx);
}

/* Write a note to the WAV file, using a square wave oscillator.
    if rest=1, the note is silent.  */
void write_note(wav_context *wav, uint8_t note, uint32_t duration_us, uint8_t rest)
{
    uint64_t n_samples = duration_us*wav->sample_rate/100000;    
    uint32_t hz = midi_to_hz(note);
    uint32_t cycle = wav->sample_rate/hz;
    uint32_t i;
    uint8_t on = 1;
    uint16_t sample;
    

    for(i=0; i<n_samples; i++) {
        if(i%cycle==0) {
            on = !on;
        }
        
        
        sample = (on && !rest) ? 1024 : 0;
        write_u16(wav->f, sample);        
        wav->n_samples++; 
    }    
    //printf("Note %d %d %d %d\n", note, duration_us, rest, wav->n_samples);
}


void wav_callback(tune_context *ctx, uint32_t event_code)
{
    /* This is the callback that is called by the parser when it encounters an event */
    /* The context is a pointer to the parser context */
    
    wav_context *wav = (wav_context*) ctx->callback_context;
    //printf("Event %d\n", event_code);
    switch(event_code) {
        case EVENT_NOTE:            
            write_note(wav, ctx->current_note, ctx->current_duration, 0);
            break;
        case EVENT_REST:
            /* A rest has been parsed */
            /* Write a rest to the WAV file */
            write_note(wav, 0, ctx->current_duration, 1);
            break;
        case EVENT_BAR:
            write_u16(wav->f, 32767); /* Write a click to the WAV file */
            write_u16(wav->f, 0); /* Write a click to the WAV file */
            wav->n_samples++;
            
            break;
        case EVENT_TUNE_START:
            /* The tune has started */
            /* Open a WAV file for writing */
            wav = open_wav("tune.wav", 44100, 1, 16);
            ctx->callback_context = wav;
            break;
        case EVENT_TUNE_END:
            /* The tune has ended */
            /* Finalise the WAV file */
            finalise_wav(wav);
            break;
    }
}