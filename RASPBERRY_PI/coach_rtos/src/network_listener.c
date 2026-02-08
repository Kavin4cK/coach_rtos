#include "tasks.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#define NETWORK_PORT 5000
#define MAX_CLIENTS 3
#define BUFFER_SIZE 256

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    int active;
    pthread_t thread;
    int client_id;
} NetworkClient;

static int server_socket = -1;
static NetworkClient clients[MAX_CLIENTS];
static bool listener_running = false;
static pthread_t accept_thread;
static pthread_mutex_t command_lock = PTHREAD_MUTEX_INITIALIZER;

// Command parsing (same as USB listener)
void network_send_response(int client_id, const char *response) {
    if (clients[client_id].active && clients[client_id].socket_fd >= 0) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s\n", response);
        send(clients[client_id].socket_fd, buffer, strlen(buffer), 0);
    }
}

void network_parse_command(const char *cmd_str, int client_id) {
    char cmd[BUFFER_SIZE];
    strncpy(cmd, cmd_str, BUFFER_SIZE - 1);
    cmd[BUFFER_SIZE - 1] = '\0';
    
    // Remove trailing newline
    char *newline = strchr(cmd, '\n');
    if (newline) *newline = '\0';
    char *carriage = strchr(cmd, '\r');
    if (carriage) *carriage = '\0';
    
    if (strlen(cmd) == 0) {
        return;
    }
    
    printf("[NET%d RX] %s\n", client_id, cmd);
    
    pthread_mutex_lock(&command_lock);
    
    char *token = strtok(cmd, " ");
    if (!token) {
        network_send_response(client_id, "ERROR: Invalid format");
        pthread_mutex_unlock(&command_lock);
        return;
    }
    
    if (strcmp(token, "LIGHT") == 0) {
        int cabin_id = atoi(strtok(NULL, " "));
        char *state = strtok(NULL, " ");
        
        if (cabin_id >= 0 && cabin_id < NUM_CABINS && state) {
            if (strcmp(state, "ON") == 0) {
                cabin_set_light(cabin_id, true);
                network_send_response(client_id, "OK: Light ON");
            } else if (strcmp(state, "OFF") == 0) {
                cabin_set_light(cabin_id, false);
                network_send_response(client_id, "OK: Light OFF");
            } else {
                network_send_response(client_id, "ERROR: Use ON or OFF");
            }
        } else {
            network_send_response(client_id, "ERROR: Invalid cabin ID");
        }
    }
    else if (strcmp(token, "TEMP") == 0) {
        int cabin_id = atoi(strtok(NULL, " "));
        int temp = atoi(strtok(NULL, " "));
        
        if (cabin_id >= 0 && cabin_id < NUM_CABINS && temp >= 10 && temp <= 35) {
            cabin_set_temperature(cabin_id, temp);
            network_send_response(client_id, "OK: Temperature set");
        } else if (cabin_id < 0 || cabin_id >= NUM_CABINS) {
            network_send_response(client_id, "ERROR: Invalid cabin ID");
        } else {
            network_send_response(client_id, "ERROR: Temperature range 10-35°C");
        }
    }
    else if (strcmp(token, "EMERGENCY") == 0) {
        int cabin_id = atoi(strtok(NULL, " "));
        
        if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
            cabin_set_emergency(cabin_id, true);
            network_send_response(client_id, "OK: Emergency triggered");
        } else {
            network_send_response(client_id, "ERROR: Invalid cabin ID");
        }
    }
    else if (strcmp(token, "FIRE") == 0) {
        int cabin_id = atoi(strtok(NULL, " "));
        
        if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
            cabin_set_fire(cabin_id, true);
            network_send_response(client_id, "OK: Fire alert triggered");
        } else {
            network_send_response(client_id, "ERROR: Invalid cabin ID");
        }
    }
    else if (strcmp(token, "POWER") == 0) {
        char *state = strtok(NULL, " ");
        
        if (state) {
            if (strcmp(state, "LOW") == 0) {
                system_set_power_low(true);
                network_send_response(client_id, "OK: Power LOW");
            } else if (strcmp(state, "NORMAL") == 0) {
                system_set_power_low(false);
                network_send_response(client_id, "OK: Power NORMAL");
            } else {
                network_send_response(client_id, "ERROR: Use LOW or NORMAL");
            }
        }
    }
    else if (strcmp(token, "CHAIN") == 0) {
        char *action = strtok(NULL, " ");
        
        if (action && strcmp(action, "PULL") == 0) {
            system_set_chain_pull(true);
            network_send_response(client_id, "OK: Chain pulled");
        } else {
            network_send_response(client_id, "ERROR: Use PULL");
        }
    }
    else if (strcmp(token, "CLEAR") == 0) {
        char *target = strtok(NULL, " ");
        
        if (target && strcmp(target, "ALL") == 0) {
            system_clear_all_emergencies();
            network_send_response(client_id, "OK: All emergencies cleared");
        } else if (target) {
            int cabin_id = atoi(target);
            if (cabin_id >= 0 && cabin_id < NUM_CABINS) {
                cabin_clear_emergency(cabin_id);
                network_send_response(client_id, "OK: Emergency cleared");
            } else {
                network_send_response(client_id, "ERROR: Invalid cabin ID");
            }
        } else {
            network_send_response(client_id, "ERROR: Use CLEAR ALL or CLEAR <cabin_id>");
        }
    }
    else {
        network_send_response(client_id, "ERROR: Unknown command");
    }
    
    pthread_mutex_unlock(&command_lock);
}

