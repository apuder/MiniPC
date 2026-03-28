
#include <kernel.h>
#include <uart.h>


#if 0
#include <vga.h>

/* TOS_IFDEF vga */
#include "vga_test.c"
/* TOS_ENDIF vga */
#endif

#define CLK_FREQ 84000000

void kernel_main()
{
  uart_set_div(CLK_FREQ / 115200.0 + 0.5);
  uart_puts("Hello TOS!\r\n");
  output_string(kernel_window, "Hello TOS!\n");
  while(1);
#if 0
    init_process();
    init_dispatcher();
    init_ipc();
    init_interrupts();
    init_null_process();
    init_timer();
    init_com();

    if (!init_vga()) {
      init_wm();
      init_keyb();
      start_shell();
    }
    /* TOS_IFDEF vga */
    else {
      test_vga();
    }
    /* TOS_ENDIF vga */

    become_zombie();
#endif
}
