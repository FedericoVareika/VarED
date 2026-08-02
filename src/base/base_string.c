
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

////////////////////////////////////////////////////////////////////////////////
// NOTE(fede): UTF-8 / Unicode

#define UTF8_REPLACEMENT_CHARACTER 0xFFD

internal bool utf8_byte_is_continuation(u8 byte) {
    return (byte & 0x80) == 0x80;
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
#if VARED_SLOW
    u8 *original_dst = dst;
#endif // VARED_SLOW

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

        *dst = utf8_byte_class_mask[byte_class];
        *dst |= character & data_mask;
        
        character >>= 6; 
    }

#if VARED_SLOW
    assert(original_dst == dst + 1);
#endif // VARED_SLOW
       
    return bytes_to_write;
}
