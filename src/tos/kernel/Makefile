
include ../MakeVars


# delete builtin rules
%.o: %.c
%.o: %.s

OBJS = startup.o irq.o main.o uart.o mem.o window.o assert.o \
	dispatch.o process.o ipc.o intr.o malloc.o stdlib.o wm.o \
	null.o timer.o

#OBJS = startup.o stdlib.o window.o process.o assert.o mem.o \
#       dispatch.o intr.o inout.o ipc.o com.o timer.o \
#       null.o keyb.o shell.o wm.o train.o pacman.o pong.o malloc.o

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ -c $<

%.o: %.c .depend
	$(CC) $(CFLAGS) -o $@ -c $<

all: kernel.bin lib

irq.o: irq.s custom_ops.s
	$(CC) $(COMMONFLAGS) -x assembler-with-cpp -o $@ -c $<

lib: kernel.lib

kernel.lib: $(OBJS)
	$(AR) rcs kernel.lib $(OBJS)

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) -Tlinker.ld -o kernel.elf $(OBJS) $(LIBS)

kernel.bin: kernel.elf Makefile
	$(OBJCOPY) kernel.elf -O binary kernel.bin

disptable.c: gentable.cc
	$(CPP_HOST) gentable.cc -o gentable
	./gentable > disptable.c
	rm -f gentable

indent:
	indent -nbad -nbap -bbo -bc -br -brs -c33 -cd33 -ce -ci4 -cli0 \
               -cp33 -di16 -fc1 -fca -hnl -i4 -ip4 -l75 -lp -npcs -nprs \
               -nut -npsl -saf -sai -saw -sc -nsob -nss -ts8 *.c

# All the following file have function with the following pattern:
#
#   f()
#   {
#       ...
#       asm ("iret");
#   }
#
# In this case we by-pass the epilogue code of function f()
# The problem is that gcc aligns %esp to multiple of 16,
# which can cause problems since we do not know inside our
# inline assembly what the current alignment is. We therefore
# force gcc to forget about stack alignment for those files.
# This is accomplished with the -mpreferred-stack-boundary=2
# option

clean :
	rm -f *~ *.o *.bak *.img *.lib *.elf *.bin disptable.c

ifeq (.depend, $(wildcard .depend))
include .depend
endif

.depend:
	echo '# Module dependencies' > .depend
	$(MAKEDEPEND) -- $(CC_OPT) $(INCLUDES) -- $(OBJS:.o=.c) >> .depend
