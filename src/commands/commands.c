global CMD_State *cmd_state = 0;

internal void cmd_init(void) {
    Arena *arena = arena_alloc();
    cmd_state = push_struct(arena, CMD_State);
    cmd_state->arena = arena;

    cmd_state->frame_arena = arena_alloc();

    cmd_state->name_to_kind_table_size = 100;
    cmd_state->name_to_kind_table = 
        push_array(arena, CMD_Name2KindHashSlot, cmd_state->name_to_kind_table_size);

    GENERATE_COMMANDS_ADD();
}

internal void cmd_tick(void) {
    CMD_Node *first = cmd_state->cmds.first;
    CMD_Node *last = cmd_state->cmds.last;

    cmd_state->cmds = (CMD_List){0};

    if (last) {
        last->next = cmd_state->first_free_cmd_n;
    }

    cmd_state->first_free_cmd_n = first;

    arena_clear(cmd_state->frame_arena);
}

internal Arena *cmd_frame_arena(void) {
    return cmd_state->frame_arena;
}

internal CMD *cmd_push_name(String8 name) {
    CMD_Node *cmd_n;
    if (cmd_state->first_free_cmd_n) {
        cmd_n = cmd_state->first_free_cmd_n;
        cmd_state->first_free_cmd_n = cmd_state->first_free_cmd_n->next;
    } else {
        cmd_n = push_struct(cmd_state->arena, CMD_Node);
    }

    DLL_PushBack(cmd_state->cmds.first, cmd_state->cmds.last, cmd_n);

    CMD *cmd = &cmd_n->v;
    *cmd = (CMD){0};
    cmd->name = name;

    return cmd;
}

internal CMD_List *cmd_get_pending(void) {
    return &cmd_state->cmds;
}

internal CMD_Kind cmd_kind_from_name(String8 name) {
    u64 key = str8_hash_u64(name);

    u32 slot_idx = key % cmd_state->name_to_kind_table_size;
    CMD_Name2KindHashSlot *slot = cmd_state->name_to_kind_table + slot_idx;

    CMD_Name2KindNode *name_to_kind_n = slot->hash_first;
    for (; name_to_kind_n != 0; name_to_kind_n = name_to_kind_n->next) {
        if (key == name_to_kind_n->key)
            break;
    }

    assert(name_to_kind_n);

    return name_to_kind_n->kind;
}

internal void cmd_add(String8 name, CMD_Kind kind) {
    u64 key = str8_hash_u64(name);

    CMD_Name2KindNode *name_to_kind_n = push_struct(cmd_state->arena, CMD_Name2KindNode);
    name_to_kind_n->key = key;
    name_to_kind_n->name = name;
    name_to_kind_n->kind = kind;

    u32 slot_idx = key % cmd_state->name_to_kind_table_size;
    CMD_Name2KindHashSlot *slot = cmd_state->name_to_kind_table + slot_idx;

    SLL_PushBack(slot->hash_first, slot->hash_last, name_to_kind_n);
}
