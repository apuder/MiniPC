
include ../MakeVars


# delete builtin rules
%.o: %.c
%.o: %.s


%.o: %.s
	$(AS) $(ASFLAGS) -o $@ -c $<

%.o: %.c .depend
	$(CC) $(CFLAGS) -o $@ -c $<

OBJS=run_tests.o common.o lib.o \
    test_mem_1.o \
    test_window_1.o \
    test_window_2.o \
    test_window_3.o \
    test_window_4.o \
    test_window_5.o \
    test_window_6.o \
    test_window_7.o \
    test_create_process_1.o \
    test_create_process_2.o \
    test_create_process_3.o \
    test_dispatcher_1.o \
    test_dispatcher_2.o \
    test_dispatcher_3.o \
    test_resign_1.o \
    test_resign_2.o \
    test_resign_3.o \
    test_resign_4.o \
    test_resign_5.o \
    test_resign_6.o \
    test_ipc_1.o test_ipc_2.o test_ipc_3.o test_ipc_4.o \
    test_ipc_5.o test_ipc_6.o test_isr_2.o test_isr_3.o \
    test_com_1.o
#    test_isr_1.o test_isr_2.o test_isr_3.o \
#    test_timer_1.o \
#    test_com_1.o \
#    test_fork_1.o

tests: kernel.bin

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) -T../kernel/linker.ld -o kernel.elf ../kernel/startup.o ../kernel/irq.o $(OBJS) ../kernel/kernel.lib $(LIBS)

kernel.bin: kernel.elf Makefile
	$(OBJCOPY) kernel.elf -O binary kernel.bin

clean :
	rm -f *~ *.o *.bak *.img *.bin *.elf

ifeq (.depend, $(wildcard .depend))
include .depend
endif

.depend:
	echo '# Module dependencies' > .depend
	$(MAKEDEPEND) -- $(CC_OPT) $(INCLUDES) -- $(OBJS:.o=.c) >> .depend
