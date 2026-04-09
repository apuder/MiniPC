
#include <kernel.h>


PCB             pcb[MAX_PROCS];
/* TOS_IFDEF assn3 */
PCB            *next_free_pcb;
/* TOS_ENDIF assn3 */

extern unsigned __stack_top;

#define PROCESS_STACK_SIZE      (16 * 1024U)
#define CONTEXT_WORD_SIZE       4U
#define CONTEXT_WORDS           32U
#define CONTEXT_FRAME_SIZE      (CONTEXT_WORDS * CONTEXT_WORD_SIZE)

#define CTX_OFS_RA              0U
#define CTX_OFS_GP              4U
#define CTX_OFS_TP              8U
#define CTX_OFS_T0              12U
#define CTX_OFS_T1              16U
#define CTX_OFS_T2              20U
#define CTX_OFS_S0              24U
#define CTX_OFS_S1              28U
#define CTX_OFS_A0              32U
#define CTX_OFS_A1              36U
#define CTX_OFS_A2              40U
#define CTX_OFS_A3              44U
#define CTX_OFS_A4              48U
#define CTX_OFS_A5              52U
#define CTX_OFS_A6              56U
#define CTX_OFS_A7              60U
#define CTX_OFS_S2              64U
#define CTX_OFS_S3              68U
#define CTX_OFS_S4              72U
#define CTX_OFS_S5              76U
#define CTX_OFS_S6              80U
#define CTX_OFS_S7              84U
#define CTX_OFS_S8              88U
#define CTX_OFS_S9              92U
#define CTX_OFS_S10             96U
#define CTX_OFS_S11             100U
#define CTX_OFS_T3              104U
#define CTX_OFS_T4              108U
#define CTX_OFS_T5              112U
#define CTX_OFS_T6              116U
#define CTX_OFS_MSTATUS         120U
#define CTX_OFS_MEPC            124U

#define IRQ_MASK_ALL_DISABLED   (~0u)
#define IRQ_MASK_TIMER_ENABLED  0xFFFFFF7Fu

/*
 * Bootstrap for a freshly-created process context.
 *
 * resign() restores registers and returns via 'ret' (using restored ra), so
 * a new process needs a callable C entry point in ra. This helper calls the
 * actual process function and turns the process into a zombie if it returns.
 */
static void __attribute__((noreturn)) process_bootstrap(PROCESS self,
                                                        PARAM param,
                                                        void (*entry)(PROCESS, PARAM))
{
    entry(self, param);
    become_zombie();
    while (1) ;
}


PORT create_process(void (*ptr_to_new_proc) (PROCESS, PARAM),
                    int prio, PARAM param, char *name)
{
    /* TOS_IFDEF assn3 */
    MEM_ADDR        esp;
    MEM_ADDR        stack_top;
    MEM_ADDR        gp;
    LONG            initial_irq_mask;
    PROCESS         new_proc;
    PORT            new_port;
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    if (prio >= MAX_READY_QUEUES)
        panic("create(): Bad priority");
    if (next_free_pcb == NULL)
        panic("create(): PCB full");
    new_proc = next_free_pcb;
    next_free_pcb = new_proc->next;
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    new_proc->used = TRUE;
    new_proc->magic = MAGIC_PCB;
    new_proc->state = STATE_READY;
    new_proc->priority = prio;
    new_proc->first_port = NULL;
    new_proc->name = name;

    /* TOS_IFDEF assn5 */
    new_port = create_new_port(new_proc);

    /* TOS_ENDIF assn5 */
    /* Compute top of the process' fixed 16 KiB stack region. */
    stack_top = (MEM_ADDR) (&__stack_top)
        - (MEM_ADDR) (new_proc - pcb) * PROCESS_STACK_SIZE;
    esp = stack_top - CONTEXT_FRAME_SIZE;

    /* Capture current global pointer so new process can access globals. */
    asm volatile ("mv %0, gp":"=r"(gp));

    if (interrupts_initialized) {
        initial_irq_mask = (LONG)IRQ_MASK_TIMER_ENABLED;
    } else {
        initial_irq_mask = (LONG)IRQ_MASK_ALL_DISABLED;
    }

    /*
     * Build a full register context on the stack. The layout matches resign()
     * and already reserves mstatus/mepc slots for a future ISR-based switch.
     */
    poke_l(esp + CTX_OFS_RA, (LONG) process_bootstrap);
    poke_l(esp + CTX_OFS_GP, (LONG) gp);
    poke_l(esp + CTX_OFS_TP, 0);
    poke_l(esp + CTX_OFS_T0, 0);
    poke_l(esp + CTX_OFS_T1, 0);
    poke_l(esp + CTX_OFS_T2, 0);
    poke_l(esp + CTX_OFS_S0, 0);
    poke_l(esp + CTX_OFS_S1, 0);
    poke_l(esp + CTX_OFS_A0, (LONG) new_proc);
    poke_l(esp + CTX_OFS_A1, (LONG) param);
    poke_l(esp + CTX_OFS_A2, (LONG) ptr_to_new_proc);
    poke_l(esp + CTX_OFS_A3, 0);
    poke_l(esp + CTX_OFS_A4, 0);
    poke_l(esp + CTX_OFS_A5, 0);
    poke_l(esp + CTX_OFS_A6, 0);
    poke_l(esp + CTX_OFS_A7, 0);
    poke_l(esp + CTX_OFS_S2, 0);
    poke_l(esp + CTX_OFS_S3, 0);
    poke_l(esp + CTX_OFS_S4, 0);
    poke_l(esp + CTX_OFS_S5, 0);
    poke_l(esp + CTX_OFS_S6, 0);
    poke_l(esp + CTX_OFS_S7, 0);
    poke_l(esp + CTX_OFS_S8, 0);
    poke_l(esp + CTX_OFS_S9, 0);
    poke_l(esp + CTX_OFS_S10, 0);
    poke_l(esp + CTX_OFS_S11, 0);
    poke_l(esp + CTX_OFS_T3, 0);
    poke_l(esp + CTX_OFS_T4, 0);
    poke_l(esp + CTX_OFS_T5, 0);
    poke_l(esp + CTX_OFS_T6, 0);
    poke_l(esp + CTX_OFS_MSTATUS, initial_irq_mask);
    poke_l(esp + CTX_OFS_MEPC, (LONG) process_bootstrap);

    /* Save context ptr (actually current stack pointer) */
    new_proc->esp = esp;

    add_ready_queue(new_proc);

    return new_port;
}


