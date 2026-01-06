#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdint.h>

// System Configuration
#define NUM_CABINS 10
#define MAX_TASKS 8
#define MAX_LOG_SIZE 1000

// Task Priorities (Higher = More Important)
#define PRIORITY_FIRE_EMERGENCY 10
#define PRIORITY_PASSENGER_EMERGENCY 9
#define PRIORITY_CHAIN_PULL 8
#define PRIORITY_POWER_MANAGEMENT 7
#define PRIORITY_TEMP_REGULATION 4
#define PRIORITY_LIGHTING 3
#define PRIORITY_DISPLAY 2
#define PRIORITY_LOGGING 1

// Cabin States
typedef enum {
    STATE_NORMAL = 0,
    STATE_LIGHT_ON = 1,
    STATE_TEMP_ADJUST = 2,
    STATE_EMERGENCY = 3,
    STATE_FIRE = 4
} CabinState;

// Task States
typedef enum {
    TASK_READY = 0,
    TASK_RUNNING = 1,
    TASK_BLOCKED = 2,
    TASK_SUSPENDED = 3
} TaskState;

// Cabin Structure
typedef struct {
    int id;
    bool light_on;
    int temperature;  // Celsius
    CabinState state;
    pthread_mutex_t mutex;
} Cabin;

// Task Structure
typedef struct {
    int id;
    char name[50];
    int priority;
    TaskState state;
    void* (*task_function)(void*);
    pthread_t thread;
    bool is_active;
    uint64_t execution_count;
    struct timespec last_execution;
} Task;

// System State
typedef struct {
    Cabin cabins[NUM_CABINS];
    Task tasks[MAX_TASKS];
    int num_tasks;
    bool system_running;
    bool power_low;
    bool emergency_active;
    bool fire_active;
    pthread_mutex_t system_mutex;
    pthread_cond_t task_ready_cond;
} SystemState;

// Global System State (extern declaration)
extern SystemState g_system;

// Utility Functions
void get_timestamp(char* buffer, size_t size);
void log_message(const char* format, ...);

#endif // COMMON_H