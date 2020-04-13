LDFLAGS = -T linker.ld --freestanding -O2 -nostdlib

KERNEL_I686_OBJECTS += $(KERNEL_OBJECT)

KERNEL_I686_OBJECTS += dev/dev_init.o
KERNEL_I686_OBJECTS += dev/serial_tty.o
KERNEL_I686_OBJECTS += dev/kbd.o
KERNEL_I686_OBJECTS += dev/lfb.o
KERNEL_I686_OBJECTS += dev/mouse.o
KERNEL_I686_OBJECTS += dev/rtc.o
KERNEL_I686_OBJECTS += dev/vga_textscreen.o
KERNEL_I686_OBJECTS += dev/virtio.o
KERNEL_I686_OBJECTS += dev/virtio_blk.o
KERNEL_I686_OBJECTS += kern/context_switch.o
KERNEL_I686_OBJECTS += kern/clone.o
KERNEL_I686_OBJECTS += kern/preinit.o
KERNEL_I686_OBJECTS += kern/boot.o
KERNEL_I686_OBJECTS += kern/gdt.o
KERNEL_I686_OBJECTS += kern/halt.o
KERNEL_I686_OBJECTS += kern/interrupt.o
KERNEL_I686_OBJECTS += kern/interrupt_dispatch.o
KERNEL_I686_OBJECTS += kern/interrupt_handler.o
KERNEL_I686_OBJECTS += kern/exec.o
KERNEL_I686_OBJECTS += kern/fork.o
KERNEL_I686_OBJECTS += kern/pci.o
KERNEL_I686_OBJECTS += kern/sched.o
KERNEL_I686_OBJECTS += kern/signal.o
KERNEL_I686_OBJECTS += kern/stacktrace.o
KERNEL_I686_OBJECTS += kern/syscall_dispatch.o
KERNEL_I686_OBJECTS += kern/traps.o
KERNEL_I686_OBJECTS += kern/usermode.o
KERNEL_I686_OBJECTS += kern/vm.o

KERNEL_I686 = kernel.i686.bin

all: $(KERNEL_I686)

$(KERNEL_I686): $(KERNEL_I686_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
%.o: %.asm
	$(AS) $(AFLAGS) $^ -o $@

clean:
	rm -f $(KERNEL_I686_OBJECTS) $(KERNEL_I686)
