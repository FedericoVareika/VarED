#ifndef WORK_QUEUE_INC_H
#define WORK_QUEUE_INC_H

#include "work_queue.h"

#if OS_LINUX
#include "linux/work_queue_linux.h"
#endif

#endif // WORK_QUEUE_INC_H
