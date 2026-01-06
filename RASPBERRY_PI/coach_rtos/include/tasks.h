#ifndef TASKS_H
#define TASKS_H

#include "common.h"

// Task Function Prototypes
void* fire_emergency_task(void* arg);
void* passenger_emergency_task(void* arg);
void* chain_pull_task(void* arg);
void* power_management_task(void* arg);
void* temperature_regulation_task(void* arg);
void* lighting_control_task(void* arg);
void* display_task(void* arg);
void* logging_task(void* arg);

// Task Helper Functions
void handle_fire_alert(int cabin_id);
void handle_emergency(int cabin_id);
void handle_chain_pull();
void handle_power_low();
void adjust_temperature(int cabin_id, int target_temp);
void control_light(int cabin_id, bool on);

#endif // TASKS_H