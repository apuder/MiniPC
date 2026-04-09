
#include <kernel.h>

#include "disptable.c"


PROCESS         active_proc;

#define CONTEXT_WORD_SIZE       4
#define CONTEXT_WORDS           32
#define CONTEXT_FRAME_SIZE      (CONTEXT_WORD_SIZE * CONTEXT_WORDS)

#define CTX_OFS_RA              0
#define CTX_OFS_GP              4
#define CTX_OFS_TP              8
#define CTX_OFS_T0              12
#define CTX_OFS_T1              16
#define CTX_OFS_T2              20
#define CTX_OFS_S0              24
#define CTX_OFS_S1              28
#define CTX_OFS_A0              32
#define CTX_OFS_A1              36
#define CTX_OFS_A2              40
#define CTX_OFS_A3              44
#define CTX_OFS_A4              48
#define CTX_OFS_A5              52
#define CTX_OFS_A6              56
#define CTX_OFS_A7              60
#define CTX_OFS_S2              64
#define CTX_OFS_S3              68
#define CTX_OFS_S4              72
#define CTX_OFS_S5              76
#define CTX_OFS_S6              80
#define CTX_OFS_S7              84
#define CTX_OFS_S8              88
#define CTX_OFS_S9              92
#define CTX_OFS_S10             96
#define CTX_OFS_S11             100
#define CTX_OFS_T3              104
#define CTX_OFS_T4              108
#define CTX_OFS_T5              112
#define CTX_OFS_T6              116
#define CTX_OFS_MSTATUS         120
#define CTX_OFS_MEPC            124


/* 
 * Ready queues for all eight priorities.
 */
PCB            *ready_queue[MAX_READY_QUEUES];

/* TOS_IFDEF assn3 */
/* 
 * The bits in ready_procs tell which ready queue is empty.
 * The MSB of ready_procs corresponds to ready_queue[7].
 */
unsigned        ready_procs;
/* TOS_ENDIF assn3 */



/* 
 * add_ready_queue
 *----------------------------------------------------------------------------
 * The process pointed to by p is put the ready queue.
 * The appropiate ready queue is determined by p->priority.
 */

void add_ready_queue(PROCESS proc)
{
    /* TOS_IFDEF assn3 */
    int             prio;
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(proc != NULL && proc->magic == MAGIC_PCB);
    prio = proc->priority;
    assert(prio >= 0 && prio < MAX_READY_QUEUES);
    if (ready_queue[prio] == NULL) {
        /* The only process on this priority level */
        ready_queue[prio] = proc;
        proc->next = proc;
        proc->prev = proc;
        ready_procs |= 1 << prio;
    } else {
        /* Some other processes on this priority level */
        proc->next = ready_queue[prio];
        proc->prev = ready_queue[prio]->prev;
        ready_queue[prio]->prev->next = proc;
        ready_queue[prio]->prev = proc;
    }
    proc->state = STATE_READY;
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    /* TOS_ENDIF assn3 */
}



/* 
 * remove_ready_queue
 *----------------------------------------------------------------------------
 * The process pointed to by p is dequeued from the ready
 * queue.
 */

void remove_ready_queue(PROCESS proc)
{
    /* TOS_IFDEF assn3 */
    int             prio;
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    assert(proc->magic == MAGIC_PCB);
    prio = proc->priority;
    if (proc->next == proc) {
        /* No further processes on this priority level */
        ready_queue[prio] = NULL;
        ready_procs &= ~(1 << prio);
    } else {
        ready_queue[prio] = proc->next;
        proc->next->prev = proc->prev;
        proc->prev->next = proc->next;
    }
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    /* TOS_ENDIF assn3 */
}



/* 
 * become_zombie
 *----------------------------------------------------------------------------
 * Turns the calling process into a zombie. It will be removed from the ready
 * queue and marked as being in STATE_ZOMBIE.
 */

void become_zombie()
{
    active_proc->state = STATE_ZOMBIE;
    /* TOS_IFDEF assn4 */
    remove_ready_queue(active_proc);
    resign();
    // Never reached
    /* TOS_ENDIF assn4 */
    while (1);
}



/* 
 * dispatcher
 *----------------------------------------------------------------------------
 * Determines a new process to be dispatched. The process
 * with the highest priority is taken. Within one priority
 * level round robin is used.
 */

