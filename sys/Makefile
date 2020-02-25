ARCH=i686
MAKE=gmake

include $(shell realpath "$(ARCH)/config.mk")

CFLAGS += -I "$(shell realpath ./)"

KERNEL_OBJECTS += dev/pseudo_device.o

KERNEL_OBJECTS += fs/devfs.o
KERNEL_OBJECTS += fs/tarfs.o
KERNEL_OBJECTS += fs/tmpfs.o

KERNEL_OBJECTS += kern/device.o
KERNEL_OBJECTS += kern/device_file.o
KERNEL_OBJECTS += kern/interrupt.o
KERNEL_OBJECTS += kern/fifo.o
KERNEL_OBJECTS += kern/kmain.o
KERNEL_OBJECTS += kern/malloc.o
KERNEL_OBJECTS += kern/mem_syscalls.o
KERNEL_OBJECTS += kern/misc_syscalls.o
KERNEL_OBJECTS += kern/mount.o
KERNEL_OBJECTS += kern/net.o
KERNEL_OBJECTS += kern/net_syscalls.o
KERNEL_OBJECTS += kern/panic.o
KERNEL_OBJECTS += kern/pipe.o
KERNEL_OBJECTS += kern/printf.o
KERNEL_OBJECTS += kern/proc.o
KERNEL_OBJECTS += kern/proc_syscalls.o
KERNEL_OBJECTS += kern/pty.o
KERNEL_OBJECTS += kern/pty_syscalls.o
KERNEL_OBJECTS += kern/shm.o
KERNEL_OBJECTS += kern/signal.o
KERNEL_OBJECTS += kern/timer.o
KERNEL_OBJECTS += kern/extract_tar.o
KERNEL_OBJECTS += kern/vfs_syscalls.o
KERNEL_OBJECTS += kern/vm.o
KERNEL_OBJECTS += kern/vnode.o
KERNEL_OBJECTS += kern/vops.o
KERNEL_OBJECTS += kern/wait_queue.o

KERNEL_OBJECTS += net/klink.o
KERNEL_OBJECTS += net/un.o

# Kernel runtime library
KERNEL_OBJECTS += libk/dict.o
KERNEL_OBJECTS += libk/fifo.o
KERNEL_OBJECTS += libk/list.o
KERNEL_OBJECTS += libk/mutex.o
KERNEL_OBJECTS += libk/string.o
KERNEL_OBJECTS += libk/membuf.o

KERNEL_OBJECT = $(shell realpath kernel.o)

KERNEL_ARCH = $(ARCH)/kernel.$(ARCH).bin
KERNEL = kernel.bin

export KERNEL_OBJECT

all: $(KERNEL)
$(KERNEL): $(KERNEL_OBJECT)
	$(MAKE) -C $(ARCH)
	mv $(KERNEL_ARCH) $(KERNEL)
$(KERNEL_OBJECT): $(KERNEL_OBJECTS)
	$(LD) -r -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(KERNEL) $(KERNEL_OBJECT) $(KERNEL_OBJECTS)
	$(MAKE) -C $(ARCH) clean
