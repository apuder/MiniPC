/* Copyright 2024 Grug Huhler.  License SPDX BSD-2-Clause.

   This program shows counting on the LEDs of the Tang Nano
   9K FPGA development board.

   Function foo lets one see stores of different size and alignment by
   looking at the wstrb signals from the pico32rv.
*/

#include "leds.h"
#include "ws2812b.h"
#include "uart.h"
#include "countdown_timer.h"

#define MEMSIZE 512
//unsigned int mem[MEMSIZE];
volatile unsigned int* mem = (unsigned int*) 0x40000000;
volatile unsigned int* mem2 = (unsigned int*) 0x40000004;
unsigned int test_vals[] = {0, 0xffffffff, 0xaaaaaaaa, 0x55555555, 0xdeadbeef};

/* A simple memory test.  Delete this and also array mem
   above to free much of the SRAM for other things
*/

typedef struct __attribute__((packed)) {
  unsigned char v1;
  unsigned short v2;
  unsigned int v3;
} test_t;

void (*func_ptr)(void);

static inline void esp_request(volatile unsigned char* esp, unsigned char cmd)
{
  *esp = cmd;
  while (*esp == 0) ;
}

void load_kernel()
{
  volatile unsigned char* spi_out = (unsigned char*) 0x80001100;
  volatile unsigned char* spi_in = (unsigned char*) 0x80001000;
  volatile unsigned char* esp = (unsigned char*) 0x80002000;
  *spi_out = 1; // Open kernel.bin
  esp_request(esp, 3);
  *spi_out = 0; // NOP, fetch results of open
  esp_request(esp, 3);

  uart_puts("spi_in[0..7]: ");
  for (int i = 0; i < 8; i++) {
    uart_print_hex(spi_in[i]);
    uart_puts(i == 7 ? "\r\n" : " ");
  }

  unsigned char err = *spi_in;
  if (err) {
    uart_puts("Error opening kernel.bin\r\n");
    return;
  }
  int size =
      ((int)spi_in[1]) |
      ((int)spi_in[2] << 8) |
      ((int)spi_in[3] << 16) |
      ((int)spi_in[4] << 24);
  uart_puts("kernel.bin size: ");
  uart_print_hex(size);
  uart_puts("\r\n");
  if (size == 0) {
    return;
  }

  *spi_out = 2; // Read next 256 bytes of kernel.bin
  // First response will be empty
  esp_request(esp, 3);

  volatile unsigned char* p = (unsigned char*) 0x40000000;

  while(size > 0) {
    esp_request(esp, 3);
    int max = size > 256 ? 256 : size;
    for (int i = 0; i < max; i++) {
      *p++ = *(spi_in + i);
    }
    size -= max;
  }

  uart_puts("kernel.bin loaded to 0x40000000\r\n");
  func_ptr = (void (*)(void)) 0x40000000;
  func_ptr();
}

int mem_test (void)
{
  // SPI test
  uart_puts("SPI test\r\n");
#if 1
  test_t* t = (test_t*) 0x80001000;
  t->v1 = 0x11;
  t->v2 = 0x2233;
  t->v3 = 0x44556677;
#if 0
  t =  (test_t*) 0x80001100;
  t->v1 = 0x88;
  t->v2 = 0x7766;
  t->v3 = 0x55443322;
#endif
  t = (test_t*) 0x80001000;
  if ((t->v1 != 0x11) || (t->v2 != 0x2233) || (t->v3 != 0x44556677)) {
    uart_puts("SPI test FAILED (0)\r\n");
    uart_puts("v1: ");
    uart_print_hex(t->v1);
    uart_puts(", v2: ");
    uart_print_hex(t->v2);
    uart_puts(", v3: ");
    uart_print_hex(t->v3);
    uart_puts("\r\n");
    return 1;
  }
  t =  (test_t*) 0x80001100;
  t->v1 = 0x88;
  t->v2 = 0x7766;
  t->v3 = 0x55443322;
  if ((t->v1 != 0x88) || (t->v2 != 0x7766) || (t->v3 != 0x55443322)) {
    uart_puts("SPI test FAILED (1)\r\n");
    uart_puts("v1: ");
    uart_print_hex(t->v1);
    uart_puts(", v2: ");
    uart_print_hex(t->v2);
    uart_puts(", v3: ");
    uart_print_hex(t->v3);
    uart_puts("\r\n");
    return 1;
  }
  #endif
  volatile unsigned char* spi = (volatile unsigned char*) 0x80001000;
  *spi = 0x11;
  uart_print_hex(*spi);
  uart_puts("\r\n");
  uart_print_hex(*spi);
  uart_puts("\r\n");

  for (int i = 0; i < 2 * 256; i++) {
    *(spi + i) = (i + 3) % 256;
  }
  for (int i = 0; i < 2 * 256; i++) {
    if (*(spi + i) != (i + 3) % 256) {
      uart_puts("Error at idx ");
      uart_print_hex(i);
      uart_puts(": ");
      uart_print_hex(*(spi + i));
      uart_puts("\r\n");
    }
  }
  return 0;
#if 0
  *mem = 0xdeadbeef;
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned int) *mem);
  uart_puts("\r\n");
