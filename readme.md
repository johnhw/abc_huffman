# ABC Player

A simple player for ABC notation music on smaller or embedded devices. Requires [abclua](https://github.com/johnhw/abclua).

A Lua script initially converts ABC notation to a compact, compressed binary version of the tune. A player that can read and playback these tunes is implemented in C (and e.g. can be used on Arduino devices).

The player is limited, and supports playback of monophonic tunes only. Metadata (such as titles, or meter) can optionally be preserved in the compressed file. A whole tune book can be written into one file, and individual tunes can be selected for playback.

## Usage

To create the compressed tune format, use:

```shell
lua huf_compress_tune.lua file.abc > file.huf
```

Command line arguments:

- `--debug` dump the raw symbol stream, and the huffman table data to stdout
- `--all-text` preserve all text in the compressed file
- `--no-text` remove all text from the compressed file
- `--title/--no-title` preserve the title in the compressed file
- `--rhythm/--no-rhythm` preserve the rhythm in the compressed file
- `--meter/--no-meter` preserve the meter in the compressed file
- `--key/--no-key` preserve the key in the compressed file
- `--bars/--no-bars` preserve the bar lines in the compressed file
- `--chords/--no-chords` preserve the chords in the compressed file
- `--bare` turn off everything but the tune itself (no metadata at all)
- `--full` turn on everything (all metadata, including all text)

The compressed file can be inserted into a C program, and played back using the `play_tune` function. The function takes a pointer to the compressed data. Binary data can be inserted into a header file using `xxd -i file.huf > file.h`. 

## Internal format
The compressed file has the following structure:

    - `HUFM` [4 byte magic number]
    - n_huffman_codes:u32 [number of huffman codes]
    - [huffman table]
        - N byte len of string:u8 
        - K bit width of code:u8 
        - string:u8*N 
        - code padded to byte width:u8*floor(K/8)
    - n_bits_compressed_data:u32 [number of bits of compressed data]
    - [compressed data]
        - [huffman codes packed into bytes]

The compressed data represents an ASCII string which encodes the simplified tune representation. It consists of the following tokens (where each token, like `%4/4` or `&C`) is mapped to a single Huffman code:

    - `|` bar line
    - `/<n>/<d>` adjust current duration by a factor of n/d
    - `+<n>` increase current note by n semitones and play note
    - `-<n>` decrease current note by n semitones and play note        
    - `~` play a rest
    - `^<duration>` set the current bar length to duration microseconds (e.g. `^1000000` for 1 second)
    - `%<n>/<d>` set current meter to n/d (e.g. (`%3/4`)
    - `&<key>` set the key (e.g. `&Cmaj` or `&D`)
    - `#<chord>` set the current chord (e.g. `#Dmin7` or `#Cmaj`)
    - `*<field>` metadata text field, like `*title` or `*rhythm`
        - This is followed by tokens, which represent elements of the metadata, either one character (e.g. for titles) or a string (e.g. for rhythm)
        - The metadata is terminated by `*` token
    - `\n` (newline) end of tune

The data is terminated by two newlines, and the compressed data is zero-padded to the nearest byte.

## Example

The tune "What shall we do with the drunken sailor?" becomes:

```
 *title D r u n k e n   S a i l o r * %4\4 &emin #em ^1600000 
 +2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 | /2/1 +0 -7 +3 +4 | #dmaj -2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 | /2/1 +0 -7 +4 +3 | 
 #em +2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 | /2/1 +0 +2 +1 +2 | #bm -2 -3 #dmaj -2 -3 | #em /2/1 -2 /1/2 +0 ~ | 
 #em /2/1 +7 /3/4 +0 /1/3 +0 | /2/1 +0 -7 +3 +4 | #dmaj /2/1 -2 /3/4 +0 /1/3 +0 | /2/1 +0 -7 +4 +3 | 
 #em /2/1 +2 /3/4 +0 /1/3 +0 | /2/1 +0 +2 +1 +2 | #bm -2 -3 #dmaj -2 -3 | #em /2/1 -2 /1/2 +0 ~ | \n
 ```

 where spaces are used to show token divisions. (lines are wrapped for readability). With all but the notes stripped out, this is:
 
 ```
 +2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 /2/1 +0 -7 +3 +4 -2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 /2/1 +0 -7 +4 +3 +2 /1/2 +0 +0 /2/1 +0 /1/2 +0 +0 /2/1 +0 +2 +1 +2 -2 -3 -2 -3 /2/1 -2 /1/2 +0 ~ /2/1 +7 /3/4 +0 /1/3 +0 /2/1 +0 -7 +3 +4 /2/1 -2 /3/4 +0 /1/3 +0 /2/1 +0 -7 +4 +3 /2/1 +2 /3/4 +0 /1/3 +0 /2/1 +0 +2 +1 +2 -2 -3 -2 -3 /2/1 -2 /1/2 +0 ~ 
 ```
 
 ### Compression
 With this format, [Paul Hardy's 2023 ABC tunebook](https://pghardy.net/tunebooks/pgh_session_tunebook.abc) of 270 traditional tunes compresses to **52KB** with titles, chords, bars, tempo included and **29KB** with all metadata, bars, chords, tempo stripped out.


    
