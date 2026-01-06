#include "common.h"
#include "scheduler.h"
#include "tasks.h"
#include "display.h"
#include <signal.h>
#include <stdarg.h>

// Global System State Definition
SystemState g_system;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_message("Received shutdown signal, stopping system...");
        g_system.system_running = false;
    }
}

// Initialize system state
void system_init() {
    pthread_mutex_init(&g_system.system_mutex, NULL);
    pthread_cond_init(&g_system.task_ready_cond, NULL);
    
    // Initialize cabins
    for (int i = 0; i < NUM_CABINS; i++) {
        g_system.cabins[i].id = i;
        g_system.cabins[i].light_on = false;
        g_system.cabins[i].temperature = 24; // Default 24°C
        g_system.cabins[i].state = STATE_NORMAL;
        pthread_mutex_init(&g_system.cabins[i].mutex, NULL);
    }
    
    g_system.num_tasks = 0;
    g_system.system_running = true;
    g_system.power_low = false;
    g_system.emergency_active = false;
    g_system.fire_active = false;
    
    log_message("System initialized with %d cabins", NUM_CABINS);
}

// Cleanup system resources
void system_cleanup() {
    log_message("Cleaning up system resources...");
    
    pthread_mutex_destroy(&g_system.system_mutex);
    pthread_cond_destroy(&g_system.task_ready_cond);
    
    for (int i = 0; i < NUM_CABINS; i++) {
        pthread_mutex_destroy(&g_system.cabins[i].mutex);
    }
    
    display_cleanup();
}

// Utility: Get timestamp
void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", t);
}

// Utility: Log message
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

// USB listener thread (reads from stdin)
void* usb_listener_thread(void* arg) {
    char buffer[256];
    
    log_message("USB listener started");
    
    while (g_system.system_running) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // Remove newline
            buffer[strcspn(buffer, "\n")] = 0;
            
            if (strlen(buffer) == 0) continue;
            
            log_message("Received command: %s", buffer);
            
            // Parse command
            char cmd[32], param1[32], param2[32];
            int n = sscanf(buffer, "%s %s %s", cmd, param1, param2);
            
            if (n >= 2) {
                if (strcmp(cmd, "LIGHT") == 0) {
                    int cabin_id = atoi(param1);
                    if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                        bool on = (n >= 3 && strcmp(param2, "ON") == 0);
                        control_light(cabin_id, on);
                    }
                }
                else if (strcmp(cmd, "TEMP") == 0) {
                    int cabin_id = atoi(param1);
                    int temp = atoi(param2);
                    if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                        adjust_temperature(cabin_id, temp);
                    }
                }
                else if (strcmp(cmd, "EMERGENCY") == 0) {
                    int cabin_id = atoi(param1);
                    if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                        handle_emergency(cabin_id);
                    }
                }
                else if (strcmp(cmd, "FIRE") == 0) {
                    int cabin_id = atoi(param1);
                    if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                        handle_fire_alert(cabin_id);
                    }
                }
                else if (strcmp(cmd, "POWER") == 0) {
                    if (strcmp(param1, "LOW") == 0) {
                        handle_power_low();
                    }
                }
                else if (strcmp(cmd, "CHAIN") == 0) {
                    handle_chain_pull();
                }
                else if (strcmp(cmd, "STATUS") == 0) {
                    scheduler_print_status();
                }
            }
        }
        usleep(50000); // 50ms
    }
    
    log_message("USB listener stopped");
    return NULL;
}

int main(int argc, char* argv[]) {
    printf("=================================================\n");
    printf("  RTOS Coach Subsystem Control Simulation\n");
    printf("  Indian Railways LHB Coach Management System\n");
    printf("=================================================\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize system
    system_init();
    
    // Initialize display
    if (display_init() != 0) {
        log_message("Warning: Display initialization failed, using terminal mode");
    }
    
    // Initialize scheduler
    scheduler_init();
    
    // Register all tasks
    register_all_tasks();
    
    // Start USB listener thread
    pthread_t usb_thread;
    pthread_create(&usb_thread, NULL, usb_listener_thread, NULL);
    
    // Start scheduler
    log_message("Starting scheduler...");
    scheduler_start();
    
    // Main loop
    log_message("System running. Commands: LIGHT, TEMP, EMERGENCY, FIRE, POWER, CHAIN, STATUS");
    
    while (g_system.system_running) {
        sleep(1);
    }
    
    // Cleanup
    log_message("Shutting down system...");
    scheduler_stop();
    pthread_join(usb_thread, NULL);
    system_cleanup();
    
    printf("\n=================================================\n");
    printf("  System shutdown complete\n");
    printf("=================================================\n");
    
    return 0;
}