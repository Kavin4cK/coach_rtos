#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

// Scheduler Functions
void scheduler_init();
int scheduler_add_task(const char* name, int priority, void* (*task_func)(void*));
void scheduler_start();
void scheduler_stop();
Task* scheduler_get_highest_priority_task();
void scheduler_preempt(int new_priority);
void scheduler_task_complete(int task_id);
void scheduler_print_status();

// Task Registration
void register_all_tasks();

#endif // SCHEDULER_H