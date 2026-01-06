#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

// Display Configuration
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 320
#define CABIN_WIDTH 45
#define CABIN_HEIGHT 60

// Color Definitions (RGB565 format for framebuffer)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_RED       0xF800
#define COLOR_ORANGE    0xFD20

// Display Functions
int display_init();
void display_cleanup();
void display_update();
void display_clear();
void display_cabin(int cabin_id);
void display_header();
void display_status_message(const char* message);
void display_terminal_update();

// Terminal-based display (fallback)
void terminal_display_system_state();

#endif // DISPLAY_H