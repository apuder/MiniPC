
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
    init_keyb();
    start_shell();

    become_zombie();
}
