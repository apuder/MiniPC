
#include "custom_ops.s"

.section .irq_vector,"ax",@progbits
.global irq_entry

/* Context layout shared with resign()/process.c */
.set CTX_OFS_RA,      0
.set CTX_OFS_GP,      4
.set CTX_OFS_TP,      8
.set CTX_OFS_T0,     12
.set CTX_OFS_T1,     16
.set CTX_OFS_T2,     20
.set CTX_OFS_S0,     24
.set CTX_OFS_S1,     28
.set CTX_OFS_A0,     32
.set CTX_OFS_A1,     36
.set CTX_OFS_A2,     40
.set CTX_OFS_A3,     44
.set CTX_OFS_A4,     48
.set CTX_OFS_A5,     52
.set CTX_OFS_A6,     56
.set CTX_OFS_A7,     60
.set CTX_OFS_S2,     64
.set CTX_OFS_S3,     68
.set CTX_OFS_S4,     72
.set CTX_OFS_S5,     76
.set CTX_OFS_S6,     80
.set CTX_OFS_S7,     84
.set CTX_OFS_S8,     88
.set CTX_OFS_S9,     92
.set CTX_OFS_S10,    96
.set CTX_OFS_S11,   100
.set CTX_OFS_T3,    104
.set CTX_OFS_T4,    108
.set CTX_OFS_T5,    112
.set CTX_OFS_T6,    116
.set CTX_OFS_MSTATUS,120
.set CTX_OFS_MEPC,  124
.set CONTEXT_FRAME_SIZE, 128
.set PCB_OFS_ESP, 12

/*
 * Timer IRQ entry:
 * 1) Save full context of interrupted process to its PCB context frame.
 * 2) Call isr_timer(), which updates active_proc via dispatcher().
 * 3) Restore context from active_proc and return with retirq.
 */
irq_entry:
    addi sp, sp, -CONTEXT_FRAME_SIZE

    /*
     * Keep the shared context format compatible with resign(): the RA slot
     * must always contain the address to resume execution at.
     *
     * For an IRQ-saved context, the true resume PC is q0 (interrupted PC),
     * while the interrupted process's architectural ra register must also be
     * preserved. Store q0 in the RA slot and stash the real ra in the MEPC
     * slot so both paths can restore correctly.
     */
    picorv32_getq_insn(t0, q0)
    sw t0,   CTX_OFS_RA(sp)
    sw gp,   CTX_OFS_GP(sp)
    sw tp,   CTX_OFS_TP(sp)

    sw t0,   CTX_OFS_T0(sp)
    sw t1,   CTX_OFS_T1(sp)
    sw t2,   CTX_OFS_T2(sp)
    sw s0,   CTX_OFS_S0(sp)
    sw s1,   CTX_OFS_S1(sp)
    sw a0,   CTX_OFS_A0(sp)
    sw a1,   CTX_OFS_A1(sp)
    sw a2,   CTX_OFS_A2(sp)
    sw a3,   CTX_OFS_A3(sp)
    sw a4,   CTX_OFS_A4(sp)
    sw a5,   CTX_OFS_A5(sp)
    sw a6,   CTX_OFS_A6(sp)
    sw a7,   CTX_OFS_A7(sp)
    sw s2,   CTX_OFS_S2(sp)
    sw s3,   CTX_OFS_S3(sp)
    sw s4,   CTX_OFS_S4(sp)
    sw s5,   CTX_OFS_S5(sp)
    sw s6,   CTX_OFS_S6(sp)
    sw s7,   CTX_OFS_S7(sp)
    sw s8,   CTX_OFS_S8(sp)
    sw s9,   CTX_OFS_S9(sp)
    sw s10,  CTX_OFS_S10(sp)
    sw s11,  CTX_OFS_S11(sp)
    sw t3,   CTX_OFS_T3(sp)
    sw t4,   CTX_OFS_T4(sp)
    sw t5,   CTX_OFS_T5(sp)
    sw t6,   CTX_OFS_T6(sp)
    /* Save the interrupted process's architectural ra register. */
    sw ra,   CTX_OFS_MEPC(sp)

    /*
     * Save the current IRQ mask in the shared context slot used by resign().
     * Do not store q1 here: q1 contains pending IRQ bits, not the mask value.
     */
    li t1, -1
    mv a0, t1
    picorv32_maskirq_insn(a0, a0)
    sw a0,   CTX_OFS_MSTATUS(sp)

    /* active_proc->esp = sp */
    la t0, active_proc
    lw t1, 0(t0)
    sw sp, PCB_OFS_ESP(t1)

    /* Ensure gp points at the small-data region before entering C. */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    call isr_timer

    /* sp = active_proc->esp */
    la t0, active_proc
    lw t1, 0(t0)
    lw sp, PCB_OFS_ESP(t1)

    /* Restore context of selected process. */
    /* Restore the architectural ra register saved in the MEPC slot. */
    lw ra,   CTX_OFS_MEPC(sp)
    lw gp,   CTX_OFS_GP(sp)
    lw tp,   CTX_OFS_TP(sp)
    lw t2,   CTX_OFS_T2(sp)
    lw s0,   CTX_OFS_S0(sp)
    lw s1,   CTX_OFS_S1(sp)
    lw a0,   CTX_OFS_A0(sp)
    lw a1,   CTX_OFS_A1(sp)
    lw a2,   CTX_OFS_A2(sp)
    lw a3,   CTX_OFS_A3(sp)
    lw a4,   CTX_OFS_A4(sp)
    lw a5,   CTX_OFS_A5(sp)
    lw a6,   CTX_OFS_A6(sp)
    lw a7,   CTX_OFS_A7(sp)
    lw s2,   CTX_OFS_S2(sp)
    lw s3,   CTX_OFS_S3(sp)
    lw s4,   CTX_OFS_S4(sp)
    lw s5,   CTX_OFS_S5(sp)
    lw s6,   CTX_OFS_S6(sp)
    lw s7,   CTX_OFS_S7(sp)
    lw s8,   CTX_OFS_S8(sp)
    lw s9,   CTX_OFS_S9(sp)
    lw s10,  CTX_OFS_S10(sp)
    lw s11,  CTX_OFS_S11(sp)
    lw t3,   CTX_OFS_T3(sp)
    lw t4,   CTX_OFS_T4(sp)
    lw t5,   CTX_OFS_T5(sp)
    lw t6,   CTX_OFS_T6(sp)

    /* Restore q0 from the shared resume-PC slot before retirq. */
    lw t0,   CTX_OFS_RA(sp)
    picorv32_setq_insn(q0, t0)

    /* Restore per-process IRQ mask before returning from IRQ. */
    lw a0,   CTX_OFS_MSTATUS(sp)
    picorv32_maskirq_insn(a0, a0)
    lw a0,   CTX_OFS_A0(sp)

    lw t0,   CTX_OFS_T0(sp)
    lw t1,   CTX_OFS_T1(sp)

    addi sp, sp, CONTEXT_FRAME_SIZE

    /*
     * PicoRV32 custom IRQ return (retirq), encoded as a raw word because
     * standard assemblers do not recognize this non-RISC-V mnemonic.
     *
     * Conceptually this exits the core's IRQ-active state and resumes at the
     * IRQ return PC held in x3 (gp here), unlike standard RISC-V mret.
     */
    picorv32_retirq_insn()
