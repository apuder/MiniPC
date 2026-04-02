#include <kernel.h>

BOOL interrupts_initialized = FALSE;

static PROCESS interrupt_table[MAX_INTERRUPTS];

/*
 * Unmask only external IRQ bit 7 (machine timer source in this system)
 * using PicoRV32's custom maskirq instruction.
 */
static void unmask_timer_irq(void)
{
    register unsigned irq_mask asm("a0") = 0xFFFFFF7Fu;
    /* 0x0605000b encodes PicoRV32 custom maskirq rs1=a0, rd=x0: write new IRQ mask from a0, discard old mask. */
    asm volatile(".word 0x0605000b" : "+r"(irq_mask) :: "memory");
}

/*
 * Timer ISR entrypoint.
 * Platform trap glue should call this when machine timer IRQ is raised.
 */
void isr_timer()
{
    PROCESS p = interrupt_table[TIMER_IRQ];

    if (p && p->state == STATE_INTR_BLOCKED) {
        add_ready_queue(p);
    }

    /* Always select the next runnable process after a timer tick. */
    active_proc = dispatcher();
}

void wait_for_interrupt(int intr_no)
{
    volatile int flag;

    if (intr_no != TIMER_IRQ) {
        panic("wait_for_interrupt(): only TIMER_IRQ is supported");
    }

    DISABLE_INTR(flag);
    if (interrupt_table[intr_no] != NULL) {
        panic("wait_for_interrupt(): ISR busy");
    }

    interrupt_table[intr_no] = active_proc;
    remove_ready_queue(active_proc);
    active_proc->state = STATE_INTR_BLOCKED;
    resign();
    interrupt_table[intr_no] = NULL;
    ENABLE_INTR(flag);
}

void init_interrupts()
{
    int i;

    for (i = 0; i < MAX_INTERRUPTS; i++) {
        interrupt_table[i] = NULL;
    }

    unmask_timer_irq();
    interrupts_initialized = TRUE;
}
