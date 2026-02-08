#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

// Scheduler Functions
void scheduler_init(void);
Task* scheduler_create_task(const char* name, int priority, void* (*task_func)(void*), void* arg);
void scheduler_add_task(Task* task);
void scheduler_start(void);
void scheduler_stop(void);
Task* scheduler_get_highest_priority_task(void);
void scheduler_preempt(int new_priority);
void scheduler_task_complete(int task_id);
void scheduler_print_status(void);

#endif // SCHEDULER_H