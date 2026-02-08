#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define NUM_CABINS 10

// Cabin state structure
typedef struct {
    uint8_t cabin_id;
    bool light_on;
    uint8_t temperature;  // Celsius
    bool emergency_active;
    bool fire_active;
    uint64_t last_update;
} CabinState;

// System state
typedef struct {
    CabinState cabins[NUM_CABINS];
    bool power_low;
    bool chain_pulled;
    bool system_emergency;
    pthread_mutex_t state_lock;
} SystemState;

// Global system state
extern SystemState g_system_state;

// Task function prototypes
void task_fire_emergency(void *arg);
void task_passenger_emergency(void *arg);
void task_chain_pull(void *arg);
void task_power_management(void *arg);
void task_temperature_regulation(void *arg);
void task_lighting_control(void *arg);
void task_display_update(void *arg);
void task_logging(void *arg);

// System state management
void system_state_init(void);
void system_state_lock(void);
void system_state_unlock(void);
void cabin_set_light(uint8_t cabin_id, bool on);
void cabin_set_temperature(uint8_t cabin_id, uint8_t temp);
void cabin_set_emergency(uint8_t cabin_id, bool active);
void cabin_set_fire(uint8_t cabin_id, bool active);
void system_set_power_low(bool low);
void system_set_chain_pull(bool pulled);

#endif // TASKS_H