
internal void wq_init_task(Arena *arena, WQ_Task *task) {
    if (arena) {
        task->arena_ = arena;
    } else {
        task->arena_ = arena_alloc();
    }
}

internal WQ_Task *wq_begin_task_memory(WQ_Task *tasks, u32 task_count) {
    assert(tasks);
    WQ_Task *result = 0;
    for (u32 i = 0; i < task_count; i++) {
        WQ_Task *task = tasks + i;
        if (!task->being_used) {
            assert(task->arena_);
            result = task;
            break;
        }
    }
    
    if (result) {
        result->temp = temp_begin(result->arena_);
        result->being_used = true;
    }

    return result;
}

internal void wq_end_task_memory(WQ_Task *task) {
    temp_end(task->temp);
    task->being_used = false;
}
