
internal void wq_init(WQ_Queue *queue) {
    sem_init(&queue->semaphore, 0, 0);
}

internal void *wq_thread_func(void *params) {
    WQ_ThreadCtx *ctx = params;
    t_context_init(ctx->id);
    while (true) {
        if (!wq_do_work(ctx->queue)) {
            sem_wait(&ctx->queue->semaphore);
        }
    }
}

internal void wq_thread_launch(WQ_ThreadCtx *ctx) {
    pthread_t thread;
    pthread_create(&thread, 0, &wq_thread_func, (void *)ctx);
}

internal void wq_push_work_entry(WQ_Queue *queue, WQ_WorkCallback *func, void *data) {
    u32 new_next_idx_to_write = (queue->next_idx_to_write + 1) % array_count(queue->buffer);
    // assert(new_pending_work != queue->next_work);

    WQ_Work *work = queue->buffer + queue->next_idx_to_write;
    work->func = func;
    work->data = data;
    queue->target_completion_count++;

    __sync_synchronize();
    _mm_sfence();

    queue->next_idx_to_write = new_next_idx_to_write;
    sem_post(&queue->semaphore);
}

internal bool wq_do_work(WQ_Queue *queue) {
    bool result = false;
    u32 old_next_idx_to_read = queue->next_idx_to_read;
    u32 new_next_idx_to_read = (queue->next_idx_to_read + 1) % array_count(queue->buffer);
    if (old_next_idx_to_read != queue->next_idx_to_write) {
        u32 idx = __sync_val_compare_and_swap(&queue->next_idx_to_read, old_next_idx_to_read, new_next_idx_to_read);
        if (idx == old_next_idx_to_read) {
            WQ_Work *work = queue->buffer + idx;
            work->func(work->data);
            result = true;
            __sync_add_and_fetch(&queue->completion_count, 1);
        }
    }
}

internal void wq_complete_all_work(WQ_Queue *queue) {
    while (queue->completion_count != queue->target_completion_count) {
        wq_do_work(queue);
    }

    queue->completion_count = 0;
    queue->target_completion_count = 0;
}
