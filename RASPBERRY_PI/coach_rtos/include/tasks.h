#ifndef TASKS_H
#define TASKS_H

#include "common.h"

// Task function prototypes (return void* for pthread compatibility)
void* task_fire_emergency(void *arg);
void* task_passenger_emergency(void *arg);
void* task_chain_pull(void *arg);
void* task_power_management(void *arg);
void* task_temperature_regulation(void *arg);
void* task_lighting_control(void *arg);
void* task_display_update(void *arg);
void* task_logging(void *arg);

// System state management
void system_state_init(void);
void cabin_set_light(uint8_t cabin_id, bool on);
void cabin_set_temperature(uint8_t cabin_id, uint8_t temp);
void cabin_set_emergency(uint8_t cabin_id, bool active);
void cabin_set_fire(uint8_t cabin_id, bool active);
void system_set_power_low(bool low);
void system_set_chain_pull(bool pulled);
void system_clear_all_emergencies(void);
void cabin_clear_emergency(uint8_t cabin_id);

#endif // TASKS_H