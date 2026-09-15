
internal inline bool u8_is_whitespace(u8 c) {
    // 9..13:
    // Horizontal tab, line feed, vertical tab, form feed, carriage return
    return 
        c >= 9 && c <= 13 || 
        c == ' ';
}

internal inline bool u8_is_word_delim(u8 c) {
    return u8_is_whitespace(c) || 
        c == '_' || 
        c == ',' || 
        c == '.' || 
        c == '/' || 
        c == '\\' || 
        c == '(' || 
        c == ')' || 
        c == '{' || 
        c == '}' || 
        c == '[' || 
        c == ']' || 
        c == '=';
}

internal String8 str8(u8 *str, u64 size) {
    return (String8){
        .str = str,
        .size = size,
    };
}

internal String8 str8_skip(String8 str, u64 n) {
    n = min(n, str.size);
    str.str += n;
    str.size -= n;
    return str;
}

internal String8 str8_cat(Arena *arena, String8 a, String8 b) {
    String8 result;
    result.size = a.size + b.size;
    result.str = push_size(arena, result.size);
    mem_copy(result.str, a.str, a.size);
    mem_copy(result.str + a.size, b.str, b.size);
    return result;
}

internal String8 str8_copy(Arena *arena, String8 str) {
    String8 result = {0};
    result.size = str.size;
    result.str = push_array(arena, u8, result.size);

    mem_copy(result.str, str.str, result.size);

    return result;
}

internal String8 str8_strip_left(String8 v) {
    u32 i = 0; 
    while (i < v.size && u8_is_whitespace(v.str[i])) {
        i++;
    }

    return str8(v.str + i, v.size - i);
}

internal String8 str8_strip_right(String8 v) {
    u32 i = v.size; 
    while (i > 0 && u8_is_whitespace(v.str[i - 1])) {
        i--;
    }

    return str8(v.str, i);
}

internal String8 str8_strip(String8 v) {
    return str8_strip_right(str8_strip_left(v));
}

internal String8 str8_from_u32(Arena *arena, u32 v) {
    // TODO(fede): There has to be a better way
    
    u64 size = 0;
    u32 scratch = v;
    do {
        scratch /= 10;
        size++;
    } while (scratch > 0);

    String8 result = {0};

    result.size = size;
    result.str = push_size(arena, size);

    scratch = v;
    u64 idx = size - 1;
    do {
        *(result.str + idx) = '0' + (scratch % 10);
        scratch /= 10;
        idx--;
    } while (scratch > 0);

    return result;
}

internal String8 str8_from_u64(Arena *arena, u64 v) {
    // TODO(fede): There has to be a better way    
    
    u64 size = 0;
    u32 scratch = v;
    do {
        scratch /= 10;
        size++;
    } while (scratch > 0);

    String8 result = {0};

    result.size = size;
    result.str = push_size(arena, size);

    scratch = v;
    u64 idx = size - 1;
    do {
        *(result.str + idx) = '0' + (scratch % 10);
        scratch /= 10;
        idx--;
    } while (scratch > 0);

    return result;
}

internal String8 str8_from_f32(Arena *arena, f32 v, u32 n_decimals) {
    u64 whole_size = 0;
    u32 scratch = abs_f32(v);
    do {
        scratch /= 10;
        whole_size++;
    } while (scratch > 0);

    String8 result = {0};

    result.size = whole_size + n_decimals + 1;
    if (v < 0) {
        result.size++;
    }

    result.str = push_size(arena, result.size);

    u8 *dst = result.str;
    if (v < 0) { 
        *dst = '-';
        dst++;
    }

    scratch = abs_f32(v);
    u64 idx = whole_size - 1;
    do {
        dst[idx] = '0' + (scratch % 10);
        scratch /= 10;
        idx--;
    } while (scratch > 0);

    dst[whole_size] = '.';

    dst = dst + whole_size + 1;

    f32 decimal = v;
    for (u32 i = 0; i < n_decimals; i++) {
        decimal = decimal - (f32)(i32)decimal;
        decimal *= 10;
        dst[i] = '0' + (u32)decimal;
    }

    return result;
}

