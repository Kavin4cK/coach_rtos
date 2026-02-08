#include "tasks.h"
#include "scheduler.h"
#include "display.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

// Utility function: Get timestamp
void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

// Utility function: Log message with timestamp
void log_message(const char* format, ...) {
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    printf("[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);
}

// Initialize system state
void system_state_init(void) {
    pthread_mutex_lock(&g_system.system_mutex);

    // Initialize all cabins
    for (int i = 0; i < NUM_CABINS; i++) {
        g_system.cabins[i].id = i;
        g_system.cabins[i].light_on = false;
        g_system.cabins[i].temperature = 22; // Default 22°C
        g_system.cabins[i].state = STATE_NORMAL;
        pthread_mutex_init(&g_system.cabins[i].mutex, NULL);
    }

    g_system.system_running = true;
    g_system.power_low = false;
    g_system.emergency_active = false;
    g_system.fire_active = false;

    pthread_mutex_unlock(&g_system.system_mutex);

    log_message("System state initialized with %d cabins", NUM_CABINS);
}

// Cabin control functions
void cabin_set_light(uint8_t cabin_id, bool on) {
    if (cabin_id >= NUM_CABINS) {
        log_message("ERROR: Invalid cabin ID %d", cabin_id);
        return;
    }

    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    g_system.cabins[cabin_id].light_on = on;

    if (on && g_system.cabins[cabin_id].state == STATE_NORMAL) {
        g_system.cabins[cabin_id].state = STATE_LIGHT_ON;
    } else if (!on && g_system.cabins[cabin_id].state == STATE_LIGHT_ON) {
        g_system.cabins[cabin_id].state = STATE_NORMAL;
    }

    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);

    log_message("Cabin %d: Light turned %s", cabin_id, on ? "ON" : "OFF");
}

void cabin_set_temperature(uint8_t cabin_id, uint8_t temp) {
    if (cabin_id >= NUM_CABINS) {
        log_message("ERROR: Invalid cabin ID %d", cabin_id);
        return;
    }

    if (temp < 10 || temp > 35) {
        log_message("ERROR: Temperature %d out of range (10-35°C)", temp);
        return;
    }

    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    g_system.cabins[cabin_id].temperature = temp;

    if (g_system.cabins[cabin_id].state == STATE_NORMAL) {
        g_system.cabins[cabin_id].state = STATE_TEMP_ADJUST;
    }

    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);

    log_message("Cabin %d: Temperature set to %d°C", cabin_id, temp);
}

void cabin_set_emergency(uint8_t cabin_id, bool active) {
    if (cabin_id >= NUM_CABINS) {
        log_message("ERROR: Invalid cabin ID %d", cabin_id);
        return;
    }

    pthread_mutex_lock(&g_system.system_mutex);
    g_system.emergency_active = active;
    pthread_mutex_unlock(&g_system.system_mutex);

    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    if (active) {
        g_system.cabins[cabin_id].state = STATE_EMERGENCY;
    }
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);

    if (active) {
        log_message("EMERGENCY ALERT in Cabin %d!", cabin_id);
        pthread_cond_broadcast(&g_system.task_ready_cond);
    }
}

void cabin_set_fire(uint8_t cabin_id, bool active) {
    if (cabin_id >= NUM_CABINS) {
        log_message("ERROR: Invalid cabin ID %d", cabin_id);
        return;
    }

    pthread_mutex_lock(&g_system.system_mutex);
    g_system.fire_active = active;
    pthread_mutex_unlock(&g_system.system_mutex);

    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    if (active) {
        g_system.cabins[cabin_id].state = STATE_FIRE;
        g_system.cabins[cabin_id].light_on = false; // Safety: Cut power
    }
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);

    if (active) {
        log_message("FIRE ALERT in Cabin %d!", cabin_id);
        pthread_cond_broadcast(&g_system.task_ready_cond);
    }
}

void system_set_power_low(bool low) {
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.power_low = low;
    pthread_mutex_unlock(&g_system.system_mutex);

    log_message("System power mode: %s", low ? "LOW" : "NORMAL");

    if (low) {
        // Load shedding: Turn off non-critical lights
        for (int i = 0; i < NUM_CABINS; i++) {
            pthread_mutex_lock(&g_system.cabins[i].mutex);
            if (g_system.cabins[i].state == STATE_NORMAL || g_system.cabins[i].state == STATE_LIGHT_ON) {
                g_system.cabins[i].light_on = false;
                log_message("Load shedding: Light OFF in Cabin %d", i);
            }
            pthread_mutex_unlock(&g_system.cabins[i].mutex);
        }
    }
}

