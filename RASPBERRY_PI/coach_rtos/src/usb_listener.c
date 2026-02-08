#include "tasks.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>

#define MAX_USB_PORTS 3

typedef struct {
    int fd;
    char device_path[64];
    pthread_t thread;
    bool active;
    int port_id;
} USBPort;

static USBPort usb_ports[MAX_USB_PORTS];
static bool listener_running = false;
static pthread_mutex_t command_lock = PTHREAD_MUTEX_INITIALIZER;

int usb_serial_init_port(USBPort *port, const char *device, int port_id) {
    port->port_id = port_id;
    strncpy(port->device_path, device, sizeof(port->device_path) - 1);
    port->active = false;
    
    port->fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (port->fd < 0) {
        fprintf(stderr, "[USB%d] Failed to open %s: %s\n", port_id, device, strerror(errno));
        return -1;
    }
    
    struct termios options;
    tcgetattr(port->fd, &options);
    
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;
    
    tcsetattr(port->fd, TCSANOW, &options);
    
    port->active = true;
    printf("[USB%d] Serial port %s opened successfully at 115200 baud\n", port_id, device);
    return 0;
}

int usb_serial_init(const char *device) {
    (void)device; // Suppress unused parameter warning
    
    // Initialize all 3 USB ports
    const char *devices[MAX_USB_PORTS] = {
        "/dev/ttyUSB0",
        "/dev/ttyUSB1",
        "/dev/ttyUSB2"
    };
    
    int active_count = 0;
    
    for (int i = 0; i < MAX_USB_PORTS; i++) {
        if (usb_serial_init_port(&usb_ports[i], devices[i], i) == 0) {
            active_count++;
        }
    }
    
    if (active_count == 0) {
        fprintf(stderr, "[USB] No USB ports could be opened. Check connections.\n");
        fprintf(stderr, "[USB] System will accept stdin input only.\n");
        return -1;
    }
    
    printf("[USB] Successfully opened %d/%d USB ports\n", active_count, MAX_USB_PORTS);
    return active_count;
}

void usb_serial_close(void) {
    for (int i = 0; i < MAX_USB_PORTS; i++) {
        if (usb_ports[i].active && usb_ports[i].fd >= 0) {
            close(usb_ports[i].fd);
            usb_ports[i].active = false;
            printf("[USB%d] Closed\n", i);
        }
    }
}

void usb_send_response(int port_id, const char *message) {
    if (port_id < 0 || port_id >= MAX_USB_PORTS) {
        printf("[USB_TX] %s\n", message);
        return;
    }
    
    if (usb_ports[port_id].active && usb_ports[port_id].fd >= 0) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "[USB%d] %s\n", port_id, message);
        ssize_t written = write(usb_ports[port_id].fd, buffer, strlen(buffer));
        (void)written; // Suppress unused variable warning
    }
}

