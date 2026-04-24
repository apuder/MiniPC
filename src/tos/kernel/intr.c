#include <kernel.h>

BOOL interrupts_initialized = FALSE;

static PROCESS interrupt_table[MAX_INTERRUPTS];

/*
 * Unmask supported external IRQ sources.
 * using PicoRV32's custom maskirq instruction.
 */
static void unmask_supported_irqs(void)
{
    register unsigned irq_mask asm("a0") = IRQ_MASK_ALL_ENABLED;

    /* 0x0605000b encodes PicoRV32 custom maskirq rs1=a0, rd=x0: write new IRQ mask from a0, discard old mask. */
    asm volatile(".word 0x0605000b" : "+r"(irq_mask) :: "memory");
}

static inline void wake_waiting_process(int intr_no)
{
    PROCESS p = interrupt_table[intr_no];

    if (p && p->state == STATE_INTR_BLOCKED) {
        add_ready_queue(p);
        interrupt_table[intr_no] = NULL;
    }
}

/*
 * Timer ISR entrypoint.
 * Platform trap glue should call this when machine timer IRQ is raised.
 */
static inline void isr_timer()
{
    wake_waiting_process(TIMER_IRQ);
}

static inline void isr_uart()
{
    wake_waiting_process(UART_IRQ);
}

static inline void isr_uart2()
{
    wake_waiting_process(UART2_IRQ);
}

static inline void isr_keyb()
{
    wake_waiting_process(KEYB_IRQ);
}

void isr_handle_pending(unsigned int pending_irqs)
{
    if (pending_irqs & (1u << TIMER_IRQ)) {
        isr_timer();
    }

    if (pending_irqs & (1u << UART_IRQ)) {
        isr_uart();
    }

    if (pending_irqs & (1u << UART2_IRQ)) {
        isr_uart2();
    }

    if (pending_irqs & (1u << KEYB_IRQ)) {
        isr_keyb();
    }

    /* Always select the next runnable process after handling an IRQ. */
    active_proc = dispatcher();
}

void wait_for_interrupt(int intr_no)
{
    volatile int flag;

    if (intr_no != TIMER_IRQ && intr_no != UART_IRQ && intr_no != UART2_IRQ && intr_no != KEYB_IRQ) {
        panic("wait_for_interrupt(): only TIMER_IRQ, UART_IRQ, UART2_IRQ, and KEYB_IRQ are supported");
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

    unmask_supported_irqs();
    interrupts_initialized = TRUE;
}
