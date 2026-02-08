#include "display.h"
#include "tasks.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <pthread.h>

DisplayContext g_display;

bool display_init(DisplayMode mode) {
    g_display.mode = mode;
    g_display.initialized = false;
    
    if (mode == DISPLAY_MODE_FRAMEBUFFER) {
        g_display.fb_fd = open("/dev/fb0", O_RDWR);
        if (g_display.fb_fd < 0) {
            fprintf(stderr, "[DISPLAY] Failed to open framebuffer, falling back to terminal\n");
            g_display.mode = DISPLAY_MODE_TERMINAL;
            g_display.initialized = true;
            return true;
        }
        
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        
        if (ioctl(g_display.fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
            fprintf(stderr, "[DISPLAY] Failed to get variable screen info\n");
            close(g_display.fb_fd);
            g_display.mode = DISPLAY_MODE_TERMINAL;
            g_display.initialized = true;
            return true;
        }
        
        if (ioctl(g_display.fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
            fprintf(stderr, "[DISPLAY] Failed to get fixed screen info\n");
            close(g_display.fb_fd);
            g_display.mode = DISPLAY_MODE_TERMINAL;
            g_display.initialized = true;
            return true;
        }
        
        g_display.fb_size = vinfo.yres_virtual * finfo.line_length;
        
        g_display.fb_ptr = (uint16_t*)mmap(0, g_display.fb_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_SHARED, g_display.fb_fd, 0);
        
        if (g_display.fb_ptr == MAP_FAILED) {
            fprintf(stderr, "[DISPLAY] Failed to mmap framebuffer\n");
            close(g_display.fb_fd);
            g_display.mode = DISPLAY_MODE_TERMINAL;
            g_display.initialized = true;
            return true;
        }
        
        printf("[DISPLAY] Framebuffer initialized (%dx%d)\n", 
               vinfo.xres, vinfo.yres);
        display_clear(COLOR_BLACK);
    } else {
        printf("[DISPLAY] Terminal mode initialized\n");
    }
    
    g_display.initialized = true;
    return true;
}

void display_cleanup(void) {
    if (g_display.mode == DISPLAY_MODE_FRAMEBUFFER && g_display.fb_ptr) {
        munmap(g_display.fb_ptr, g_display.fb_size);
        close(g_display.fb_fd);
    }
    printf("[DISPLAY] Cleaned up\n");
}

void display_clear(uint16_t color) {
    if (g_display.mode != DISPLAY_MODE_FRAMEBUFFER || !g_display.fb_ptr) {
        return;
    }
    
    for (uint32_t i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        g_display.fb_ptr[i] = color;
    }
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (g_display.mode != DISPLAY_MODE_FRAMEBUFFER || !g_display.fb_ptr) {
        return;
    }
    
    for (uint16_t dy = 0; dy < h; dy++) {
        for (uint16_t dx = 0; dx < w; dx++) {
            uint16_t px = x + dx;
            uint16_t py = y + dy;
            if (px < DISPLAY_WIDTH && py < DISPLAY_HEIGHT) {
                g_display.fb_ptr[py * DISPLAY_WIDTH + px] = color;
            }
        }
    }
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (g_display.mode != DISPLAY_MODE_FRAMEBUFFER || !g_display.fb_ptr) {
        return;
    }
    
    // Top and bottom horizontal lines
    for (uint16_t i = 0; i < w; i++) {
        if (x + i < DISPLAY_WIDTH) {
            if (y < DISPLAY_HEIGHT)
                g_display.fb_ptr[y * DISPLAY_WIDTH + x + i] = color;
            if (y + h - 1 < DISPLAY_HEIGHT)
                g_display.fb_ptr[(y + h - 1) * DISPLAY_WIDTH + x + i] = color;
        }
    }
    
    // Left and right vertical lines
    for (uint16_t i = 0; i < h; i++) {
        if (y + i < DISPLAY_HEIGHT) {
            if (x < DISPLAY_WIDTH)
                g_display.fb_ptr[(y + i) * DISPLAY_WIDTH + x] = color;
            if (x + w - 1 < DISPLAY_WIDTH)
                g_display.fb_ptr[(y + i) * DISPLAY_WIDTH + x + w - 1] = color;
        }
    }
}

void display_update_coach_layout(void) {
    if (g_display.mode != DISPLAY_MODE_FRAMEBUFFER) {
        return;
    }

    display_clear(COLOR_BLACK);

    // Draw title area
    display_fill_rect(0, 0, DISPLAY_WIDTH, 30, COLOR_GRAY);

    // Draw cabins (2 rows of 5)
    uint16_t cabin_width = 80;
    uint16_t cabin_height = 100;
    uint16_t margin = 10;
    uint16_t start_y = 50;

    pthread_mutex_lock(&g_system.system_mutex);

    for (int i = 0; i < NUM_CABINS; i++) {
        int row = i / 5;
        int col = i % 5;

        uint16_t x = margin + col * (cabin_width + margin);
        uint16_t y = start_y + row * (cabin_height + margin);

        // Determine cabin color based on state
        uint16_t color = COLOR_GREEN; // Normal

        if (g_system.cabins[i].state == STATE_FIRE) {
            color = COLOR_ORANGE;
        } else if (g_system.cabins[i].state == STATE_EMERGENCY) {
            color = COLOR_RED;
        } else if (g_system.cabins[i].temperature > 26 ||
                   g_system.cabins[i].temperature < 18) {
            color = COLOR_YELLOW;
        } else if (g_system.cabins[i].light_on) {
            color = COLOR_BLUE;
        }

        display_fill_rect(x, y, cabin_width, cabin_height, color);
        display_draw_rect(x, y, cabin_width, cabin_height, COLOR_WHITE);
    }

    pthread_mutex_unlock(&g_system.system_mutex);
}

void display_show_emergency(const char *message) {
    if (g_display.mode == DISPLAY_MODE_FRAMEBUFFER) {
        // Flash red bar at top
        display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_RED);
    }
    
    printf("\n*** %s ***\n", message);
}

void display_terminal_coach_status(void) {
    static int update_counter = 0;

    if (update_counter++ % 5 != 0) {
        return; // Update every 5th call
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         INDIAN RAILWAYS - COACH STATUS DISPLAY            ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");

    pthread_mutex_lock(&g_system.system_mutex);

    for (int i = 0; i < NUM_CABINS; i++) {
        char status[50];

        pthread_mutex_lock(&g_system.cabins[i].mutex);

        if (g_system.cabins[i].state == STATE_FIRE) {
            snprintf(status, sizeof(status), "FIRE ALERT");
        } else if (g_system.cabins[i].state == STATE_EMERGENCY) {
            snprintf(status, sizeof(status), "EMERGENCY");
        } else if (g_system.cabins[i].light_on) {
            snprintf(status, sizeof(status), "Light ON, %d°C",
                    g_system.cabins[i].temperature);
        } else {
            snprintf(status, sizeof(status), "Normal, %d°C",
                    g_system.cabins[i].temperature);
        }

        pthread_mutex_unlock(&g_system.cabins[i].mutex);

        printf("║  Cabin %d: %-45s║\n", i, status);
    }

    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Power: %-10s  Emergency: %-8s  Fire: %-8s   ║\n",
           g_system.power_low ? "LOW" : "NORMAL",
           g_system.emergency_active ? "ACTIVE" : "INACTIVE",
           g_system.fire_active ? "ACTIVE" : "INACTIVE");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    pthread_mutex_unlock(&g_system.system_mutex);
}

// Main render function called by display task
void display_render(void) {
    if (g_display.mode == DISPLAY_MODE_FRAMEBUFFER) {
        display_update_coach_layout();
    } else {
        display_terminal_coach_status();
    }
}

// Display status message
void display_status_message(const char *message) {
    display_show_emergency(message);
}