PROCESS dispatcher()
{
    /* TOS_IFDEF assn3 */
    PROCESS         new_proc;
    unsigned        i;
    /* TOS_IFDEF assn7 */
    volatile int    flag;

    DISABLE_INTR(flag);
    /* TOS_ENDIF assn7 */

    /* Find queue with highest priority that is not empty */
    i = table[ready_procs];
    assert(i != -1);
    if (i == active_proc->priority)
        /* Round robin within the same priority level */
        new_proc = active_proc->next;
    else
        /* Dispatch a process at a different priority level */
        new_proc = ready_queue[i];
    /* TOS_IFDEF assn7 */
    ENABLE_INTR(flag);
    /* TOS_ENDIF assn7 */
    return new_proc;
    /* TOS_ENDIF assn3 */
}



/* 
 * resign
 *----------------------------------------------------------------------------
 * The current process gives up the CPU voluntarily. The
 * next running process is determined via dispatcher().
 * The stack of the calling process is setup such that it
 * looks like an interrupt.
 */
/* Stringify a constant so it can be pasted directly into an asm string. */
#define _STR(x) #x
#define STR(x)  _STR(x)

void __attribute__ ((naked)) resign()
{
    /* TOS_IFDEF assn4 */
    /*
     * All offsets are compile-time constants, so we stringify them directly
     * into the asm template.
     */
    asm volatile (
        /* Save complete RV32 context. */
        "addi sp, sp, -" STR(CONTEXT_FRAME_SIZE)    "\n"
        "sw ra,   " STR(CTX_OFS_RA)      "(sp)      \n"
        "sw gp,   " STR(CTX_OFS_GP)      "(sp)      \n"
        "sw tp,   " STR(CTX_OFS_TP)      "(sp)      \n"
        "sw t0,   " STR(CTX_OFS_T0)      "(sp)      \n"
        "sw t1,   " STR(CTX_OFS_T1)      "(sp)      \n"
        "sw t2,   " STR(CTX_OFS_T2)      "(sp)      \n"
        "sw s0,   " STR(CTX_OFS_S0)      "(sp)      \n"
        "sw s1,   " STR(CTX_OFS_S1)      "(sp)      \n"
        "sw a0,   " STR(CTX_OFS_A0)      "(sp)      \n"
        "sw a1,   " STR(CTX_OFS_A1)      "(sp)      \n"
        "sw a2,   " STR(CTX_OFS_A2)      "(sp)      \n"
        "sw a3,   " STR(CTX_OFS_A3)      "(sp)      \n"
        "sw a4,   " STR(CTX_OFS_A4)      "(sp)      \n"
        "sw a5,   " STR(CTX_OFS_A5)      "(sp)      \n"
        "sw a6,   " STR(CTX_OFS_A6)      "(sp)      \n"
        "sw a7,   " STR(CTX_OFS_A7)      "(sp)      \n"
        "sw s2,   " STR(CTX_OFS_S2)      "(sp)      \n"
        "sw s3,   " STR(CTX_OFS_S3)      "(sp)      \n"
        "sw s4,   " STR(CTX_OFS_S4)      "(sp)      \n"
        "sw s5,   " STR(CTX_OFS_S5)      "(sp)      \n"
        "sw s6,   " STR(CTX_OFS_S6)      "(sp)      \n"
        "sw s7,   " STR(CTX_OFS_S7)      "(sp)      \n"
        "sw s8,   " STR(CTX_OFS_S8)      "(sp)      \n"
        "sw s9,   " STR(CTX_OFS_S9)      "(sp)      \n"
        "sw s10,  " STR(CTX_OFS_S10)     "(sp)      \n"
        "sw s11,  " STR(CTX_OFS_S11)     "(sp)      \n"
        "sw t3,   " STR(CTX_OFS_T3)      "(sp)      \n"
        "sw t4,   " STR(CTX_OFS_T4)      "(sp)      \n"
        "sw t5,   " STR(CTX_OFS_T5)      "(sp)      \n"
        "sw t6,   " STR(CTX_OFS_T6)      "(sp)      \n"
        /* Disable IRQs and save previous IRQ mask as part of context. */
        "li t0, -1                                 \n"
        "mv a0, t0                                 \n"
        " .word 0x0605650b                         \n"
        "sw a0, " STR(CTX_OFS_MSTATUS) "(sp)      \n"
        "sw ra,   " STR(CTX_OFS_MEPC)    "(sp)      \n"

        /* active_proc->esp = sp */
        "la t0, active_proc                         \n"
        "lw t1, 0(t0)                               \n"
        "sw sp, 12(t1)                              \n"

        /* active_proc = dispatcher(); */
        "call dispatcher                            \n"
        "la t0, active_proc                         \n"
        "sw a0, 0(t0)                               \n"

        /* sp = active_proc->esp */
        "lw t1, 0(t0)                               \n"
        "lw sp, 12(t1)                              \n"

        /*
         * Restore context of selected process.
         *
         * The shared context convention is that CTX_OFS_RA always contains
         * the address where execution should resume. For voluntary resign()
         * switches, that is the caller's return address. For IRQ-saved
         * contexts, irq.s writes the interrupted PC into that same slot.
         * This lets us finish with a plain 'ret' without clobbering any
         * restored general-purpose register just to hold a jump target.
         */
        "lw ra,   " STR(CTX_OFS_RA)      "(sp)      \n"
        "lw gp,   " STR(CTX_OFS_GP)      "(sp)      \n"
        "lw tp,   " STR(CTX_OFS_TP)      "(sp)      \n"
        "lw t0,   " STR(CTX_OFS_T0)      "(sp)      \n"
        "lw t1,   " STR(CTX_OFS_T1)      "(sp)      \n"
        "lw t2,   " STR(CTX_OFS_T2)      "(sp)      \n"
        "lw s0,   " STR(CTX_OFS_S0)      "(sp)      \n"
        "lw s1,   " STR(CTX_OFS_S1)      "(sp)      \n"
        "lw a0,   " STR(CTX_OFS_A0)      "(sp)      \n"
        "lw a1,   " STR(CTX_OFS_A1)      "(sp)      \n"
        "lw a2,   " STR(CTX_OFS_A2)      "(sp)      \n"
        "lw a3,   " STR(CTX_OFS_A3)      "(sp)      \n"
        "lw a4,   " STR(CTX_OFS_A4)      "(sp)      \n"
        "lw a5,   " STR(CTX_OFS_A5)      "(sp)      \n"
        "lw a6,   " STR(CTX_OFS_A6)      "(sp)      \n"
        "lw a7,   " STR(CTX_OFS_A7)      "(sp)      \n"
        "lw s2,   " STR(CTX_OFS_S2)      "(sp)      \n"
        "lw s3,   " STR(CTX_OFS_S3)      "(sp)      \n"
        "lw s4,   " STR(CTX_OFS_S4)      "(sp)      \n"
        "lw s5,   " STR(CTX_OFS_S5)      "(sp)      \n"
        "lw s6,   " STR(CTX_OFS_S6)      "(sp)      \n"
        "lw s7,   " STR(CTX_OFS_S7)      "(sp)      \n"
        "lw s8,   " STR(CTX_OFS_S8)      "(sp)      \n"
        "lw s9,   " STR(CTX_OFS_S9)      "(sp)      \n"
        "lw s10,  " STR(CTX_OFS_S10)     "(sp)      \n"
        "lw s11,  " STR(CTX_OFS_S11)     "(sp)      \n"
        "lw t3,   " STR(CTX_OFS_T3)      "(sp)      \n"
        "lw t4,   " STR(CTX_OFS_T4)      "(sp)      \n"
        "lw t5,   " STR(CTX_OFS_T5)      "(sp)      \n"
        "lw t6,   " STR(CTX_OFS_T6)      "(sp)      \n"

        /* Restore per-process IRQ mask saved in context frame. */
        "lw t0,   " STR(CTX_OFS_MSTATUS) "(sp)      \n"
        "mv a0, t0                                 \n"
        " .word 0x0605650b                         \n"
        "lw a0,   " STR(CTX_OFS_A0)      "(sp)      \n"
        "lw t0,   " STR(CTX_OFS_T0)      "(sp)      \n"

        "addi sp, sp, " STR(CONTEXT_FRAME_SIZE)     "\n"
        "ret                                        \n"
        ::: "memory");
    /* TOS_ENDIF assn4 */
}



/* 
 * init_dispatcher
 *----------------------------------------------------------------------------
 * Initializes the necessary data structures.
 */

void init_dispatcher()
{
    /* TOS_IFDEF assn3 */
    int             i;

    for (i = 0; i < MAX_READY_QUEUES; i++)
        ready_queue[i] = NULL;

    ready_procs = 0;

    /* Setup first process */
    add_ready_queue(active_proc);
    /* TOS_ENDIF assn3 */
}
