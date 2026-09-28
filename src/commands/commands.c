global CMD_State *cmd_state = 0;

internal void cmd_init(void) {
    Arena *arena = arena_alloc();
    cmd_state = push_struct(arena, CMD_State);
    cmd_state->arena = arena;

    for (u32 i = 0; i < array_count(cmd_state->frame_arenas); i++) {
        cmd_state->frame_arenas[i] = arena_alloc();
    }

    cmd_state->name_to_kind_table_size = 100;
    cmd_state->name_to_kind_table = 
        push_array(arena, CMD_Name2KindHashSlot, cmd_state->name_to_kind_table_size);

    GENERATE_COMMANDS_ADD();
}

internal void cmd_tick(void) {
    CMD_Node *first = cmd_state->cmds.first;
    CMD_Node *last = cmd_state->cmds.last;

    for (CMD_Node *cmd_n = first;
            cmd_n != 0;) {
        CMD_Node *next_cmd_n = cmd_n->next;
        if (cmd_is_for_this_generation(cmd_n)) {
            DLL_Remove(first, last, cmd_n);
            cmd_n->next = cmd_state->first_free_cmd_n;
            cmd_state->first_free_cmd_n = cmd_n;
        }

        cmd_n = next_cmd_n;
    }

    cmd_state->cmds = (CMD_List){0};
    cmd_state->cmds.first = first;
    cmd_state->cmds.last = last;

    arena_clear(cmd_frame_arena());

    cmd_state->generation++;
}

internal Arena *cmd_frame_arena(void) {
    u32 idx = cmd_state->generation % array_count(cmd_state->frame_arenas);
    return cmd_state->frame_arenas[idx];
}

internal Arena *cmd_next_frame_arena(void) {
    u32 idx = (cmd_state->generation + 1) % array_count(cmd_state->frame_arenas);
    return cmd_state->frame_arenas[idx];
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
    cmd->generation = cmd_state->generation;

    return cmd;
}

internal CMD_List *cmd_get_pending(void) {
    return &cmd_state->cmds;
}

internal bool cmd_is_for_this_generation(CMD_Node *cmd_n) {
    return cmd_n->v.generation == cmd_state->generation;
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
