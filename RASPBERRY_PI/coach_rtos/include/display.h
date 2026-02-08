#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

// Display dimensions
#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

// Color definitions (RGB565 format)
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_RED     0xF800
#define COLOR_ORANGE  0xFC00
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define COLOR_GRAY    0x7BEF

// Display modes
typedef enum {
    DISPLAY_MODE_TERMINAL,
    DISPLAY_MODE_FRAMEBUFFER
} DisplayMode;

// Display context
typedef struct {
    DisplayMode mode;
    int fb_fd;
    uint16_t *fb_ptr;
    uint32_t fb_size;
    bool initialized;
} DisplayContext;

extern DisplayContext g_display;

// Display API
bool display_init(DisplayMode mode);
void display_cleanup(void);
void display_clear(uint16_t color);
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_update_coach_layout(void);
void display_show_emergency(const char *message);
void display_terminal_coach_status(void);
void display_render(void); // Main render function called by tasks
void display_status_message(const char *message); // Display status messages

#endif // DISPLAY_H