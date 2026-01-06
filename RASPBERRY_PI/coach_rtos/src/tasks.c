#include "tasks.h"
#include "scheduler.h"
#include "display.h"

// Fire Emergency Task (Priority 10)
void* fire_emergency_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Fire Emergency Task started");
    
    while (g_system.system_running && self->is_active) {
        pthread_mutex_lock(&g_system.system_mutex);
        
        if (g_system.fire_active) {
            self->state = TASK_RUNNING;
            pthread_mutex_unlock(&g_system.system_mutex);
            
            log_message("[FIRE TASK] Processing fire emergency");
            scheduler_task_complete(self->id);
            
            sleep(1);
        } else {
            self->state = TASK_READY;
            pthread_cond_wait(&g_system.task_ready_cond, &g_system.system_mutex);
            pthread_mutex_unlock(&g_system.system_mutex);
        }
    }
    
    log_message("Fire Emergency Task stopped");
    return NULL;
}

// Passenger Emergency Task (Priority 9)
void* passenger_emergency_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Passenger Emergency Task started");
    
    while (g_system.system_running && self->is_active) {
        pthread_mutex_lock(&g_system.system_mutex);
        
        if (g_system.emergency_active) {
            self->state = TASK_RUNNING;
            pthread_mutex_unlock(&g_system.system_mutex);
            
            log_message("[EMERGENCY TASK] Handling passenger emergency");
            scheduler_task_complete(self->id);
            
            sleep(1);
        } else {
            self->state = TASK_READY;
            pthread_cond_wait(&g_system.task_ready_cond, &g_system.system_mutex);
            pthread_mutex_unlock(&g_system.system_mutex);
        }
    }
    
    log_message("Passenger Emergency Task stopped");
    return NULL;
}

// Chain Pull Task (Priority 8)
void* chain_pull_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Chain Pull Task started");
    
    while (g_system.system_running && self->is_active) {
        self->state = TASK_READY;
        sleep(2);
        scheduler_task_complete(self->id);
    }
    
    log_message("Chain Pull Task stopped");
    return NULL;
}

// Power Management Task (Priority 7)
void* power_management_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Power Management Task started");
    
    while (g_system.system_running && self->is_active) {
        pthread_mutex_lock(&g_system.system_mutex);
        
        if (g_system.power_low) {
            self->state = TASK_RUNNING;
            pthread_mutex_unlock(&g_system.system_mutex);
            
            log_message("[POWER TASK] Managing low power state");
            scheduler_task_complete(self->id);
            
            sleep(2);
        } else {
            self->state = TASK_READY;
            pthread_mutex_unlock(&g_system.system_mutex);
            sleep(3);
            scheduler_task_complete(self->id);
        }
    }
    
    log_message("Power Management Task stopped");
    return NULL;
}

// Temperature Regulation Task (Priority 4)
void* temperature_regulation_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Temperature Regulation Task started");
    
    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;
        
        // Check and regulate temperature
        for (int i = 0; i < NUM_CABINS; i++) {
            pthread_mutex_lock(&g_system.cabins[i].mutex);
            
            if (g_system.cabins[i].state == STATE_TEMP_ADJUST) {
                // Simulate temperature adjustment
                sleep(1);
            }
            
            pthread_mutex_unlock(&g_system.cabins[i].mutex);
        }
        
        self->state = TASK_READY;
        sleep(5);
        scheduler_task_complete(self->id);
    }
    
    log_message("Temperature Regulation Task stopped");
    return NULL;
}

// Lighting Control Task (Priority 3)
void* lighting_control_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Lighting Control Task started");
    
    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;
        
        // Monitor lighting states
        scheduler_task_complete(self->id);
        
        self->state = TASK_READY;
        sleep(3);
    }
    
    log_message("Lighting Control Task stopped");
    return NULL;
}

// Display Task (Priority 2)
void* display_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Display Task started");
    
    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;
        
        // Update display
        display_terminal_update();
        scheduler_task_complete(self->id);
        
        self->state = TASK_READY;
        sleep(2);
    }
    
    log_message("Display Task stopped");
    return NULL;
}