#endif
#if 0
  static int flip = 0;

  *mem = flip ? 0x12345678 : 0xdeadbeef;
  flip = !flip;
  *mem2 = 0xdeadbeef;
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned int) *mem);
  uart_puts("\r\n");
  uart_puts("peek(0x40000004): ");
  uart_print_hex((unsigned int) *mem2);
  uart_puts("\r\n");
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned int) *mem);
  uart_puts("\r\n");
#endif
#if 0
  uart_puts("peek(0x40000004): ");
  uart_print_hex((unsigned int) *mem2);
  uart_puts("\r\n");
  *mem = 0x87654321;
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned int) *mem);
  uart_puts("\r\n");
  unsigned char* b = (unsigned char*) mem;
  *b++ = 0xaa;
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned char) *b);
  uart_puts("\r\n");
  *b++ = 0xbb;
  uart_puts("peek(0x40000001): ");
  uart_print_hex((unsigned char) *b);
  uart_puts("\r\n");
  *b++ = 0xcc;
  uart_puts("peek(0x40000002): ");
  uart_print_hex((unsigned char) *b);
  uart_puts("\r\n");
  uart_puts("peek(0x40000000): ");
  uart_print_hex((unsigned int) *mem);
  uart_puts("\r\n\r\n");
  
#if 1
  uart_puts("Testing display memory: ");
  b = (unsigned char*) 0x50000000;
  for (int i = 0; i < 27; i++) *(b + i) = i + 65;
  for (int i = 0; i < 27; i++) {
    uart_print_hex(*(b + i));
    uart_puts(" ");
  }
  uart_puts("\r\n");
#endif
  //return 0;
#endif
#if 0
  int i, test, errors;
  unsigned int val, val_read;

  errors = 0;
  for (test = 0; test < sizeof(test_vals)/sizeof(test_vals[0]); test++) {

    for (i = 0; i < MEMSIZE; i++) mem[i] = test_vals[test];

    for (i = 0; i < MEMSIZE; i++) {
      val_read = mem[i];
      if (val_read != test_vals[test]) errors += 1;
    }
  }

  for (i = 0; i < MEMSIZE; i++) mem[i] = i + (i << 17);

  for (i = 0; i < MEMSIZE; i++) {
    val_read = mem[i];
    if (val_read != i + (i << 17)) errors += 1;
  }

  return(errors);
#endif
}

/* The picorv32 core implements several counters and
   instructions to access them.  These are part of the
   risc-v specification.  Function readtime uses one
   of them.
*/

static inline unsigned int readtime(void)
{
  unsigned int val;
  unsigned long long jj;
  asm volatile("rdtime %0" : "=r" (val));
  return val;

}

void endian_test(void)
{
  volatile unsigned int test_loc = 0;
  volatile unsigned int *addr = &test_loc;
  volatile unsigned char *cp0, *cp3;
  char byte0, byte3;
  unsigned int i, ok;

  cp0 = (volatile unsigned char *) addr;
  cp3 = cp0 + 3;
  *addr = 0x44332211;
  byte0 = *cp0;
  byte3 = *cp3;
  *cp3 = 0xab;
  i = *addr;

  ok = (byte0 == 0x11) && (byte3 == 0x44) && (i == 0xab332211);
  uart_puts("\r\nEndian test: at ");
  uart_print_hex((unsigned int) addr);
  uart_puts(", byte0: ");
  uart_print_hex((unsigned int) byte0);
  uart_puts(", byte3: ");
  uart_print_hex((unsigned int) byte3);
  uart_puts(",\r\n     word: ");
  uart_print_hex(i);
  if (ok)
    uart_puts(" [PASSED]\r\n");
  else
    uart_puts(" [FAILED]\r\n");
}

