
#include <kernel.h>


PCB             pcb[MAX_PROCS];
/* TOS_IFDEF assn3 */
PCB            *next_free_pcb;
/* TOS_ENDIF assn3 */


PORT create_process(void (*ptr_to_new_proc) (PROCESS, PARAM),
                    int prio, PARAM param, char *name)
{
    /* TOS_IFDEF assn3 */
    MEM_ADDR        esp;
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
    /* Compute linear address of new process' system stack */
    esp = 640 * 1024 - (new_proc - pcb) * 16 * 1024;

#define PUSH(x)    esp -= 4; \
                   poke_l (esp, (LONG) x);

    /* Initialize the stack for the new process */
    PUSH(param);                /* First data */
    PUSH(new_proc);             /* Self */
    PUSH(0);                    /* Dummy return address */
    /* TOS_IFDEF assn7 */
    if (interrupts_initialized) {
        PUSH(512);              /* Flags with enabled Interrupts */
    } else {
        PUSH(0);                /* Flags with disabled Interrupts */
    }
    PUSH(CODE_SELECTOR);        /* Kernel code selector */
    /* TOS_ENDIF assn7 */
    PUSH(ptr_to_new_proc);      /* Entry point of new process */
    PUSH(0);                    /* EAX */
    PUSH(0);                    /* ECX */
    PUSH(0);                    /* EDX */
    PUSH(0);                    /* EBX */
    PUSH(0);                    /* EBP */
    PUSH(0);                    /* ESI */
    PUSH(0);                    /* EDI */

#undef PUSH

    /* Save context ptr (actually current stack pointer) */
    new_proc->esp = esp;

    add_ready_queue(new_proc);

    /* TOS_IFDEF assn5 */
    return new_port;
    /* TOS_ELSE */
    return NULL;
    /* TOS_ENDIF assn5 */
    /* TOS_ENDIF assn3 */
}


PROCESS fork()
{
    /* TOS_IFDEF never */
    // Pop off the return address
    asm("popl %eax");
    // Push it back onto the stack. %EAX now contains the return address
    asm("pushl %eax");
    // Push end_active_proc_esp
    asm("pushl %esp");
    // Push ret_addr
    asm("pushl %eax");
    // Call fork_impl()
    asm("call fork_impl");
    // Cleanup stack
    asm("addl $8, %esp");
    // Simply do a ret. This will pass-through the return value
    // we got from fork_impl()
    asm("ret");
    /* TOS_ENDIF never */
    // Dummy return to make gcc happy
    return (PROCESS) NULL;
}

/* TOS_IFDEF never */
PROCESS fork_impl(MEM_ADDR ret_addr, MEM_ADDR end_active_proc_esp)
{
    MEM_ADDR        begin_new_proc_esp;
    MEM_ADDR        begin_active_proc_esp;
    PROCESS         new_proc;
    volatile int    flag;

    DISABLE_INTR(flag);
    if (next_free_pcb == NULL)
        panic("create(): PCB full");
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

    /* Compute linear address of new process' system stack */
    begin_new_proc_esp = 640 * 1024 - (new_proc - pcb) * 30 * 1024;

    begin_active_proc_esp = 640 * 1024 - (active_proc - pcb) * 30 * 1024;

    /* Adjust the %ESP we got as a parameter */
    end_active_proc_esp += 4;

    /* Copy the parents stack frame to the childs stack frame */
    do {
        begin_new_proc_esp -= 4;
        begin_active_proc_esp -= 4;
        poke_l(begin_new_proc_esp, peek_l(begin_active_proc_esp));
    } while (begin_active_proc_esp != end_active_proc_esp);

    /* Push the context onto the stack for the child. This is similar to
     * what is done in create_process(). Note that we push 1 for the value 
     * * of the %EAX register. The %EAX register holds the return value of
     * function fork(). 1 means child process. */
#define PUSH(x)    begin_new_proc_esp -= 4; \
                   poke_l (begin_new_proc_esp, (LONG) x);

    /* Initialize the stack for the new process */
    if (interrupts_initialized) {
        PUSH(512);              /* Flags with enabled Interrupts */
    } else {
        PUSH(0);                /* Flags with disabled Interrupts */
    }
    PUSH(CODE_SELECTOR);        /* Kernel code selector */
    PUSH(ret_addr);             /* Entry point of new process */
    // EAX holds the return value that fork() will return for the child
    PUSH(0);                    /* EAX */
    PUSH(0);                    /* ECX */
    PUSH(0);                    /* EDX */
    PUSH(0);                    /* EBX */
    PUSH(0);                    /* EBP */
    PUSH(0);                    /* ESI */
    PUSH(0);                    /* EDI */

#undef PUSH

    /* Save context ptr (actually current stack pointer) */
    new_proc->esp = begin_new_proc_esp;

    add_ready_queue(new_proc);

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
    for (i = 1; i < MAX_PROCS; i++) {
        pcb[i].magic = 0;
        pcb[i].used = FALSE;
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
    pcb[0].name = "Boot process";
    /* TOS_ENDIF assn3 */
}
