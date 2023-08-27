# ABC Player

A simple player for ABC notation music on smaller or embedded devices. Requires [abclua](https://github.com/johnhw/abclua).

A Lua script initially converts ABC notation to a compact, compressed binary version of the tune. A player that can read and playback these tunes is implemented in C (and e.g. can be used on Arduino devices).

The player is limited, and supports playback of monophonic tunes only. Metadata (such as titles, or meter) can optionally be preserved in the compressed file.

