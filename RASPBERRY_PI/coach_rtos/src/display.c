#include "display.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

// Framebuffer variables
static int fb_fd = -1;
static uint16_t* fb_ptr = NULL;
static size_t fb_size = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static bool use_terminal_only = false;

// Initialize display (framebuffer or terminal)
int display_init() {
    // Try to open framebuffer
    fb_fd = open("/dev/fb0", O_RDWR);
    
    if (fb_fd < 0) {
        log_message("Cannot open framebuffer, using terminal mode only");
        use_terminal_only = true;
        return 0;
    }
    
    // Get screen info
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        log_message("Error reading framebuffer info");
        close(fb_fd);
        fb_fd = -1;
        use_terminal_only = true;
        return -1;
    }
    
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        log_message("Error reading fixed framebuffer info");
        close(fb_fd);
        fb_fd = -1;
        use_terminal_only = true;
        return -1;
    }
    
    // Map framebuffer to memory
    fb_size = vinfo.yres_virtual * finfo.line_length;
    fb_ptr = (uint16_t*)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    
    if (fb_ptr == MAP_FAILED) {
        log_message("Error mapping framebuffer");
        close(fb_fd);
        fb_fd = -1;
        use_terminal_only = true;
        return -1;
    }
    
    log_message("Framebuffer initialized: %dx%d, %d bpp", 
                vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    
    display_clear();
    return 0;
}

// Cleanup display
void display_cleanup() {
    if (fb_ptr && fb_ptr != MAP_FAILED) {
        munmap(fb_ptr, fb_size);
    }
    
    if (fb_fd >= 0) {
        close(fb_fd);
    }
    
    log_message("Display cleaned up");
}

// Clear display
void display_clear() {
    if (use_terminal_only || !fb_ptr) return;
    
    memset(fb_ptr, 0, fb_size);
}

// Draw a filled rectangle
static void draw_rect(int x, int y, int w, int h, uint16_t color) {
    if (use_terminal_only || !fb_ptr) return;
    
    for (int j = y; j < y + h && j < vinfo.yres; j++) {
        for (int i = x; i < x + w && i < vinfo.xres; i++) {
            fb_ptr[j * vinfo.xres + i] = color;
        }
    }
}

// Get color for cabin state
static uint16_t get_cabin_color(CabinState state) {
    switch (state) {
        case STATE_NORMAL: return COLOR_GREEN;
        case STATE_LIGHT_ON: return COLOR_BLUE;
        case STATE_TEMP_ADJUST: return COLOR_YELLOW;
        case STATE_EMERGENCY: return COLOR_RED;
        case STATE_FIRE: return COLOR_ORANGE;
        default: return COLOR_WHITE;
    }
}

// Display header
void display_header() {
    if (use_terminal_only) return;
    
    draw_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_BLUE);
}

// Display a single cabin
void display_cabin(int cabin_id) {
    if (use_terminal_only || cabin_id < 0 || cabin_id >= NUM_CABINS) return;
    
    int x = 10 + (cabin_id % 5) * (CABIN_WIDTH + 10);
    int y = 60 + (cabin_id / 5) * (CABIN_HEIGHT + 15);
    
    pthread_mutex_lock(&g_system.cabins[cabin_id].mutex);
    uint16_t color = get_cabin_color(g_system.cabins[cabin_id].state);
    pthread_mutex_unlock(&g_system.cabins[cabin_id].mutex);
    
    draw_rect(x, y, CABIN_WIDTH, CABIN_HEIGHT, color);
    draw_rect(x + 2, y + 2, CABIN_WIDTH - 4, CABIN_HEIGHT - 4, COLOR_BLACK);
    draw_rect(x + 4, y + 4, CABIN_WIDTH - 8, CABIN_HEIGHT - 8, color);
}

// Update entire display
void display_update() {
    if (use_terminal_only) {
        terminal_display_system_state();
        return;
    }
    
    display_clear();
    display_header();
    
    for (int i = 0; i < NUM_CABINS; i++) {
        display_cabin(i);
    }
}

// Display status message
void display_status_message(const char* message) {
    log_message("STATUS: %s", message);
    
    if (!use_terminal_only) {
        // Draw message area at bottom
        draw_rect(0, DISPLAY_HEIGHT - 40, DISPLAY_WIDTH, 40, COLOR_RED);
    }
}

// Terminal-based display update
void display_terminal_update() {
    // Just log, don't flood terminal
    static int update_count = 0;
    update_count++;
    
    if (update_count % 10 == 0) {
        // Periodic summary
        terminal_display_system_state();
    }
}

// Terminal display system state
void terminal_display_system_state() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           COACH SYSTEM STATUS - TERMINAL VIEW                ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    pthread_mutex_lock(&g_system.system_mutex);
    
    printf("\nSystem Flags:\n");
    printf("  Emergency Active: %s\n", g_system.emergency_active ? "YES" : "NO");
    printf("  Fire Active:      %s\n", g_system.fire_active ? "YES" : "NO");
    printf("  Power Low:        %s\n", g_system.power_low ? "YES" : "NO");
    
    printf("\nCabin Status:\n");
    printf("┌──────┬────────┬──────────┬─────────────┐\n");
    printf("│ Cabin│ Light  │ Temp(°C) │   State     │\n");
    printf("├──────┼────────┼──────────┼─────────────┤\n");
    
    for (int i = 0; i < NUM_CABINS; i++) {
        const char* state_str;
        const char* state_icon;
        
        switch (g_system.cabins[i].state) {
            case STATE_NORMAL:
                state_str = "Normal";
                state_icon = "✓";
                break;
            case STATE_LIGHT_ON:
                state_str = "Light On";
                state_icon = "💡";
                break;
            case STATE_TEMP_ADJUST:
                state_str = "Temp Adj";
                state_icon = "🌡";
                break;
            case STATE_EMERGENCY:
                state_str = "EMERGENCY";
                state_icon = "⚠";
                break;
            case STATE_FIRE:
                state_str = "FIRE";
                state_icon = "🔥";
                break;
            default:
                state_str = "Unknown";
                state_icon = "?";
                break;
        }
        
        printf("│  %2d  │  %3s   │   %3d    │ %s %-10s │\n",
               i,
               g_system.cabins[i].light_on ? "ON" : "OFF",
               g_system.cabins[i].temperature,
               state_icon,
               state_str);
    }
    
    printf("└──────┴────────┴──────────┴─────────────┘\n");
    
    pthread_mutex_unlock(&g_system.system_mutex);
    
    printf("\n");
}