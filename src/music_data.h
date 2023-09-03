#ifndef _MUSIC_DATA_H_
#define _MUSIC_DATA_H_


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

// note_offset *note_offsets;
uint32_t midi_to_hz(uint8_t note);
// chord_type *chord_types;
// mode_type *modes;

#endif