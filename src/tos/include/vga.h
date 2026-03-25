
#ifndef __VGA_H__
#define __VGA_H__

#include <kernel.h>

#define VGA_CREATE_WINDOW 0

typedef struct {
  // Input
  const char* title;
  int x, y, width, height;
  // Output
  unsigned int window_id;
} PARAM_VGA_CREATE_WINDOW;

#define VGA_DRAW_TEXT 1

typedef struct {
  // Input
  unsigned int window_id;
  int x, y;
  const char* text;
  int fg_color;
  int bg_color;
} PARAM_VGA_DRAW_TEXT;

#define VGA_DRAW_PIXEL 2

typedef struct {
  // Input
  unsigned int window_id;
  int x, y;
  int color;
} PARAM_VGA_DRAW_PIXEL;

#define VGA_DRAW_LINE 3

typedef struct {
  // Input
  unsigned int window_id;
  int x0, y0, x1, y1;
  int color;
} PARAM_VGA_DRAW_LINE;

typedef struct {
  int cmd;
  union {
    PARAM_VGA_CREATE_WINDOW create_window;
    PARAM_VGA_DRAW_TEXT draw_text;
    PARAM_VGA_DRAW_PIXEL draw_pixel;
    PARAM_VGA_DRAW_LINE draw_line;
  } u;
} VGA_WINDOW_MSG;

extern PORT vga_port;

int init_vga();

#endif
