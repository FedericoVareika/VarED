#ifndef FREETYPE_FONT_PROVIDER_H
#define FREETYPE_FONT_PROVIDER_H

#undef internal
#include <ft2build.h>
#include <freetype/freetype.h>
#define internal static

typedef struct FT_FaceNode FT_FaceNode;
struct FT_FaceNode {
    FT_FaceNode *next;
    FT_Face v;
};

typedef struct FT_State FT_State;
struct FT_State {
    Arena *arena;

    FT_Library ft_lib;
    FT_FaceNode *first_free_face;
}; 

#endif // FREETYPE_FONT_PROVIDER_H