// internal String8 str8_from_f64(Arena *arena, f64 v) {
// }

////////////////////////////////////////////////////////////////////////////////
// NOTE(fede): Cstr

internal u64 cstr_len(char *cstr) {
    u64 result = 0;
    while (*cstr++) {
        result++;
    }

    return result;
}

internal char *cstr_from_str8(Arena *arena, String8 str) {
    char *result = push_size(arena, str.size + 1);
    mem_copy(result, str.str, str.size);
    result[str.size] = 0;
    return result;
}

internal String8 str8_from_cstr(char *cstr) {
    u64 len = cstr_len(cstr);
    return str8((u8 *)cstr, len);
}

////////////////////////////////////////////////////////////////////////////////
// NOTE(fede): UTF-8 / Unicode

#define UTF8_REPLACEMENT_CHARACTER 0xFFD

internal inline bool utf8_byte_is_header(u8 byte) {
    return !utf8_byte_is_continuation(byte);
}

internal inline bool utf8_byte_is_continuation(u8 byte) {
    return (byte & 0xC0) == 0x80;
}

internal int utf8_byte_class_map[32] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1 byte
    0, 0, 0, 0, 0, 0, 0, 0,                         // Continuation byte
    2, 2, 2, 2,                                     // 2 bytes
    3, 3,                                           // 3 bytes
    4,                                              // 4 bytes
    -1,                                             // Error
};

// NOTE(fede): These should not be accessed by Error (-1)

internal u32 utf8_byte_class_mask[5] = {
    [0] = 0x80, // 1000_0000 -- Continuation byte
    [1] = 0x00, // 0000_0000 -- 1 byte
    [2] = 0xC0, // 1100_0000 -- 2 bytes
    [3] = 0xE0, // 1110_0000 -- 3 bytes
    [4] = 0xF0, // 1111_0000 -- 4 bytes
};

internal u32 utf8_byte_class_data_mask[5] = {
    [0] = (1 << 6) - 1, // 0011_1111 -- Continuation byte
    [1] = (1 << 7) - 1, // 0111_1111 -- 1 byte
    [2] = (1 << 5) - 1, // 0001_1111 -- 2 bytes
    [3] = (1 << 4) - 1, // 0000_1111 -- 3 bytes
    [4] = (1 << 3) - 1, // 0000_0111 -- 4 bytes
};

#define utf8_byte_class(byte) utf8_byte_class_map[byte >> 3]

internal UnicodeCodepoint utf8_decode(u8 *base, u64 max_size) {
    bool is_error = false;
    
    UnicodeCodepoint result = {0};

    u8 header_byte = *base;
    int header_byte_class = utf8_byte_class(header_byte);

    // NOTE(fede): This is so that an error is set, and character consumed
    //      if the header_byte_class < 1.
    int bytes_left = max(1, header_byte_class);

    int expected_byte_class = max(1, header_byte_class);
    while (bytes_left > 0 && max_size > 0) {
        u8 decoding_byte = *(base++);

        int byte_class = utf8_byte_class(decoding_byte);

        if (expected_byte_class != byte_class) {
            is_error = true;
            break;
        }

        // NOTE(fede): If we do this before checking the byte class, it would 
        //      include the unexpected byte in the error result, which we do 
        //      not want.
        result.byte_size++;
        bytes_left--;

        int class_mask = utf8_byte_class_mask[byte_class];

        // TODO(fede): Detect overlong characters correctly, need to STUDY.
        u32 byte_data = ((~class_mask) & decoding_byte);
        // STUDY(fede): I think continuation characters can have empty data
        if (byte_class != 0 && byte_class != 4 && byte_data == 0) 
            is_error = true;

        result.character <<= 6;
        result.character |= byte_data;

        // NOTE(fede): The next bytes should be continuation bytes.
        expected_byte_class = 0;
    }

    if (bytes_left > 0) 
        is_error = true;

    if (result.character > 0x10FFFF)
        is_error = true;

    if (is_error)
        result.character = UTF8_REPLACEMENT_CHARACTER;

    return result;
}