void system_set_chain_pull(bool pulled) {
    if (pulled) {
        pthread_mutex_lock(&g_system.system_mutex);
        g_system.emergency_active = true;
        pthread_mutex_unlock(&g_system.system_mutex);

        log_message("CHAIN PULLED - Emergency stop triggered!");
        pthread_cond_broadcast(&g_system.task_ready_cond);
    }
}

void cabin_clear_emergency(uint8_t cabin_id) {
    if (cabin_id >= NUM_CABINS) {
        log_message("ERROR: Invalid cabin ID %d", cabin_id);
        return;
    }

    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    if (g_system.cabins[cabin_id].state == STATE_EMERGENCY || 
        g_system.cabins[cabin_id].state == STATE_FIRE) {
        g_system.cabins[cabin_id].state = STATE_NORMAL;
        log_message("Cabin %d: Emergency cleared", cabin_id);
    }
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
}

void system_clear_all_emergencies(void) {
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.emergency_active = false;
    g_system.fire_active = false;
    pthread_mutex_unlock(&g_system.system_mutex);

    // Clear all cabin emergency states
    for (int i = 0; i < NUM_CABINS; i++) {
        pthread_mutex_lock(&g_system.cabins[i].mutex);
        if (g_system.cabins[i].state == STATE_EMERGENCY || 
            g_system.cabins[i].state == STATE_FIRE) {
            g_system.cabins[i].state = STATE_NORMAL;
        }
        pthread_mutex_unlock(&g_system.cabins[i].mutex);
    }

    log_message("ALL EMERGENCIES CLEARED - System returned to normal");
    pthread_cond_broadcast(&g_system.task_ready_cond);
}

// ==================== TASK IMPLEMENTATIONS ====================

// Fire Emergency Task (Priority 10)
void* task_fire_emergency(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Fire Emergency Handler started", self->id);
    bool prev_fire_active = false;

    while (g_system.system_running && self->is_active) {
        pthread_mutex_lock(&g_system.system_mutex);
        bool current_fire_active = g_system.fire_active;

        // Only process on rising edge (false -> true transition)
        if (current_fire_active && !prev_fire_active) {
            self->state = TASK_RUNNING;
            pthread_mutex_unlock(&g_system.system_mutex);

            log_message("[FIRE EMERGENCY] Processing fire alert - HIGHEST PRIORITY");

            // Critical fire response actions
            for (int i = 0; i < NUM_CABINS; i++) {
                pthread_mutex_lock(&g_system.cabins[i].mutex);
                if (g_system.cabins[i].state == STATE_FIRE) {
                    log_message("[FIRE EMERGENCY] Handling fire in Cabin %d", i);
                }
                pthread_mutex_unlock(&g_system.cabins[i].mutex);
            }

            log_message("[FIRE EMERGENCY] Alert processed. Use CLEAR to reset.");
            scheduler_task_complete(self->id);
            self->state = TASK_READY;
            prev_fire_active = true;
            usleep(100000); // 100ms response time
        } else if (!current_fire_active) {
            // Reset when emergency is cleared
            prev_fire_active = false;
            self->state = TASK_READY;
            pthread_cond_wait(&g_system.task_ready_cond, &g_system.system_mutex);
            pthread_mutex_unlock(&g_system.system_mutex);
        } else {
            // Emergency still active but already processed
            self->state = TASK_READY;
            pthread_mutex_unlock(&g_system.system_mutex);
            usleep(500000); // Sleep 500ms while waiting for clear
        }
    }

    log_message("[Task %d] Fire Emergency Handler stopped", self->id);
    return NULL;
}

