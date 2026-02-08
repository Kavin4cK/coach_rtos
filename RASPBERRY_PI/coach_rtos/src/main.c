#include "scheduler.h"
#include "tasks.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

extern int usb_serial_init(const char *device);
extern void usb_serial_close(void);
extern void usb_start_listener(void);
extern void usb_stop_listener(void);

extern int network_init(void);
extern void network_start_listener(void);
extern void network_stop_listener(void);
extern void network_close(void);

static volatile bool running = true;

void signal_handler(int signum) {
    (void)signum; // Suppress unused parameter warning
    printf("\n[MAIN] Received signal, shutting down...\n");
    running = false;
}

void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║       INDIAN RAILWAYS - RTOS COACH CONTROL SYSTEM         ║\n");
    printf("║                                                           ║\n");
    printf("║  Real-Time Operating System Simulation Project           ║\n");
    printf("║  Priority-Based Task Scheduling & Emergency Management   ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void print_help(void) {
    printf("Usage: coach_rtos [options]\n");
    printf("\nOptions:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -d, --device <device>   USB serial device (default: stdin)\n");
    printf("  -n, --network           Enable network mode (TCP port 5000)\n");
    printf("  -f, --framebuffer       Use framebuffer display\n");
    printf("  -t, --terminal          Use terminal display (default)\n");
    printf("\nCommands (via USB/stdin):\n");
    printf("  LIGHT <cabin_id> ON|OFF        Control cabin lighting\n");
    printf("  TEMP <cabin_id> <value>        Set cabin temperature (10-35°C)\n");
    printf("  EMERGENCY <cabin_id>           Trigger passenger emergency\n");
    printf("  FIRE <cabin_id>                Trigger fire alert\n");
    printf("  POWER LOW|NORMAL               Set power status\n");
    printf("  CHAIN PULL                     Simulate chain pull\n");
    printf("  CLEAR <cabin_id>               Clear emergency in specific cabin\n");
    printf("  CLEAR ALL                      Clear all emergencies system-wide\n");
    printf("\nExamples:\n");
    printf("  LIGHT 3 ON\n");
    printf("  TEMP 5 24\n");
    printf("  EMERGENCY 2\n");
    printf("  FIRE 7\n");
    printf("  CLEAR 2\n");
    printf("  CLEAR ALL\n");
    printf("  POWER LOW\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    const char *serial_device = NULL;
    DisplayMode display_mode = DISPLAY_MODE_TERMINAL;
    bool use_network = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
            if (i + 1 < argc) {
                serial_device = argv[++i];
            } else {
                fprintf(stderr, "Error: --device requires an argument\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--network") == 0) {
            use_network = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--framebuffer") == 0) {
            display_mode = DISPLAY_MODE_FRAMEBUFFER;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--terminal") == 0) {
            display_mode = DISPLAY_MODE_TERMINAL;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        }
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    print_banner();
    
    printf("[MAIN] Initializing RTOS Coach Control System...\n");
    
    // Initialize system state
    system_state_init();
    
    // Initialize display
    if (!display_init(display_mode)) {
        fprintf(stderr, "[MAIN] Failed to initialize display\n");
        return 1;
    }
    
    // Initialize communication (USB or Network)
    if (use_network) {
        printf("[MAIN] Starting network mode on port 5000...\n");
        if (network_init() < 0) {
            fprintf(stderr, "[MAIN] Failed to initialize network\n");
            return 1;
        }
    } else {
        printf("[MAIN] Starting USB serial mode...\n");
        if (serial_device) {
            usb_serial_init(serial_device);
        } else {
            usb_serial_init("/dev/ttyUSB0"); // Default, will fallback to stdin
        }
    }
    
    // Initialize scheduler
    scheduler_init();
    
    // Create all tasks with their priorities
    printf("\n[MAIN] Creating tasks...\n");
    
    Task *task_fire = scheduler_create_task(
        "Fire Emergency Handler",
        PRIORITY_FIRE_EMERGENCY,
        task_fire_emergency,
        NULL
    );
    
    Task *task_emergency = scheduler_create_task(
        "Passenger Emergency Handler",
        PRIORITY_PASSENGER_EMERGENCY,
        task_passenger_emergency,
        NULL
    );
    
    Task *task_chain = scheduler_create_task(
        "Chain Pull Handler",
        PRIORITY_CHAIN_PULL,
        task_chain_pull,
        NULL
    );
    
    Task *task_power = scheduler_create_task(
        "Power Management",
        PRIORITY_POWER_MANAGEMENT,
        task_power_management,
        NULL
    );
    
    Task *task_temp = scheduler_create_task(
        "Temperature Regulation",
        PRIORITY_TEMP_REGULATION,
        task_temperature_regulation,
        NULL
    );
    
    Task *task_light = scheduler_create_task(
        "Lighting Control",
        PRIORITY_LIGHTING,
        task_lighting_control,
        NULL
    );
    
    Task *task_display = scheduler_create_task(
        "Display Update",
        PRIORITY_DISPLAY,
        task_display_update,
        NULL
    );
    
    Task *task_log = scheduler_create_task(
        "System Logging",
        PRIORITY_LOGGING,
        task_logging,
        NULL
    );
    
    // Add tasks to scheduler
    scheduler_add_task(task_fire);
    scheduler_add_task(task_emergency);
    scheduler_add_task(task_chain);
    scheduler_add_task(task_power);
    scheduler_add_task(task_temp);
    scheduler_add_task(task_light);
    scheduler_add_task(task_display);
    scheduler_add_task(task_log);
    
    printf("\n[MAIN] Starting scheduler and listener...\n");
    
    // Start listener (USB or Network)
    if (use_network) {
        network_start_listener();
    } else {
        usb_start_listener();
    }
    
    // Start scheduler (creates threads for all tasks)
    scheduler_start();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SYSTEM READY - Waiting for commands...                  ║\n");
    printf("║  Press Ctrl+C to exit                                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (!serial_device) {
        printf("Enter commands (type 'help' for command list):\n");
    }
    
    // Main loop
    while (running) {
        sleep(1);
    }
    
    // Cleanup
    printf("\n[MAIN] Shutting down system...\n");
    
    if (use_network) {
        network_stop_listener();
        network_close();
    } else {
        usb_stop_listener();
        usb_serial_close();
    }
    scheduler_stop();
    display_cleanup();
    
    printf("[MAIN] System shutdown complete\n");
    
    return 0;
}