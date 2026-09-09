#ifndef COMMANDS_H
#define COMMANDS_H

#define MACROS_H
#include "commands.macros.h"
#undef MACROS_H

typedef struct CMD CMD;
struct CMD {
    String8 name;

    String8 filepath;
    TXT_ViewNode *view_n;
    // TODO(fede): Add txt_action command
    TXT_ViewAction action;
};

typedef struct CMD_Node CMD_Node;
struct CMD_Node {
    CMD_Node *next;
    CMD_Node *prev;

    CMD v;
};

typedef struct CMD_List CMD_List;
struct CMD_List {
    CMD_Node *first;
    CMD_Node *last;
};

typedef struct CMD_Name2KindNode CMD_Name2KindNode;
struct CMD_Name2KindNode {
    CMD_Name2KindNode *next;

    u64 key;
    String8 name;
    CMD_Kind kind;
};

typedef struct CMD_Name2KindHashSlot CMD_Name2KindHashSlot;
struct CMD_Name2KindHashSlot {
    CMD_Name2KindNode *hash_first;
    CMD_Name2KindNode *hash_last;
};

typedef struct CMD_State CMD_State;
struct CMD_State {
    Arena *arena;
    Arena *frame_arena;

    CMD_Node *first_free_cmd_n;
    CMD_List cmds;

    CMD_Name2KindHashSlot *name_to_kind_table;
    u64 name_to_kind_table_size;
};

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): API

internal void cmd_init(void);
// NOTE(fede): This should be called after command consumption. 
//      Command consumption lives after event consumption, but before ui 
//      building, meaning that events immediately affect the ui, and the 
//      commands pushed by the ui are consumed before the next ui build.
internal void cmd_tick(void);

// NOTE(fede): The lifetime of this arena is between 'cmd_tick's, so that 
//      if we need to allocate something in the ui that is used for the command 
//      (e.g. some filepath string), we should use this frame arena. If we use 
//      a normal frame arena, the string is cleared before command consumption 
//      in the next frame.
internal Arena *cmd_frame_arena(void);

internal CMD *cmd_push_name(String8 name);
internal CMD_List *cmd_get_pending(void);

internal CMD_Kind cmd_kind_from_name(String8 name);

////////////////////////////////////////////////////////////////////////////////
/// NOTE(fede): Helpers

internal void cmd_add(String8 name, CMD_Kind kind);

#endif // COMMANDS_H
