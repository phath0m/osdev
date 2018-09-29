AS=nasm
CC=i686-elf-gcc
LD=i686-elf-gcc

AFLAGS= -f elf32
CFLAGS = -c -std=gnu99 --freestanding -Wall -Werror -nostartfiles -nodefaultlibs -I ../include
LDFLAGS = -T linker.ld --freestanding -O2 -nostdlib

# TODO: Make this conditional (for different arch)
# i686 specific things....
#
CRTBEGIN_OBJECT := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJECT   := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

KERNEL_OBJECTS += i686/crti.o
KERNEL_OBJECTS += i686/crtbegin.o
KERNEL_OBJECTS += i686/preinit.o
KERNEL_OBJECTS += i686/startup.o

# non i686 things here

KERNEL_OBJECTS += dev/vga_textscreen.o

KERNEL_OBJECTS += fs/devfs.o
KERNEL_OBJECTS += fs/initramfs.o

KERNEL_OBJECTS += kern/device.o
KERNEL_OBJECTS += kern/vfs.o
KERNEL_OBJECTS += kern/kmain.o
KERNEL_OBJECTS += kern/kputs.o

# Kernel runtime library
KERNEL_OBJECTS += rtl/dict.o
KERNEL_OBJECTS += rtl/list.o
KERNEL_OBJECTS += rtl/malloc.o
KERNEL_OBJECTS += rtl/mutex.o
KERNEL_OBJECTS += rtl/printf.o
KERNEL_OBJECTS += rtl/string.o

KERNEL_OBJECTS += i686/crtend.o
KERNEL_OBJECTS += i686/crtn.o

KERNEL = kernel.bin

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
i686/crtbegin.o i686/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
%.o: %.asm
	$(AS) $(AFLAGS) $^ -o $@

clean:
	rm -f $(KERNEL_OBJECTS)