// Logging Task (Priority 1)
void* logging_task(void* arg) {
    Task* self = (Task*)arg;
    log_message("Logging Task started");
    
    while (g_system.system_running && self->is_active) {
        self->state = TASK_RUNNING;
        
        // Periodic logging
        scheduler_task_complete(self->id);
        
        self->state = TASK_READY;
        sleep(10);
    }
    
    log_message("Logging Task stopped");
    return NULL;
}

// Helper: Handle fire alert
void handle_fire_alert(int cabin_id) {
    log_message("FIRE ALERT in Cabin %d!", cabin_id);
    
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.fire_active = true;
    pthread_mutex_unlock(&g_system.system_mutex);
    
    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    g_system.cabins[cabin_id].state = STATE_FIRE;
    g_system.cabins[cabin_id].light_on = false; // Cut power
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
    
    // Trigger high-priority task
    scheduler_preempt(PRIORITY_FIRE_EMERGENCY);
    pthread_cond_broadcast(&g_system.task_ready_cond);
    
    display_status_message("FIRE EMERGENCY!");
}

// Helper: Handle emergency
void handle_emergency(int cabin_id) {
    log_message("EMERGENCY in Cabin %d!", cabin_id);
    
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.emergency_active = true;
    pthread_mutex_unlock(&g_system.system_mutex);
    
    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    g_system.cabins[cabin_id].state = STATE_EMERGENCY;
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
    
    scheduler_preempt(PRIORITY_PASSENGER_EMERGENCY);
    pthread_cond_broadcast(&g_system.task_ready_cond);
    
    display_status_message("PASSENGER EMERGENCY!");
}

// Helper: Handle chain pull
void handle_chain_pull() {
    log_message("CHAIN PULLED - Emergency stop!");
    
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.emergency_active = true;
    pthread_mutex_unlock(&g_system.system_mutex);
    
    scheduler_preempt(PRIORITY_CHAIN_PULL);
    pthread_cond_broadcast(&g_system.task_ready_cond);
    
    display_status_message("CHAIN PULLED!");
}

// Helper: Handle low power
void handle_power_low() {
    log_message("LOW POWER condition detected");
    
    pthread_mutex_lock(&g_system.system_mutex);
    g_system.power_low = true;
    pthread_mutex_unlock(&g_system.system_mutex);
    
    // Turn off lights in non-critical cabins
    for (int i = 0; i < NUM_CABINS; i++) {
        pthread_mutex_lock(&g_system.cabins[i].mutex);
        
        if (g_system.cabins[i].state == STATE_NORMAL || 
            g_system.cabins[i].state == STATE_LIGHT_ON) {
            g_system.cabins[i].light_on = false;
            log_message("Power saving: Light OFF in Cabin %d", i);
        }
        
        pthread_mutex_unlock(&g_system.cabins[i].mutex);
    }
    
    display_status_message("LOW POWER MODE");
}

// Helper: Adjust temperature
void adjust_temperature(int cabin_id, int target_temp) {
    log_message("Adjusting temperature in Cabin %d to %d°C", cabin_id, target_temp);
    
    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    
    g_system.cabins[cabin_id].temperature = target_temp;
    if (g_system.cabins[cabin_id].state == STATE_NORMAL) {
        g_system.cabins[cabin_id].state = STATE_TEMP_ADJUST;
    }
    
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
}

// Helper: Control light
void control_light(int cabin_id, bool on) {
    log_message("Light %s in Cabin %d", on ? "ON" : "OFF", cabin_id);
    
    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    
    g_system.cabins[cabin_id].light_on = on;
    if (on && g_system.cabins[cabin_id].state == STATE_NORMAL) {
        g_system.cabins[cabin_id].state = STATE_LIGHT_ON;
    } else if (!on && g_system.cabins[cabin_id].state == STATE_LIGHT_ON) {
        g_system.cabins[cabin_id].state = STATE_NORMAL;
    }
    
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
}