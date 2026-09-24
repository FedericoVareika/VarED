#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

typedef struct WQ_Queue WQ_Queue;

#define WQ_CALLBACK(name) void name(void *data)
typedef WQ_CALLBACK(WQ_WorkCallback); 

typedef struct WQ_Work WQ_Work;
struct WQ_Work {
    WQ_WorkCallback *func;
    void *data;
};

typedef struct WQ_ThreadCtx WQ_ThreadCtx;
struct WQ_ThreadCtx {
    u32 id;
    WQ_Queue *queue;
};

internal void wq_init(WQ_Queue *queue);
internal void wq_thread_launch(WQ_ThreadCtx *ctx);
internal void wq_push_work_entry(WQ_Queue *queue, WQ_WorkCallback *callback, void *data);

internal bool wq_do_work(WQ_Queue *queue);

internal void wq_complete_all_work(WQ_Queue *queue);

#endif // WORK_QUEUE_H
