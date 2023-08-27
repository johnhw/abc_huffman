#define BASE_NOTE 69 /* A440 */
#define A440 69
#define TUNE_TERMINATOR "\n"
#define STRING_TERMINATOR "\t"
#define STRING_TOKENS 1
#define NORMAL_TOKENS 0
#define MAX_TITLE 256
#define BASE_DURATION 0.25 /* 1/4 bar */

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

/* Note offset strings, for keys and chords */
note_offset *note_offsets = {
    {"C", 0},
    {"Cs", 1},
    {"Db", 1},
    {"D", 2},
    {"Ds", 3},
    {"Eb", 3},
    {"E", 4},
    {"F", 5},
    {"Fs", 6},
    {"Gb", 6},
    {"G", 7},
    {"Gs", 8},
    {"Ab", 8},
    {"A", 9},
    {"As", 10},
    {"Bb", 10},
    {"B", 11},    
    {NULL, 0},
};

/* Chord types */
chord_type *chord_types = {
    {"maj", {0, 4, 7, 11, -1}},
    {"min", {0, 3, 7, 10, -1}},
    {"dim", {0, 3, 6, 9, -1}},
    {"aug", {0, 4, 8, 12, -1}},
    {"maj7", {0, 4, 7, 11, 14}},
    {"min7", {0, 3, 7, 10, 14}},
    {"7", {0, 4, 7, 10, 14}},
    {"dim7", {0, 3, 6, 9, 12}},
    {"hdim7", {0, 3, 6, 10, 12}},
    {"minmaj7", {0, 3, 7, 11, 14}},
    {"maj6", {0, 4, 7, 9, 14}},
    {"min6", {0, 3, 7, 9, 14}},
    {"sus2", {0, 2, 7, 12, -1}},
    {"sus4", {0, 5, 7, 12, -1}},
    {"7sus4", {0, 5, 7, 10, 14}},
    {"9", {0, 4, 7, 10, 14}},
    {"maj9", {0, 4, 7, 11, 14}},
    {"min9", {0, 3, 7, 10, 14}},
    {"maj11", {0, 4, 7, 11, 14}},
    {"min11", {0, 3, 7, 10, 14}},
    {"13", {0, 4, 7, 10, 14}},
    {"maj13", {0, 4, 7, 11, 14}},
    {"min13", {0, 3, 7, 10, 14}},
    {"add9", {0, 4, 7, 14, -1}},
    {"6add9", {0, 4, 7, 9, 14}},
    {"madd9", {0, 3, 7, 14, -1}},
    {"m6add9", {0, 3, 7, 9, 14}},
    {"9sus4", {0, 5, 7, 10, 14}},
    {"7b5", {0, 4, 6, 10, 14}},
    {"7#5", {0, 4, 8, 10, 14}},
    {"7b9", {0, 4, 7, 10, 13}},
    {"7#9", {0, 4, 7, 10, 15}},
    {"7b5b9", {0, 4, 6, 10, 13}},
    {"7b5#9", {0, 4, 6, 10, 15}},
    {"7#5b9", {0, 4, 8, 10, 13}},
    {"7#5#9", {0, 4, 8, 10, 15}},
    {"11b9", {0, 4, 7, 10, 13}},
    {"11#9", {0, 4, 7, 10, 15}},
    {"13b9", {0, 4, 7, 10, 13}},
    {"7b9sus4", {0, 5, 7, 10, 13}},
    {"7#9sus4", {0, 5, 7, 10, 15}},
    {"7sus4b9", {0, 5, 7, 10, 13}},
    {"7sus4#9", {0, 5, 7, 10, 15}},
    {"7b5sus4", {0, 5, 6, 10, 14}},
    {"7#5sus4", {0, 5, 8, 10, 14}},   
    {"m7b5", {0, 3, 6, 10, 14}},
    {"m7#5", {0, 3, 8, 10, 14}},
    {"m7b5b9", {0, 3, 6, 10, 13}},
    {"m7b5#9", {0, 3, 6, 10, 15}},
    {"m7#5b9", {0, 3, 8, 10, 13}},
    {"m7#5#9", {0, 3, 8, 10, 15}},
    {"m11b9", {0, 3, 7, 10, 13}},
    {"m11#9", {0, 3, 7, 10, 15}},
    {"m13b9", {0, 3, 7, 10, 13}},
    {"m13#9", {0, 3, 7, 10, 15}},   
    {"m7b9sus4", {0, 5, 7, 10, 13}},
    {"m7#9sus4", {0, 5, 7, 10, 15}},
    {"m7sus4b9", {0, 5, 7, 10, 13}},
    {"m7sus4#9", {0, 5, 7, 10, 15}},   
    {"6", {0, 4, 7, 9, 14}},
    {"m6", {0, 3, 7, 9, 14}},
    {"6sus4", {0, 5, 7, 9, 14}},
    {"7sus4", {0, 5, 7, 10, 14}},    
    {"9sus4", {0, 5, 7, 10, 14}},
    {"9sus4b9", {0, 5, 7, 10, 13}},    
    {"maj7sus4", {0, 5, 7, 11, 14}},   
    {"m7sus4", {0, 5, 7, 10, 14}},
    {"m7sus4b9", {0, 5, 7, 10, 13}},
    {"m7sus4#9", {0, 5, 7, 10, 15}},
    {NULL, {0, 0, 0, 0, 0}},
};

/* Modes */
mode_type *mode_types = {
    {"ionian", {0, 2, 4, 5, 7, 9, 11}},
    {"dorian", {0, 2, 3, 5, 7, 9, 10}},
    {"phrygian", {0, 1, 3, 5, 7, 8, 10}},
    {"lydian", {0, 2, 4, 6, 7, 9, 11}},
    {"mixolydian", {0, 2, 4, 5, 7, 9, 10}},
    {"aeolian", {0, 2, 3, 5, 7, 8, 10}},
    {"locrian", {0, 1, 3, 5, 6, 8, 10}},
    {"major", {0, 2, 4, 5, 7, 9, 11}},
    {"minor", {0, 2, 3, 5, 7, 8, 10}},
    {"harmonic minor", {0, 2, 3, 5, 7, 8, 11}},
    {"melodic minor", {0, 2, 3, 5, 7, 9, 11}},      
    {NULL, {0, 0, 0, 0, 0, 0, 0}}, 
};