/*
 * Buffer used by the naked fork() stub to pass the parent's callee-saved
 * register state to fork_impl().
 * Layout: [0]=ra  [1]=sp  [2..13]=s0..s11
 * Must have global linkage so the 'la' instruction in the naked asm body
 * can reference it as an external symbol.
 * Single-CPU system: no concurrent fork() calls possible.
 */
MEM_ADDR fork_regs[14];

/*
 * fork
 * ----------------------------------------------------------------------------
 * Naked wrapper: the compiler generates no prologue/epilogue, so ra and sp
 * here reflect the values in fork()'s CALLER at the moment of the call.
 *
 *   ra  = return address back into fork()'s caller
 *   sp  = caller's stack pointer (top of the caller's live stack frame)
 *
 * All callee-saved registers (s0-s11) are snapshotted into fork_regs so
 * fork_impl can propagate them into the child's context frame, preserving
 * the full register state across the fork boundary.
 *
 * Returns: child PROCESS pointer to parent, NULL (0) to child.
 */
PROCESS __attribute__((naked)) fork()
{
    asm volatile (
        "la   t0, fork_regs  \n"
        "sw   ra,  0(t0)     \n"   /* fork_regs[0]  = ra  */
        "sw   sp,  4(t0)     \n"   /* fork_regs[1]  = sp  */
        "sw   s0,  8(t0)     \n"   /* fork_regs[2]  = s0  */
        "sw   s1,  12(t0)    \n"   /* fork_regs[3]  = s1  */
        "sw   s2,  16(t0)    \n"   /* fork_regs[4]  = s2  */
        "sw   s3,  20(t0)    \n"   /* fork_regs[5]  = s3  */
        "sw   s4,  24(t0)    \n"   /* fork_regs[6]  = s4  */
        "sw   s5,  28(t0)    \n"   /* fork_regs[7]  = s5  */
        "sw   s6,  32(t0)    \n"   /* fork_regs[8]  = s6  */
        "sw   s7,  36(t0)    \n"   /* fork_regs[9]  = s7  */
        "sw   s8,  40(t0)    \n"   /* fork_regs[10] = s8  */
        "sw   s9,  44(t0)    \n"   /* fork_regs[11] = s9  */
        "sw   s10, 48(t0)    \n"   /* fork_regs[12] = s10 */
        "sw   s11, 52(t0)    \n"   /* fork_regs[13] = s11 */
        "mv   a0,  ra        \n"   /* arg0 to fork_impl: return address */
        "mv   a1,  sp        \n"   /* arg1 to fork_impl: caller's sp    */
        "tail fork_impl      \n"   /* tail-call; ra unchanged → fork_impl
                                      returns directly to fork()'s caller */
    );
}