internal u32 utf8_encode(u32 character, u8 *dst) {
    u8 *original_dst = dst;

    u32 bytes_to_write = 0;

    if (0);
    else if (character <= 0x007F)
        bytes_to_write = 1;
    else if (0x007F < character && character <= 0x07FF)
        bytes_to_write = 2;
    else if (0x07FF < character && character <= 0xFFFF)
        bytes_to_write = 3;
    else if (0xFFFF < character && character <= 0x10FFFF)
        bytes_to_write = 4;
    else 
        bytes_to_write = 0;

    dst += bytes_to_write - 1;
    for (u32 byte_idx = 0; byte_idx < bytes_to_write; byte_idx++, dst--) {
        u32 byte_class = 0;
        if (byte_idx == bytes_to_write - 1) {
            byte_class = bytes_to_write;
        }

        u32 data_mask = utf8_byte_class_data_mask[byte_class];

        if (original_dst) {
            *dst = utf8_byte_class_mask[byte_class];
            *dst |= character & data_mask;
        }
        
        character >>= 6; 
    }

    if (original_dst) {
        assert(original_dst == dst + 1);
    }
       
    return bytes_to_write;
}

// NOTE(fede): This scan stops at newlines or end of lines (size limit).
internal i32 utf8_scan_codepoints(String8 str, u32 at, i32 delta) {
    i32 result = 0;

    i32 sign = delta / abs(delta);
    assert(sign == 1 || sign == -1);

    bool end = false;
    while (!end && delta != 0) {
        i32 delta_char = 0;
        do {
            i64 new_cursor_col = (i64)at + result + delta_char + sign;
            if (new_cursor_col < 0) {
                end = true; 
                break;
            } else if (new_cursor_col > (u32)str.size) {
                end = true; 
                break;
            }

            delta_char += sign;
        } while (!utf8_byte_is_header(str.str[at + result + delta_char]));

        // NOTE(fede): Border case where we want to go right, but the next 
        //      character is newline, then we cap the movement. 
        if (sign > 0 && str.str[at + result] == '\n') {
            end = true; 
        } else {
            result += delta_char;
        }

        delta -= sign;
    }

    return result;
}

internal i32 utf8_scan_words(String8 str, u32 at, i32 delta) {
    assert(delta != 0);
    i32 result = 0;
    i32 step = delta / abs(delta);

    bool look_ahead = step < 0;

    if (look_ahead) {

    } else {
    }

    while (delta != 0) {
        bool first = look_ahead;
        bool find_word_delim = true;

        i64 next_char_offset = -1;
        // If the next_char_offset is 0, then we have reached a string boundary. 
        while (next_char_offset != 0) {
            i64 check_at = at + result;

            next_char_offset = utf8_scan_codepoints(str, at + result, step);
            // If we are looking ahead, check ahead, but dont move ahead.
            if (look_ahead) 
                check_at += next_char_offset;

            if (check_at < 0 || (u64)check_at > str.size)
                break;

            if (find_word_delim == u8_is_word_delim(str.str[check_at])) {
                if (!find_word_delim) {
                    break;
                } else { 
                    find_word_delim = false;
                }
            }

            result += next_char_offset;
        }

        delta -= step;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Hashing

// djb2 from http://www.cse.yorku.ca/~oz/hash.html
internal u64 str8_djb2_u64_seed(String8 str, u64 seed) {
    u64 result = seed;
    for (u32 i = 0; i < str.size; i++) {
        result = (result << 5) + result + str.str[i];
    }

    return result;
}

internal u64 str8_xxh3_u64(String8 str, u64 seed) {
    XXH64_hash_t result = XXH64(str.str, str.size, (XXH64_hash_t)seed);

    return (u64)result;
}

internal u64 str8_hash_u64_seed(String8 str, u64 seed) {
    return str8_xxh3_u64(str, seed);
}

internal u64 str8_hash_u64(String8 str) {
    u64 seed = 5381;
    return str8_hash_u64_seed(str, seed);
}

