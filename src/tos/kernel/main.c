
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

    init_process();
    init_dispatcher();
    init_ipc();
    init_interrupts();
    init_null_process();
    init_timer();
    //init_com();

    init_wm();
    //init_keyb();
    //start_shell();

    int window_id = wm_create(10, 3, 50, 17);
    wm_print(window_id, "Welcome to TOS!\n");

    become_zombie();
}