/* TOS_IFDEF never */
/*
 * fork_impl
 * ----------------------------------------------------------------------------
 * Called via tail-call from naked fork() with:
 *   ra_val    = the return address that fork()'s caller will see
 *   caller_sp = fork()'s caller's stack pointer = top of the live parent stack
 *
 * Steps:
 *  1. Allocate a child PCB.
 *  2. Copy [caller_sp, parent_stack_top) word-by-word into the equivalent
 *     region of the child's 16 KiB stack.
 *  3. Build a full context frame for the child, placed directly below
 *     child_sp (= child's equivalent of caller_sp).
 *     - ra  = ra_val           → child returns to fork()'s call site
 *     - a0  = 0                → child's fork() return value is NULL
 *     - s0-s11 = parent values → callee-saved registers preserved across fork
 *     - mstatus/mepc reserved  → seamless ISR-based pre-emption later
 *  4. Store the context frame base in the child PCB, add to ready queue.
 *  5. Return the child PCB pointer to the parent (a0 != 0).
 */
PROCESS fork_impl(MEM_ADDR ra_val, MEM_ADDR caller_sp)
{
    MEM_ADDR        parent_stack_top;
    MEM_ADDR        child_stack_top;
    MEM_ADDR        child_sp;
    MEM_ADDR        child_context;
    MEM_ADDR        src, dst, bytes;
    PROCESS         new_proc;
    volatile int    flag;
    MEM_ADDR        gp;
    LONG            inherited_irq_mask;

    DISABLE_INTR(flag);
    if (next_free_pcb == NULL)
        panic("fork(): PCB full");
    new_proc = next_free_pcb;
    next_free_pcb = new_proc->next;
    ENABLE_INTR(flag);

    new_proc->used = TRUE;
    new_proc->magic = MAGIC_PCB;
    new_proc->state = STATE_READY;
    new_proc->priority = active_proc->priority;
    new_proc->first_port = NULL;
    new_proc->name = "Forked process";

    create_new_port(new_proc);

    /* Compute top of each process' 16 KiB stack region. */
    parent_stack_top = (MEM_ADDR)(&__stack_top)
        - (MEM_ADDR)(active_proc - pcb) * PROCESS_STACK_SIZE;
    child_stack_top  = (MEM_ADDR)(&__stack_top)
        - (MEM_ADDR)(new_proc - pcb) * PROCESS_STACK_SIZE;

    /*
     * Copy the live portion of the parent's stack word-by-word.
     * Covers [caller_sp, parent_stack_top) → same relative offset in child.
     */
    bytes = parent_stack_top - caller_sp;
    src   = parent_stack_top;
    dst   = child_stack_top;
    while (bytes > 0) {
        src   -= 4;
        dst   -= 4;
        poke_l(dst, peek_l(src));
        bytes -= 4;
    }

    /*
     * Place the child's context frame immediately below child_sp.
     * child_sp is the child's equivalent of caller_sp (same relative offset
     * from the top of its stack region).
     *
     * resign() restores context then does:
     *   addi sp, sp, CONTEXT_FRAME_SIZE   → sp = child_sp  (= caller's sp)
     *   ret                               → jumps to ra_val
     */
    child_sp      = child_stack_top - (parent_stack_top - caller_sp);
    child_context = child_sp - CONTEXT_FRAME_SIZE;

    asm volatile ("mv %0, gp" : "=r"(gp));

    /* Inherit current IRQ mask state into the child context. */
    DISABLE_INTR(flag);
    inherited_irq_mask = (LONG)flag;
    ENABLE_INTR(flag);

    poke_l(child_context + CTX_OFS_RA,      ra_val);       /* return to fork()'s call site */
    poke_l(child_context + CTX_OFS_GP,      gp);
    poke_l(child_context + CTX_OFS_TP,      0);
    poke_l(child_context + CTX_OFS_T0,      0);
    poke_l(child_context + CTX_OFS_T1,      0);
    poke_l(child_context + CTX_OFS_T2,      0);
    poke_l(child_context + CTX_OFS_S0,      fork_regs[2]); /* preserve s0  */
    poke_l(child_context + CTX_OFS_S1,      fork_regs[3]); /* preserve s1  */
    poke_l(child_context + CTX_OFS_A0,      0);            /* child: fork() == NULL */
    poke_l(child_context + CTX_OFS_A1,      0);
    poke_l(child_context + CTX_OFS_A2,      0);
    poke_l(child_context + CTX_OFS_A3,      0);
    poke_l(child_context + CTX_OFS_A4,      0);
    poke_l(child_context + CTX_OFS_A5,      0);
    poke_l(child_context + CTX_OFS_A6,      0);
    poke_l(child_context + CTX_OFS_A7,      0);
    poke_l(child_context + CTX_OFS_S2,      fork_regs[4]); /* preserve s2  */
    poke_l(child_context + CTX_OFS_S3,      fork_regs[5]); /* preserve s3  */
    poke_l(child_context + CTX_OFS_S4,      fork_regs[6]); /* preserve s4  */
    poke_l(child_context + CTX_OFS_S5,      fork_regs[7]); /* preserve s5  */
    poke_l(child_context + CTX_OFS_S6,      fork_regs[8]); /* preserve s6  */
    poke_l(child_context + CTX_OFS_S7,      fork_regs[9]); /* preserve s7  */
    poke_l(child_context + CTX_OFS_S8,      fork_regs[10]);/* preserve s8  */
    poke_l(child_context + CTX_OFS_S9,      fork_regs[11]);/* preserve s9  */
    poke_l(child_context + CTX_OFS_S10,     fork_regs[12]);/* preserve s10 */
    poke_l(child_context + CTX_OFS_S11,     fork_regs[13]);/* preserve s11 */
    poke_l(child_context + CTX_OFS_T3,      0);
    poke_l(child_context + CTX_OFS_T4,      0);
    poke_l(child_context + CTX_OFS_T5,      0);
    poke_l(child_context + CTX_OFS_T6,      0);
    poke_l(child_context + CTX_OFS_MSTATUS, inherited_irq_mask);
    poke_l(child_context + CTX_OFS_MEPC,    ra_val);       /* for future ISR use */

    new_proc->esp = child_context;

    add_ready_queue(new_proc);

    /* Parent: return the child PCB pointer (non-zero). */
    return new_proc;
}
/* TOS_ENDIF never */


