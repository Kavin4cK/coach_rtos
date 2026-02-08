#include "scheduler.h"
#include "tasks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global system state definition
SystemState g_system;

// Initialize scheduler
void scheduler_init(void) {
    pthread_mutex_init(&g_system.system_mutex, NULL);
    pthread_cond_init(&g_system.task_ready_cond, NULL);

    g_system.num_tasks = 0;
    g_system.system_running = true;
    g_system.power_low = false;
    g_system.emergency_active = false;
    g_system.fire_active = false;

    log_message("Scheduler initialized");
}

// Create a new task (returns Task pointer)
Task* scheduler_create_task(const char* name, int priority, void* (*task_func)(void*), void* arg) {
    (void)arg; // Currently unused, for future expansion

    if (g_system.num_tasks >= MAX_TASKS) {
        log_message("Error: Maximum tasks reached");
        return NULL;
    }

    int task_id = g_system.num_tasks;
    Task* task = &g_system.tasks[task_id];

    task->id = task_id;
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
    task->priority = priority;
    task->state = TASK_READY;
    task->task_function = task_func;
    task->is_active = false; // Will be activated when added
    task->execution_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &task->last_execution);

    g_system.num_tasks++;

    return task;
}

// Add a task to the scheduler
void scheduler_add_task(Task* task) {
    if (task == NULL) {
        log_message("Error: Cannot add NULL task");
        return;
    }

    pthread_mutex_lock(&g_system.system_mutex);
    task->is_active = true;
    pthread_mutex_unlock(&g_system.system_mutex);

    log_message("Task added: %s (Priority: %d)", task->name, task->priority);
}

// Get highest priority runnable task
Task* scheduler_get_highest_priority_task(void) {
    Task* highest = NULL;
    int max_priority = -1;

    pthread_mutex_lock(&g_system.system_mutex);

    for (int i = 0; i < g_system.num_tasks; i++) {
        Task* task = &g_system.tasks[i];

        if (task->is_active && task->state == TASK_READY) {
            if (task->priority > max_priority) {
                max_priority = task->priority;
                highest = task;
            }
        }
    }

    pthread_mutex_unlock(&g_system.system_mutex);

    return highest;
}

// Start all tasks
void scheduler_start(void) {
    log_message("Starting all tasks...");

    pthread_mutex_lock(&g_system.system_mutex);
    g_system.system_running = true;

    for (int i = 0; i < g_system.num_tasks; i++) {
        Task* task = &g_system.tasks[i];

        if (task->is_active) {
            if (pthread_create(&task->thread, NULL, task->task_function, task) != 0) {
                log_message("Error: Failed to create thread for task %s", task->name);
                task->is_active = false;
            } else {
                log_message("Started task: %s", task->name);
            }
        }
    }

    pthread_mutex_unlock(&g_system.system_mutex);
}

// Stop all tasks
void scheduler_stop(void) {
    log_message("Stopping all tasks...");

    pthread_mutex_lock(&g_system.system_mutex);
    g_system.system_running = false;

    for (int i = 0; i < g_system.num_tasks; i++) {
        Task* task = &g_system.tasks[i];
        task->is_active = false;
    }

    pthread_mutex_unlock(&g_system.system_mutex);
    pthread_cond_broadcast(&g_system.task_ready_cond);

    // Wait for all tasks to complete
    for (int i = 0; i < g_system.num_tasks; i++) {
        Task* task = &g_system.tasks[i];
        if (task->thread) {
            pthread_join(task->thread, NULL);
            log_message("Task stopped: %s", task->name);
        }
    }
}

// Simulate preemption
void scheduler_preempt(int new_priority) {
    log_message("Preemption triggered with priority %d", new_priority);
    pthread_cond_broadcast(&g_system.task_ready_cond);
}

// Mark task execution complete
void scheduler_task_complete(int task_id) {
    pthread_mutex_lock(&g_system.system_mutex);

    if (task_id >= 0 && task_id < g_system.num_tasks) {
        Task* task = &g_system.tasks[task_id];
        task->execution_count++;
        clock_gettime(CLOCK_MONOTONIC, &task->last_execution);
    }

    pthread_mutex_unlock(&g_system.system_mutex);
}

// Print scheduler status
void scheduler_print_status(void) {
    printf("\n=== SCHEDULER STATUS ===\n");

    pthread_mutex_lock(&g_system.system_mutex);

    printf("Total Tasks: %d\n", g_system.num_tasks);
    printf("System Running: %s\n", g_system.system_running ? "YES" : "NO");
    printf("\nTask Details:\n");
    printf("%-3s %-30s %-8s %-10s %-12s\n", "ID", "Name", "Priority", "State", "Exec Count");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < g_system.num_tasks; i++) {
        Task* task = &g_system.tasks[i];

        const char* state_str;
        switch (task->state) {
            case TASK_READY: state_str = "READY"; break;
            case TASK_RUNNING: state_str = "RUNNING"; break;
            case TASK_BLOCKED: state_str = "BLOCKED"; break;
            case TASK_SUSPENDED: state_str = "SUSPENDED"; break;
            default: state_str = "UNKNOWN"; break;
        }

        printf("%-3d %-30s %-8d %-10s %-12lu\n",
               task->id, task->name, task->priority, state_str, (unsigned long)task->execution_count);
    }

    printf("\nCabin Status:\n");
    printf("%-6s %-10s %-12s %-10s\n", "Cabin", "Light", "Temp (°C)", "State");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < NUM_CABINS; i++) {
        Cabin* cabin = &g_system.cabins[i];

        const char* state_str;
        switch (cabin->state) {
            case STATE_NORMAL: state_str = "Normal"; break;
            case STATE_LIGHT_ON: state_str = "Light On"; break;
            case STATE_TEMP_ADJUST: state_str = "Temp Adj"; break;
            case STATE_EMERGENCY: state_str = "EMERGENCY"; break;
            case STATE_FIRE: state_str = "FIRE"; break;
            default: state_str = "Unknown"; break;
        }

        printf("%-6d %-10s %-12d %-10s\n",
               cabin->id, cabin->light_on ? "ON" : "OFF", cabin->temperature, state_str);
    }

    pthread_mutex_unlock(&g_system.system_mutex);

    printf("========================\n\n");
}