// Passenger Emergency Task (Priority 9)
void* task_passenger_emergency(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Passenger Emergency Handler started", self->id);
    bool prev_emergency_active = false;

    while (g_system.system_running && self->is_active) {
        pthread_mutex_lock(&g_system.system_mutex);
        bool current_emergency_active = g_system.emergency_active;

        // Only process on rising edge (false -> true transition)
        if (current_emergency_active && !prev_emergency_active) {
            self->state = TASK_RUNNING;
            pthread_mutex_unlock(&g_system.system_mutex);

            log_message("[PASSENGER EMERGENCY] Handling emergency alert");

            // Emergency response actions
            for (int i = 0; i < NUM_CABINS; i++) {
                pthread_mutex_lock(&g_system.cabins[i].mutex);
                if (g_system.cabins[i].state == STATE_EMERGENCY) {
                    log_message("[PASSENGER EMERGENCY] Alert in Cabin %d", i);
                }
                pthread_mutex_unlock(&g_system.cabins[i].mutex);
            }

            log_message("[PASSENGER EMERGENCY] Alert processed. Use CLEAR to reset.");
            scheduler_task_complete(self->id);
            self->state = TASK_READY;
            prev_emergency_active = true;
            usleep(150000); // 150ms adaptive response
        } else if (!current_emergency_active) {
            // Reset when emergency is cleared
            prev_emergency_active = false;
            self->state = TASK_READY;
            pthread_cond_wait(&g_system.task_ready_cond, &g_system.system_mutex);
            pthread_mutex_unlock(&g_system.system_mutex);
        } else {
            // Emergency still active but already processed
            self->state = TASK_READY;
            pthread_mutex_unlock(&g_system.system_mutex);
            usleep(500000); // Sleep 500ms while waiting for clear
        }
    }

    log_message("[Task %d] Passenger Emergency Handler stopped", self->id);
    return NULL;
}

// Chain Pull Task (Priority 8)
void* task_chain_pull(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Chain Pull Handler started", self->id);
    bool prev_chain_pulled = false;

    while (g_system.system_running && self->is_active) {
        self->state = TASK_READY;

        // Check for chain pull emergency
        pthread_mutex_lock(&g_system.system_mutex);
        bool emergency = g_system.emergency_active;
        pthread_mutex_unlock(&g_system.system_mutex);

        // Only process on rising edge
        if (emergency && !prev_chain_pulled) {
            self->state = TASK_RUNNING;
            log_message("[CHAIN PULL] Emergency stop sequence activated");
            log_message("[CHAIN PULL] Alert processed. Use CLEAR to reset.");
            scheduler_task_complete(self->id);
            self->state = TASK_READY;
            prev_chain_pulled = true;
        } else if (!emergency) {
            prev_chain_pulled = false;
        }

        usleep(200000); // 200ms cycle
    }

    log_message("[Task %d] Chain Pull Handler stopped", self->id);
    return NULL;
}

// Power Management Task (Priority 7)
void* task_power_management(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Power Management started", self->id);

    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;

        pthread_mutex_lock(&g_system.system_mutex);
        bool power_low = g_system.power_low;
        pthread_mutex_unlock(&g_system.system_mutex);

        if (power_low) {
            log_message("[POWER MGMT] Operating in LOW POWER mode");
        }

        scheduler_task_complete(self->id);
        self->state = TASK_READY;

        usleep(500000); // 500ms cycle
    }

    log_message("[Task %d] Power Management stopped", self->id);
    return NULL;
}

// Temperature Regulation Task (Priority 4)
void* task_temperature_regulation(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Temperature Regulation started", self->id);

    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;

        // Monitor and regulate temperature
        for (int i = 0; i < NUM_CABINS; i++) {
            pthread_mutex_lock(&g_system.cabins[i].mutex);
            if (g_system.cabins[i].state == STATE_TEMP_ADJUST) {
                // Simulate temperature regulation
            }
            pthread_mutex_unlock(&g_system.cabins[i].mutex);
        }

        scheduler_task_complete(self->id);
        self->state = TASK_READY;

        sleep(1); // 1s cycle
    }

    log_message("[Task %d] Temperature Regulation stopped", self->id);
    return NULL;
}

// Lighting Control Task (Priority 3)
void* task_lighting_control(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Lighting Control started", self->id);

    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;

        // Monitor lighting systems
        scheduler_task_complete(self->id);
        self->state = TASK_READY;

        usleep(800000); // 800ms cycle
    }

    log_message("[Task %d] Lighting Control stopped", self->id);
    return NULL;
}

// Display Update Task (Priority 2)
void* task_display_update(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] Display Update started", self->id);

    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;

        // Update display (terminal or framebuffer)
        display_render();

        scheduler_task_complete(self->id);
        self->state = TASK_READY;

        usleep(500000); // 500ms cycle (2Hz)
    }

    log_message("[Task %d] Display Update stopped", self->id);
    return NULL;
}

// System Logging Task (Priority 1)
void* task_logging(void* arg) {
    Task* self = (Task*)arg;
    log_message("[Task %d] System Logging started", self->id);

    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;

        // Periodic system status logging
        scheduler_task_complete(self->id);
        self->state = TASK_READY;

        sleep(2); // 2s cycle
    }

    log_message("[Task %d] System Logging stopped", self->id);
    return NULL;
}
