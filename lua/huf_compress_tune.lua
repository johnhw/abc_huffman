require "abclua"

base_note = 69
base_duration = 0.25

-- Output format:
-- 'ABCM' [n_huffman_codes:u32] [huffman table] [compressed data]
-- Huffman table format:
-- [N byte width of string:u8] [K bit width of code:u8] [string:u8*N] [code padded to byte width:u8*|`K/8`|]
-- Compressed data format:
-- [huffman stream] ending with codes for "||" 
-- Compressed data
-- [*title] [ch][ch][ch]...[\n]
-- [*rhythm] [token][\n]
-- [#num\den] meter 
-- [^<bar_in_millis>]
-- [/x/y] change duration by ratio x/y
-- ([+x] | [-x] | [.]) +/- x semitones, or same as last (.)

-- [:] end of bar
-- [|] end of tune
-- ...
-- [|][|] end of tunes

tune_terminator = "\n"

function reset_state(seq, tracked_state)
    tracked_state.semitones = base_note
    tracked_state.duration = base_duration
    tracked_state.bar_length = nil
    tracked_state.current_bar_length = nil
    tracked_state.delta = 0
    tracked_state.title = nil
    tracked_state.rhythm = nil
end
    
function update_state(seq, tracked_state, note, duration, is_rest)    
    
    local new_duration = duration
    -- duration changed, so adjust it
    if tracked_state.duration ~= duration then
        --n, d = reduce(1, duration)            
        n, d = reduce(duration, tracked_state.duration)    
        if n~=1 or d~=1 then 
             table.insert(seq, '/'..n.."/"..d)                            
        end        
        tracked_state.duration = tracked_state.duration * n / d        

    end    
    if is_rest then
        table.insert(seq, "~")
    else
        local delta = note - tracked_state.semitones 
        -- otherwise, we might need to stuff in pitch shifts                
        if delta>=0 then                
            table.insert(seq, "+"..delta)                        
        elseif delta<0 then         
            table.insert(seq, ""..delta)        
        end
        tracked_state.delta = delta 
        tracked_state.semitones = note
    end        
           
end

-- using gcd reduce a fraction to lowest terms
function reduce(n, d)
    -- return the greatest common divisor of a and b
    function limited_gcd(a, b)
        local i =0 
        while a ~= 0  and i<10 do
            a,b = (b%a),a
            i = i + 1        
        end
        return b
    end
    if n==0 then return 0,1 end
    if d==0 then return 1,0 end
    if n==d then return 1,1 end    
    
    local g = limited_gcd(n, d)
    if g<0.0001 then return 1,1 end     
    return n / g, d / g
end    

function insert_string(seq, content)
    local text = content
    for i=1,#text do
        local c = text:sub(i,i)
        table.insert(seq, c)
    end
   
end

-- allow elements to included/excluded from the stream
-- note, rest are essential; timing_change is essential if
-- tempos are to be preserved
default_included_elements = {
    field_text=true,
    bar=true,
    key=true,
    chord=true,
    note=true,
    rest=true,
    meter=true,
    timing_change=true,
    rhythm=false,
    title=true,
    debug_mode=false, 
}

in_abc = nil -- input ABC file
out_huf = nil  -- output huffman file

-- `--debug` dump the raw symbol stream, and the huffman table data to stdout
-- `--all-text` preserve all text in the compressed file
-- - `--no-text` remove all text from the compressed file
-- - `--title/--no-title` preserve the title in the compressed file
-- - `--rhythm/--no-rhythm` preserve the rhythm in the compressed file
-- - `--meter/--no-meter` preserve the meter in the compressed file
-- - `--key/--no-key` preserve the key in the compressed file
-- - `--bars/--no-bars` preserve the bar lines in the compressed file
-- - `--chords/--no-chords` preserve the chords in the compressed file
-- - `--timing/--no-timing` preserve the timing changes in the compressed file
-- - `--bare` turn off everything but the tune itself (no metadata at all)
-- - `--full` turn on everything (all metadata, including all text)

function parse_command_line_args()
   -- set included_elements according to flags    
    local included_elements = default_included_elements    
    for v, k in ipairs(arg) do 
        -- capture positional arguments
        if k:sub(1,2)~='--' then 
            if in_abc==nil then 
                in_abc = k
            elseif out_huf==nil then 
                out_huf = k
            end         
        -- find '--' prefixed arguments
        elseif k:sub(1,2)=='--' then 
            -- remove '--' prefix
            k = k:sub(3)
            flag = true 
            -- remove 'no-' prefix
            if k:sub(1,3)=='no-' then 
                k = k:sub(4)
                flag = false 
            end 

            if k=='debug' then 
                included_elements.debug = flag
            elseif k=='all-text' then 
                included_elements.field_text = flag
                included_elements.title = flag
                included_elements.rhythm = flag
            elseif k=='meter' then 
                included_elements.meter = flag
            elseif k=='key' then
                included_elements.key = flag
            elseif k=='bars' then
                included_elements.bar = flag
            elseif k=='chords' then
                included_elements.chord = flag
            elseif k=='title' then
                included_elements.title = flag
            elseif k=='rhythm' then
                included_elements.rhythm = flag
            elseif k=='timing' then
                included_elements.timing_change = flag
            elseif k=='bare' then
                included_elements.field_text = false
                included_elements.title = false
                included_elements.rhythm = false
                included_elements.meter = false
                included_elements.key = false
                included_elements.bar = false
                included_elements.chord = false
                included_elements.timing_change = false
            elseif k=='text' and not flag then 
                included_elements.field_text = false
                included_elements.title = false
                included_elements.rhythm = false
            elseif k=='full' then
                included_elements.field_text = true
                included_elements.title = true
                included_elements.rhythm = true
                included_elements.meter = true
                included_elements.key = true
                included_elements.bar = true
                included_elements.chord = true
                included_elements.timing_change = true                
            end                
        end
    end     
    if in_abc == nil or out_huf==nil then 
        io.stderr:write("Usage: lua huf_compress_tune.lua [options] <abc_file> <huf_file>\n")
        io.stderr:write("Options: [--debug] [--all-text] [--no-text] [--title] [--no-title]\n")
        io.stderr:write("         [--rhythm] [--no-rhythm] [--meter] [--no-meter] [--key] [--no-key]\n")
        io.stderr:write("         [--bars] [--no-bars] [--chords] [--no-chords] [--timing] [--no-timing] [--bare] [--full]\n")
        os.exit(1)
    end
    return in_abc, out_huf, included_elements
end


function code_stream_tune(seq, tracked_state, stream)        
    for i,v in ipairs(stream) do        
        if included_elements[v.event] or (v.event=='field_text' and included_elements[v.name]) then 
            -- output finalised timing change
            if (v.event=='note' or v.event=='rest') and tracked_state.bar_length~=nil and tracked_state.bar_length~=tracked_state.current_bar_length then 
                table.insert(seq, '^'..tracked_state.bar_length)
                tracked_state.duration = base_duration -- reset to base duration
                tracked_state.current_bar_length = tracked_state.bar_length
                tracked_state.bar_length = nil                                 
            end             
            if v.event=='bar' then 
                table.insert(seq, "|")        
            elseif v.event=="field_text" then 
                if not tracked_state[v.name] and included_elements[v.name] then                 
                    table.insert(seq, '*'..v.name)                
                    if v.name=="rhythm" then 
                        table.insert(seq, v.content)
                    else
                        insert_string(seq, v.content)
                    end                    
                    table.insert(seq, '*')    -- end of text field marker
                    tracked_state[v.name] = v.content
                end
            elseif v.event=='key' then 
                table.insert(seq, '&'..v.key.root..(v.key.mode or 'maj'))
            elseif v.event=='meter' then 
            table.insert(seq, '%'..v.meter.num.."\\"..v.meter.den)                       
            elseif v.event=='chord' then 
                table.insert(seq, '#'..v.chord.root..v.chord.chord_type)                
                
            elseif v.event=='note'  then                                                
                update_state(seq, tracked_state, v.note.play_pitch , v.note.play_bars, false)                
            elseif v.event=='rest' then             
                update_state(seq, tracked_state, 0, v.note.play_bars , true)                    
            elseif v.event=='timing_change' then                       
                    tracked_state.bar_length = math.floor(v.timing.bar_length)
            
            end 
        end 
    end    
end

function bits_to_bytes(s)
    -- take a string of 0 and 1s and return 
    -- a sequence of bytes, right padded with 0s
    -- note: first bit will be the LSB of the first byte
    local result = {}
    local byte = 0
    local bit = 0
    for i=1,#s do
        local c = s:sub(i,i)
        if c=="1" then
            byte = byte + 2^bit
        end
        bit = bit + 1
        if bit==8 then
            table.insert(result, string.char(byte))
            byte = 0
            bit = 0
        end
    end
    if bit>0 then        
        table.insert(result, string.char(byte))
    end
    return table.concat(result)
end 

-- convert a binary string to an integer
function bits_to_int(s)
    local result = 0
    for i=1,#s do
        local c = s:sub(i,i)
        if c=="1" then
            result = result + 2^(#s-i)
        end
    end
    return result
end

-- output a huffman table in the format
-- bytes_str bits_code [str] [code padded to byte width]
function output_huffman_table(codes)
    local t = {}
    for str, bin_code in pairs(codes) do
        local byte_code = bits_to_bytes(bin_code)        
        table.insert(t, string.char(#str)..string.char(#bin_code)..str..byte_code)    
    end
    return table.concat(t)
end 


-- huffman compress a table of tokens, returning a string
-- of binary digits representing the compressed version
function huffman_compress_table(tab)
    local freq = {}
    local codes = {}
    local tree = {}
    local result = {}
    for i=1,#tab do
        local c = tab[i]
        freq[c] = (freq[c] or 0) + 1
    end
    -- build the tree
    for i,v in pairs(freq) do
        table.insert(tree, {v, i})
    end
    while #tree>1 do
        table.sort(tree, function(a,b) return a[1]<b[1] end)
        local a = table.remove(tree, 1)
        local b = table.remove(tree, 1)
        table.insert(tree, {a[1]+b[1], {a,b}})
    end
    -- build the codes
    local function build_codes(node, prefix)
        if type(node[2])=='string' then
            codes[node[2]] = prefix
        else
            build_codes(node[2][1], prefix.."0")
            build_codes(node[2][2], prefix.."1")
        end
    end
    build_codes(tree[1], "")
    -- encode the string
    for i=1,#tab do
        local c = tab[i]
        local code = codes[c]
        for j=1,#code do
            table.insert(result, code:sub(j,j))
        end       
    end
    return table.concat(result), codes
end


-- The main conversion routine, taking
-- an ABC file name and returing the token sequence
function abc_to_tokens(abc_file)
    local songs = parse_abc_file(abc_file)
    local seq_out = {}
    local state = {}
    for i, song in ipairs(songs) do    
        if song.metadata.title then                
            reset_state(seq_out, state)
            for j,voice in pairs(song.voices) do 
                code_stream_tune(seq_out, state, voice.stream)
            end        
            table.insert(seq_out, tune_terminator)    
        end     
    
    end
    -- double end-of-tune indicates end of all tunes
    table.insert(seq_out, tune_terminator)    
    table.insert(seq_out, tune_terminator)
    return seq_out
end

function bytes_to_hex(s)
    local t = {}
    for i=1,#s do
        table.insert(t, string.format("%02x", s:byte(i,i)))
    end
    return table.concat(t, " ")
end

function byte_uint32(n)
    -- return a 4 byte string representing the unsigned integer n
    -- in little endian order
    local output = {}
    for i=1,4 do
        table.insert(output, string.char(n%256))
        n = math.floor(n/256)
    end
    return table.concat(output)
end


function table_len(t)
    local count = 0
    for i,v in pairs(t) do
        count = count + 1
    end
    return count
end

-- main
in_abc, out_file, included_elements = parse_command_line_args()
seq_out = abc_to_tokens(in_abc)
bit_stream, codes = huffman_compress_table(seq_out)
code_count = table_len(codes)
byte_stream = bits_to_bytes(bit_stream)
stream = "HUFM".. byte_uint32(code_count) .. output_huffman_table(codes) .. byte_uint32(#bit_stream) .. byte_stream

if included_elements.debug then     
    print(table.concat(seq_out, ' '))    
    table_print(codes)
    print(#stream)    
else
    local f = io.open(out_file, "wb")
    f:write(stream)
    f:close()    
    io.stderr:write("Wrote "..#stream.." bytes to "..out_file.."\n")
end

