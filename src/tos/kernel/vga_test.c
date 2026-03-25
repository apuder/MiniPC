
#include <kernel.h>

#define WHITE 0x3f

#define GENERATE_REFERENCE_SCREENSHOT

#ifdef GENERATE_REFERENCE_SCREENSHOT

void test_vga()
{
  VGA_WINDOW_MSG msg;

  // Create Window 1
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 1";
  msg.u.create_window.x = 50;
  msg.u.create_window.y = 50;
  msg.u.create_window.width = 100;
  msg.u.create_window.height = 50;
  send(vga_port, &msg);
  unsigned int window1_id = msg.u.create_window.window_id;

  // Create Window 2
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 2";
  msg.u.create_window.x = 10;
  msg.u.create_window.y = 120;
  msg.u.create_window.width = 150;
  msg.u.create_window.height = 60;
  send(vga_port, &msg);
  unsigned int window2_id = msg.u.create_window.window_id;

  // Create Window 3
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 3";
  msg.u.create_window.x = 180;
  msg.u.create_window.y = 30;
  msg.u.create_window.width = 100;
  msg.u.create_window.height = 100;
  send(vga_port, &msg);
  unsigned int window3_id = msg.u.create_window.window_id;

  // Draw some lines in Window 1
  char current_color = 0;
  for (int x = 0; x < 100; x += 2) {
    msg.cmd = VGA_DRAW_LINE;
    msg.u.draw_line.window_id = window1_id;
    msg.u.draw_line.x0 = x;
    msg.u.draw_line.y0 = 0;
    msg.u.draw_line.x1 = 100 - x;
    msg.u.draw_line.y1 = 49;
    msg.u.draw_line.color = current_color++;
    send(vga_port, &msg);
  }

  // Write some text in Window 2
  msg.cmd = VGA_DRAW_TEXT;
  msg.u.draw_text.window_id = window2_id;
  msg.u.draw_text.text = "Hello CSC720!";
  msg.u.draw_text.x = 1;
  msg.u.draw_text.y = 1;
  msg.u.draw_text.fg_color = 0x3f; // White
  msg.u.draw_text.bg_color = 0;
  send(vga_port, &msg);

  // Write some text in Window 2 that will be clipped
  msg.cmd = VGA_DRAW_TEXT;
  msg.u.draw_text.window_id = window2_id;
  msg.u.draw_text.text = "Text that is too long will be clipped";
  msg.u.draw_text.x = 20;
  msg.u.draw_text.y = 20;
  msg.u.draw_text.fg_color = 0x3f; // White
  msg.u.draw_text.bg_color = 0;
  send(vga_port, &msg);

  // Draw some random pixels in Window 3
  msg.cmd = VGA_DRAW_PIXEL;
  msg.u.draw_pixel.window_id = window3_id;
  current_color = 0;
  for (int x = 3; x < 100; x += 5) {
    for (int y = 3; y < 100; y += 5) {
      msg.u.draw_pixel.x = x;
      msg.u.draw_pixel.y = y;
      msg.u.draw_pixel.color = current_color;
      current_color = (current_color + 1) % 64;
      send(vga_port, &msg);
    }
  }
}

#else

void test_vga()
{
  VGA_WINDOW_MSG msg;

  // Create Window 1
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 1";
  msg.u.create_window.x = 20;
  msg.u.create_window.y = 20;
  msg.u.create_window.width = 70;
  msg.u.create_window.height = 30;
  send(vga_port, &msg);
  unsigned int window1_id = msg.u.create_window.window_id;

  // Create Window 2
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 2";
  msg.u.create_window.x = 100;
  msg.u.create_window.y = 20;
  msg.u.create_window.width = 120;
  msg.u.create_window.height = 45;
  send(vga_port, &msg);
  unsigned int window2_id = msg.u.create_window.window_id;

  // Create Window 3
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 3";
  msg.u.create_window.x = 20;
  msg.u.create_window.y = 90;
  msg.u.create_window.width = 110;
  msg.u.create_window.height = 100;
  send(vga_port, &msg);
  unsigned int window3_id = msg.u.create_window.window_id;

  // Create Window 4
  msg.cmd = VGA_CREATE_WINDOW;
  msg.u.create_window.title = "Window 4";
  msg.u.create_window.x = 150;
  msg.u.create_window.y = 90;
  msg.u.create_window.width = 150;
  msg.u.create_window.height = 90;
  send(vga_port, &msg);
  unsigned int window4_id = msg.u.create_window.window_id;

  char current_color = 0;

  msg.cmd = VGA_DRAW_PIXEL;
  msg.u.draw_pixel.window_id = window1_id;
  current_color = 0;
  for (int x = 0; x < 100; x++) {
    msg.u.draw_pixel.x = x + 30;
    msg.u.draw_pixel.y = x + 30;
    msg.u.draw_pixel.color = current_color;
    current_color = (current_color + 1) % 64;
    send(vga_port, &msg);
  }
  for (int x = 0; x < 20; x++) {
    msg.u.draw_pixel.x = x;
    msg.u.draw_pixel.y = x;
    msg.u.draw_pixel.color = current_color;
    current_color = (current_color + 1) % 64;
    send(vga_port, &msg);
  }

  current_color = 0;
  msg.cmd = VGA_DRAW_LINE;
  msg.u.draw_line.window_id = window2_id;
  msg.u.draw_line.x0 = 0;
  msg.u.draw_line.y0 = 0;
  msg.u.draw_line.x1 = 119;
  msg.u.draw_line.y1 = 44;
  msg.u.draw_line.color = WHITE;
  send(vga_port, &msg);
  msg.u.draw_line.x0 = 100;
  msg.u.draw_line.y0 = 44;
  msg.u.draw_line.x1 = 10;
  msg.u.draw_line.y1 = 0;
  send(vga_port, &msg);
  msg.u.draw_line.x0 = 20;
  msg.u.draw_line.y0 = 70;
  msg.u.draw_line.x1 = 140;
  msg.u.draw_line.y1 = 5;
  send(vga_port, &msg);

  current_color = 0;
  for (int x = 0; x < 200; x += 5) {
    msg.cmd = VGA_DRAW_TEXT;
    msg.u.draw_text.window_id = window4_id;
    msg.u.draw_text.text = "Hello CSC720!";
    msg.u.draw_text.x = x;
    msg.u.draw_text.y = x;
    msg.u.draw_text.fg_color = current_color;
    msg.u.draw_text.bg_color = 0;
    send(vga_port, &msg);
    current_color = (current_color + 1) % 64;
  }

  msg.cmd = VGA_DRAW_PIXEL;
  msg.u.draw_pixel.window_id = window3_id;
  for (int x = 0; x < 110; x++) {
    for (int y = 0; y < 100; y++) {
      msg.u.draw_pixel.x = x;
      msg.u.draw_pixel.y = y;
      msg.u.draw_pixel.color = current_color;
      current_color = (current_color + 1) % 64;
      send(vga_port, &msg);
    }
  }

  msg.cmd = VGA_DRAW_TEXT;
  msg.u.draw_text.window_id = window3_id;
  msg.u.draw_text.text = "Hello World!";
  msg.u.draw_text.x = 5;
  msg.u.draw_text.y = 20;
  msg.u.draw_text.fg_color = 0;
  msg.u.draw_text.bg_color = WHITE;
  send(vga_port, &msg);
}

#endif