void uart_rx_test(void)
{
  char buf[5];
  int i;
  
  uart_puts("Type 4 characters (they will echo): ");
  for (i = 0; i < 4; i++) {
    buf[i] = uart_getchar();
    uart_putchar(buf[i]);
  }
  buf[4] = 0;
  uart_puts("\r\nUART read: ");
  uart_puts(buf);
  uart_puts("\r\n");
}

/* la_functions are useful for looking at the bus using a logic
   analyzer.
*/

void la_wtest(void)
{
  unsigned int v;
  volatile unsigned int *ip = (volatile unsigned int *) &v;
  volatile unsigned short *sp = (volatile unsigned short *) &v;
  volatile unsigned char *cp = (volatile unsigned char *) &v;

  *ip = 0x03020100;  // addr 0x00

  *sp = 0x0302;      // addr 0x00
  *(sp+1) = 0x0100;  // addr 0x02

  *cp = 0x03;        // addr 0x00
  *(cp+1) = 0x02;    // addr 0x01
  *(cp+2) = 0x01;    // addr 0x02
  *(cp+3) = 0x00;    // addr 0x03
}


void la_rtest(void)
{
  unsigned int v;
  volatile unsigned int *ip = (volatile unsigned int *) &v;
  volatile unsigned short *sp = (volatile unsigned short *) &v;
  volatile unsigned char *cp = (volatile unsigned char *) &v;

  *ip = 0x03020100;  // addr 0x00

  *ip;     // addr 0x00

  *sp;     // addr 0x00
  *(sp+1); // addr 0x02

  *cp;     // addr 0x00
  *(cp+1); // addr 0x01
  *(cp+2); // addr 0x02
  *(cp+3); // addr 0x03
}


void cdt_test(void)
{
  unsigned int val;
  unsigned int test_errors = 0;
  
  // If register is little-endian, write to 0x80000013 should set
  // the MSB,  Does it?

  cdt_wbyte3(0xff);
  val = cdt_read();
  if ((val == 0xff000000) || (val < 0xfe000000)) test_errors = 1;

  // Write zero to most significant half-word.
  cdt_whalf2(0);
  val = cdt_read();
  if (val > 0xffff) test_errors |= 2;

  uart_puts("Countdown timer test ");
  if (test_errors) {
    uart_puts("FAILED, mask = ");
    uart_print_hex(test_errors);
    uart_puts("\r\n");
  } else {
    uart_puts("PASSED\r\n");
  }
}

int main()
{
  unsigned char v, ch;

  set_leds(6);
  la_wtest();
  la_rtest();

  uart_set_div(CLK_FREQ/115200.0 + 0.5);
  
  uart_puts("\r\nStarting, CLK_FREQ: 0x");
  uart_print_hex(CLK_FREQ);
  uart_puts("\r\n\r\n");

  //mem_test();

  while (1) {
    uart_puts("Enter command:\r\n");
    uart_puts("   c: countdown timer test\r\n");
    uart_puts("   d: delay 3 seconds\r\n");
    uart_puts("   e: endian test\r\n");
    uart_puts("   g: read LED value\r\n");
    uart_puts("   i: increment LED value\r\n");
    uart_puts("   l: set RGB LED\r\n");
    uart_puts("   m: memory test\r\n");
    uart_puts("   r: read clock\r\n");
    uart_puts("   k: load kernel.bin\r\n");
    ch = uart_getchar();
    switch (ch) {
    case 'c':
      cdt_test();
      break;
    case 'e':
      endian_test();
      break;
    case 'd':
      cdt_delay(3*CLK_FREQ);
      uart_puts("delay done\r\n");
      break;
    case 'g':
      v = get_leds();
      uart_puts("LED = ");
      uart_print_hex(v);
      uart_puts("\r\n");
      break;
    case 'i':
      v = get_leds();
      set_leds(v+1);
      break;
    case 'l':
      uart_puts(" enter 6 hex digits: ");
      set_ws2812b(uart_get_hex());
      uart_puts("\r\n");
      break;
    case 'm':
      if (mem_test())
	uart_puts("memory test FAILED.\r\n");
      else
	uart_puts("memory test PASSED.\r\n");
      break;
    case 'r':
      uart_puts("time is ");
      uart_print_hex(readtime());
      uart_puts("\r\n");
      break;
    case 'k':
      load_kernel();
      break;
    default:
      uart_puts("  Try again...\r\n");
      break;
    }
  }

  return 0;
}
