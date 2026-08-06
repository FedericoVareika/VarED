#ifndef BASE_STRING_H
#define BASE_STRING_H

typedef struct {
    u8 *str;
    u64 size;
} String8;

internal String8 str8(u8 *str, u64 size);

#define S(str) S8(str)
#define S8(str) str8((u8 *)str, sizeof(str) - 1)

internal String8 str8_skip(String8 str, u64 n);

internal u64 cstr_len(char *cstr);
internal char *cstr_from_str8(Arena *arena, String8 str);
internal String8 str8_from_cstr(char *cstr);

////////////////////////////////////////////////////////////////////////////////
// NOTE(fede): UTF-8

typedef struct {
    u32 character;
    u32 byte_size;
} UnicodeCodepoint;

internal inline bool utf8_byte_is_header(u8 byte);
internal inline bool utf8_byte_is_continuation(u8 byte);
internal UnicodeCodepoint utf8_decode(u8 *base, u64 max_size);
internal u32 utf8_encode(u32 character, u8 *dst);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Hashing

internal u64 str8_hash_u64(String8 str);

#endif // BASE_STRING_H