/* TOS_IFDEF assn3 */
void print_process_heading(WINDOW * wnd)
{
    wprintf(wnd, "State           Active Prio Name\n");
    wprintf(wnd, "------------------------------------------------\n");
}

void print_process_details(WINDOW * wnd, PROCESS p)
{
    static const char *state[] = { "READY          ",
        "ZOMBIE         ",
        "SEND_BLOCKED   ",
        "REPLY_BLOCKED  ",
        "RECEIVE_BLOCKED",
        "MESSAGE_BLOCKED",
        "INTR_BLOCKED   "
    };
    if (!p->used) {
        wprintf(wnd, "PCB slot unused!\n");
        return;
    }
    /* State */
    wprintf(wnd, state[p->state]);
    /* Check for active_proc */
    if (p == active_proc)
        wprintf(wnd, " *      ");
    else
        wprintf(wnd, "        ");
    /* Priority */
    wprintf(wnd, "  %2d", p->priority);
    /* Name */
    wprintf(wnd, " %s\n", p->name);
}
/* TOS_ENDIF assn3 */

void print_process(WINDOW * wnd, PROCESS p)
{
    /* TOS_IFDEF assn3 */
    print_process_heading(wnd);
    print_process_details(wnd, p);
    /* TOS_ENDIF assn3 */
}

void print_all_processes(WINDOW * wnd)
{
    /* TOS_IFDEF assn3 */
    int             i;
    PCB            *p = pcb;

    print_process_heading(wnd);
    for (i = 0; i < MAX_PROCS; i++, p++) {
        if (!p->used)
            continue;
        print_process_details(wnd, p);
    }
    /* TOS_ENDIF assn3 */
}



void init_process()
{
    /* TOS_IFDEF assn3 */
    int             i;

    /* Clear all PCB's */
    for (i = 0; i < MAX_PROCS; i++) {
        pcb[i].magic = 0;
        pcb[i].used = FALSE;
        pcb[i].priority = 0;
        pcb[i].state = 0;
        pcb[i].esp = 0;
        pcb[i].param_proc = NULL;
        pcb[i].param_data = NULL;
        pcb[i].first_port = NULL;
        pcb[i].next_blocked = NULL;
        pcb[i].next = NULL;
        pcb[i].prev = NULL;
        pcb[i].name = NULL;
    }

    /* Create free list; don't bother about the first entry, it'll be used 
     * for the boot process. */
    for (i = 1; i < MAX_PROCS - 1; i++)
        pcb[i].next = &pcb[i + 1];
    pcb[MAX_PROCS - 1].next = NULL;
    next_free_pcb = &pcb[1];

    /* Define pcb[0] for this process */
    active_proc = pcb;
    pcb[0].state = STATE_READY;
    pcb[0].magic = MAGIC_PCB;
    pcb[0].used = TRUE;
    pcb[0].priority = 1;
    pcb[0].first_port = NULL;
    pcb[0].next = NULL;
    pcb[0].prev = NULL;
    pcb[0].name = "Boot process";
    /* TOS_ENDIF assn3 */
}
