
#include <kernel.h>
#include <vga.h>

/* TOS_IFDEF vga */
#include "vga_test.c"
/* TOS_ENDIF vga */

void kernel_main()
{
    // this turns off the VGA hardware cursor
    // otherwise we get an annoying, meaningless,
    // blinking cursor in the middle of our screen
    outportb(0x03D4, 0x0E);
    outportb(0x03D5, 0xFF);
    outportb(0x03D4, 0x0F);
    outportb(0x03D5, 0xFF);

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
}
