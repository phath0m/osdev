AS=nasm
CC=i686-pc-elysium-gcc
LD=i686-pc-elysium-gcc

AFLAGS= -f elf32
CFLAGS = -c -std=gnu99 --freestanding -Wall -Werror -nostartfiles -nodefaultlibs -I ../include
LDFLAGS = -T i686/linker.ld --freestanding -O2 -nostdlib

# TODO: Make this conditional (for different arch)
# i686 specific things....
#
CRTBEGIN_OBJECT := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJECT   := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

KERNEL_OBJECTS += i686/rtl/crti.o
KERNEL_OBJECTS += i686/rtl/crtbegin.o

KERNEL_OBJECTS += i686/rtl/malloc.o
KERNEL_OBJECTS += i686/dev/serial_tty.o
KERNEL_OBJECTS += i686/kern/context_switch.o
KERNEL_OBJECTS += i686/kern/preinit.o
KERNEL_OBJECTS += i686/kern/boot.o
KERNEL_OBJECTS += i686/kern/exceptions.o
KERNEL_OBJECTS += i686/kern/gdt.o
KERNEL_OBJECTS += i686/kern/halt.o
KERNEL_OBJECTS += i686/kern/interrupt.o
KERNEL_OBJECTS += i686/kern/interrupt_dispatch.o
KERNEL_OBJECTS += i686/kern/interrupt_handler.o
KERNEL_OBJECTS += i686/kern/exec.o
KERNEL_OBJECTS += i686/kern/fork.o
KERNEL_OBJECTS += i686/kern/sched.o
KERNEL_OBJECTS += i686/kern/syscall_dispatch.o
KERNEL_OBJECTS += i686/kern/usermode.o
KERNEL_OBJECTS += i686/kern/vm.o

# non i686 things here
KERNEL_OBJECTS += dev/pseudo_device.o
KERNEL_OBJECTS += dev/vga_textscreen.o

KERNEL_OBJECTS += fs/devfs.o
KERNEL_OBJECTS += fs/tarfs.o

KERNEL_OBJECTS += kern/device.o
KERNEL_OBJECTS += kern/interrupt.o
KERNEL_OBJECTS += kern/vfs.o
KERNEL_OBJECTS += kern/kmain.o
KERNEL_OBJECTS += kern/kputs.o
KERNEL_OBJECTS += kern/net.o
KERNEL_OBJECTS += kern/panic.o
KERNEL_OBJECTS += kern/pipe.o
KERNEL_OBJECTS += kern/proc.o
KERNEL_OBJECTS += kern/proc_syscalls.o
KERNEL_OBJECTS += kern/vfs_syscalls.o
KERNEL_OBJECTS += kern/wait_queue.o

KERNEL_OBJECTS += net/klink.o

# Kernel runtime library
KERNEL_OBJECTS += libk/dict.o
KERNEL_OBJECTS += libk/list.o
KERNEL_OBJECTS += libk/malloc.o
KERNEL_OBJECTS += libk/mutex.o
KERNEL_OBJECTS += libk/printf.o
KERNEL_OBJECTS += libk/string.o

# end CRT
KERNEL_OBJECTS += i686/rtl/crtend.o
KERNEL_OBJECTS += i686/rtl/crtn.o

KERNEL = kernel.bin

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
i686/rtl/crtbegin.o i686/rtl/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
%.o: %.asm
	$(AS) $(AFLAGS) $^ -o $@

clean:
	rm -f $(KERNEL_OBJECTS)