void* network_client_handler(void *arg) {
    NetworkClient *client = (NetworkClient *)arg;
    char buffer[BUFFER_SIZE];
    
    printf("[NET%d] Client connected from %s:%d\n",
           client->client_id,
           inet_ntoa(client->address.sin_addr),
           ntohs(client->address.sin_port));
    
    while (listener_running && client->active) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client->socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("[NET%d] Client disconnected\n", client->client_id);
            } else {
                printf("[NET%d] Error: %s\n", client->client_id, strerror(errno));
            }
            break;
        }
        
        buffer[bytes] = '\0';
        network_parse_command(buffer, client->client_id);
    }
    
    close(client->socket_fd);
    client->active = false;
    client->socket_fd = -1;
    
    return NULL;
}

void* network_accept_loop(void *arg) {
    (void)arg;
    
    printf("[NETWORK] Listening on port %d...\n", NETWORK_PORT);
    
    while (listener_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        
        if (client_fd < 0) {
            if (listener_running) {
                fprintf(stderr, "[NETWORK] Accept failed: %s\n", strerror(errno));
            }
            continue;
        }
        
        // Find available slot
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                slot = i;
                break;
            }
        }
        
        if (slot >= 0) {
            clients[slot].socket_fd = client_fd;
            clients[slot].address = client_addr;
            clients[slot].active = true;
            clients[slot].client_id = slot;
            
            if (pthread_create(&clients[slot].thread, NULL, network_client_handler, &clients[slot]) != 0) {
                fprintf(stderr, "[NETWORK] Failed to create client thread\n");
                close(client_fd);
                clients[slot].active = false;
            }
        } else {
            printf("[NETWORK] Max clients reached, rejecting connection\n");
            close(client_fd);
        }
    }
    
    return NULL;
}

int network_init(void) {
    // Initialize client structures
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = -1;
        clients[i].active = false;
        clients[i].client_id = i;
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        fprintf(stderr, "[NETWORK] Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "[NETWORK] setsockopt failed: %s\n", strerror(errno));
        close(server_socket);
        return -1;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(NETWORK_PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "[NETWORK] Bind failed: %s\n", strerror(errno));
        close(server_socket);
        return -1;
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        fprintf(stderr, "[NETWORK] Listen failed: %s\n", strerror(errno));
        close(server_socket);
        return -1;
    }
    
    printf("[NETWORK] Server initialized on port %d\n", NETWORK_PORT);
    return 0;
}

void network_start_listener(void) {
    if (listener_running) {
        printf("[NETWORK] Already running\n");
        return;
    }
    
    listener_running = true;
    
    if (pthread_create(&accept_thread, NULL, network_accept_loop, NULL) != 0) {
        fprintf(stderr, "[NETWORK] Failed to create accept thread\n");
        listener_running = false;
    }
}

void network_stop_listener(void) {
    if (!listener_running) {
        return;
    }
    
    printf("[NETWORK] Stopping listener...\n");
    listener_running = false;
    
    // Close server socket to unblock accept
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
    
    // Wait for accept thread
    pthread_join(accept_thread, NULL);
    
    // Close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            clients[i].active = false;
            if (clients[i].socket_fd >= 0) {
                close(clients[i].socket_fd);
            }
            pthread_join(clients[i].thread, NULL);
        }
    }
    
    printf("[NETWORK] Listener stopped\n");
}

void network_close(void) {
    network_stop_listener();
}
