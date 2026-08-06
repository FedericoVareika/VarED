#ifndef VARED_PLATFORM_H
#define VARED_PLATFORM_H

/*
 * NOTE(fede): These are platform services that are to be called from the
 * game layer.
 * */

#if VARED_INTERNAL

// TODO(fede): Move to another file, separate platform 
typedef struct {
    u64 size;
    void *memory;
} DebugReadFileResult;

internal DebugReadFileResult debug_platform_read_entire_file(ThreadContext *thread, char *filename);
internal void debug_platform_free_file_memory(ThreadContext *thread, DebugReadFileResult file_result);
internal bool debug_platform_write_entire_file(ThreadContext *thread, char *filename, u64 size, void *memory);

#endif // VARED_INTERNAL

typedef u32 WMModifiers;
enum {
    WMModifier_ctrl =   (1 << 0),
    WMModifier_shift =  (1 << 1),
    WMModifier_alt =    (1 << 2),
    WMModifier_caps =   (1 << 3),
};

// NOTE(fede): From SDL's SDL_KeyCode
typedef u32 WMKey;
enum {
    WMKey_NONE = 0,

    WMKey_RETURN = '\r',
    WMKey_ESCAPE = '\x1B',
    WMKey_BACKSPACE = '\b',
    WMKey_TAB = '\t',
    WMKey_SPACE = ' ',
    WMKey_EXCLAIM = '!',
    WMKey_QUOTEDBL = '"',
    WMKey_HASH = '#',
    WMKey_PERCENT = '%',
    WMKey_DOLLAR = '$',
    WMKey_AMPERSAND = '&',
    WMKey_QUOTE = '\'',
    WMKey_LEFTPAREN = '(',
    WMKey_RIGHTPAREN = ')',
    WMKey_ASTERISK = '*',

    // TODO(fede): Remove, i think this never happens. 
    //      Check if the same occurs on others.
    WMKey_PLUS = '+',

    WMKey_COMMA = ',',
    WMKey_MINUS = '-',
    WMKey_PERIOD = '.',
    WMKey_SLASH = '/',
    WMKey_0 = '0',
    WMKey_1 = '1',
    WMKey_2 = '2',
    WMKey_3 = '3',
    WMKey_4 = '4',
    WMKey_5 = '5',
    WMKey_6 = '6',
    WMKey_7 = '7',
    WMKey_8 = '8',
    WMKey_9 = '9',
    WMKey_COLON = ':',
    WMKey_SEMICOLON = ';',
    WMKey_LESS = '<',
    WMKey_EQUALS = '=',
    WMKey_GREATER = '>',
    WMKey_QUESTION = '?',
    WMKey_AT = '@',

    WMKey_LEFTBRACKET = '[',
    WMKey_BACKSLASH = '\\',
    WMKey_RIGHTBRACKET = ']',
    WMKey_CARET = '^',
    WMKey_UNDERSCORE = '_',
    WMKey_BACKQUOTE = '`',
    WMKey_a = 'a',
    WMKey_b = 'b',
    WMKey_c = 'c',
    WMKey_d = 'd',
    WMKey_e = 'e',
    WMKey_f = 'f',
    WMKey_g = 'g',
    WMKey_h = 'h',
    WMKey_i = 'i',
    WMKey_j = 'j',
    WMKey_k = 'k',
    WMKey_l = 'l',
    WMKey_m = 'm',
    WMKey_n = 'n',
    WMKey_o = 'o',
    WMKey_p = 'p',
    WMKey_q = 'q',
    WMKey_r = 'r',
    WMKey_s = 's',
    WMKey_t = 't',
    WMKey_u = 'u',
    WMKey_v = 'v',
    WMKey_w = 'w',
    WMKey_x = 'x',
    WMKey_y = 'y',
    WMKey_z = 'z',

    WMKey_CAPSLOCK,

    WMKey_F1,
    WMKey_F2,
    WMKey_F3,
    WMKey_F4,
    WMKey_F5,
    WMKey_F6,
    WMKey_F7,
    WMKey_F8,
    WMKey_F9,
    WMKey_F10,
    WMKey_F11,
    WMKey_F12,

    // WMKey_PRINTSCREEN,
    // WMKey_SCROLLLOCK,
    // WMKey_PAUSE,
    // WMKey_INSERT,
    // WMKey_HOME,
    // WMKey_PAGEUP,
    // WMKey_DELETE = '\x7F',
    // WMKey_END,
    // WMKey_PAGEDOWN,
    WMKey_RIGHT,
    WMKey_LEFT,
    WMKey_DOWN,
    WMKey_UP,

    WMKey_MOUSE0,
    WMKey_MOUSE1,
    WMKey_MOUSE2,
    WMKey_MOUSE3,
    WMKey_MOUSE4,
    WMKey_MOUSE5,
    WMKey_MOUSE6,

    WMKey_MOUSELEFT = WMKey_MOUSE0,
    WMKey_MOUSERIGHT = WMKey_MOUSE1,
    WMKey_MOUSEMIDDLE = WMKey_MOUSE2,

    WMKey_COUNT,
};

typedef enum {
    WMEventKind_Press, 
    WMEventKind_Release,
    WMEventKind_Text,
    WMEventKind_MouseMove,
} WMEventKind;

typedef struct {
    WMEventKind kind;
    WMModifiers modifiers;

    WMKey key;

    u32 character;
    u32 repeat;

    // TODO(fede): half_transition_count?
    v2 pos;
} WMEvent;

typedef struct WMEventNode WMEventNode;
struct WMEventNode {
    WMEventNode *next; 
    WMEvent v; 
};

typedef struct {
    WMEventNode *first;
    WMEventNode *last;
    u32 count;
} WMEventList;

typedef struct {
    WMEventList *events;
    String8 *clipboard;

} Input;

typedef struct {
    void **memory;
    WMEventList *events;
} EditorParams;

void editor_init(EditorParams *params);
void editor_update_and_render(EditorParams *params);

#endif // VARED_PLATFORM_H