void parse_and_execute_command(char *cmd, int port_id) {
    pthread_mutex_lock(&command_lock);
    
    cmd[strcspn(cmd, "\r\n")] = 0;
    
    if (strlen(cmd) == 0) {
        pthread_mutex_unlock(&command_lock);
        return;
    }
    
    printf("[USB%d RX] Received: '%s'\n", port_id, cmd);
    
    char command[32], arg1[32], arg2[32];
    int cabin_id, value;
    
    int parsed = sscanf(cmd, "%s %s %s", command, arg1, arg2);
    
    if (parsed < 2) {
        printf("[USB%d] Invalid command format\n", port_id);
        usb_send_response(port_id, "ERROR: Invalid format");
        pthread_mutex_unlock(&command_lock);
        return;
    }
    
    cabin_id = atoi(arg1);
    
    if (strcmp(command, "LIGHT") == 0) {
        if (cabin_id < 0 || cabin_id >= NUM_CABINS) {
            usb_send_response(port_id, "ERROR: Invalid cabin ID");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
        if (strcmp(arg2, "ON") == 0) {
            cabin_set_light(cabin_id, true);
            usb_send_response(port_id, "OK: Light ON");
        } else if (strcmp(arg2, "OFF") == 0) {
            cabin_set_light(cabin_id, false);
            usb_send_response(port_id, "OK: Light OFF");
        } else {
            usb_send_response(port_id, "ERROR: Use ON or OFF");
        }
        
    } else if (strcmp(command, "TEMP") == 0) {
        if (cabin_id < 0 || cabin_id >= NUM_CABINS) {
            usb_send_response(port_id, "ERROR: Invalid cabin ID");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
        value = atoi(arg2);
        if (value < 10 || value > 35) {
            usb_send_response(port_id, "ERROR: Temperature range 10-35°C");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
        cabin_set_temperature(cabin_id, value);
        usb_send_response(port_id, "OK: Temperature set");
        
    } else if (strcmp(command, "EMERGENCY") == 0) {
        if (cabin_id < 0 || cabin_id >= NUM_CABINS) {
            usb_send_response(port_id, "ERROR: Invalid cabin ID");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
        cabin_set_emergency(cabin_id, true);
        usb_send_response(port_id, "OK: Emergency activated");
        
        // Trigger preemption with emergency priority
        scheduler_preempt(PRIORITY_PASSENGER_EMERGENCY);
        
    } else if (strcmp(command, "FIRE") == 0) {
        if (cabin_id < 0 || cabin_id >= NUM_CABINS) {
            usb_send_response(port_id, "ERROR: Invalid cabin ID");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
        cabin_set_fire(cabin_id, true);
        usb_send_response(port_id, "OK: Fire alert activated");
        
        // Trigger preemption with fire priority (highest)
        scheduler_preempt(PRIORITY_FIRE_EMERGENCY);
        
    } else if (strcmp(command, "POWER") == 0) {
        if (strcmp(arg1, "LOW") == 0) {
            system_set_power_low(true);
            usb_send_response(port_id, "OK: Power set to LOW");
        } else if (strcmp(arg1, "NORMAL") == 0) {
            system_set_power_low(false);
            usb_send_response(port_id, "OK: Power set to NORMAL");
        } else {
            usb_send_response(port_id, "ERROR: Use LOW or NORMAL");
            pthread_mutex_unlock(&command_lock);
            return;
        }
        
    } else if (strcmp(command, "CHAIN") == 0) {
        if (strcmp(arg1, "PULL") == 0) {
            system_set_chain_pull(true);
            usb_send_response(port_id, "OK: Chain pulled");
            
            // Trigger preemption with chain pull priority
            scheduler_preempt(PRIORITY_CHAIN_PULL);
        } else {
            usb_send_response(port_id, "ERROR: Use PULL");
        }
        
    } else if (strcmp(command, "CLEAR") == 0) {
        if (strcmp(arg1, "ALL") == 0) {
            system_clear_all_emergencies();
            usb_send_response(port_id, "OK: All emergencies cleared");
        } else {
            int cabin_id = atoi(arg1);
            if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                cabin_clear_emergency(cabin_id);
                usb_send_response(port_id, "OK: Emergency cleared");
            } else {
                usb_send_response(port_id, "ERROR: Invalid cabin ID");
            }
        }
        
    } else {
        usb_send_response(port_id, "ERROR: Unknown command");
    }
    
    pthread_mutex_unlock(&command_lock);
}

void* usb_listener_loop(void *arg) {
    USBPort *port = (USBPort*)arg;
    char buffer[256];
    int buffer_pos = 0;
    
    printf("[USB%d] Listener thread started for %s\n", port->port_id, port->device_path);
    
    while (listener_running && port->active) {
        char c;
        ssize_t bytes_read = read(port->fd, &c, 1);
        
        if (bytes_read <= 0) {
            usleep(10000); // 10ms
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            if (buffer_pos > 0) {
                buffer[buffer_pos] = '\0';
                parse_and_execute_command(buffer, port->port_id);
                buffer_pos = 0;
            }
        } else if (buffer_pos < (int)sizeof(buffer) - 1) {
            buffer[buffer_pos++] = c;
        }
    }
    
    printf("[USB%d] Listener thread stopped\n", port->port_id);
    return NULL;
}

void usb_start_listener(void) {
    listener_running = true;
    
    int started = 0;
    for (int i = 0; i < MAX_USB_PORTS; i++) {
        if (usb_ports[i].active) {
            if (pthread_create(&usb_ports[i].thread, NULL, usb_listener_loop, &usb_ports[i]) == 0) {
                started++;
                printf("[USB%d] Listener thread created\n", i);
            } else {
                fprintf(stderr, "[USB%d] Failed to create listener thread\n", i);
            }
        }
    }
    
    printf("[USB] Started %d listener threads\n", started);
}

void usb_stop_listener(void) {
    listener_running = false;
    
    for (int i = 0; i < MAX_USB_PORTS; i++) {
        if (usb_ports[i].active) {
            pthread_join(usb_ports[i].thread, NULL);
            printf("[USB%d] Thread joined\n", i);
        }
    }
}