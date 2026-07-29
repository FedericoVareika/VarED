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

////////////////////////////////////////////////////////////////////////////////
// NOTE(fede): UTF-8

typedef struct {
    u32 character;
    u32 byte_size;
} UnicodeCodepoint;

internal UnicodeCodepoint utf8_decode(u8 *base, u64 max_size);
internal u32 utf8_encode(u32 character, u8 *dst);

#endif // BASE_STRING_H
