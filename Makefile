CC=i686-elf-gcc
LD=i686-elf-gcc

CFLAGS = -c -std=gnu11 -Werror -I ./include

KERNEL = ./sys/kernel.bin
ISO_IMAGE = os.iso

all: $(KERNEL) $(ISO)

clean:
	rm -f $(ISO_IMAGE) $(KERNEL)

kernel: $(KERNEL)

iso: $(KERNEL) $(ISO_IMAGE)

$(KERNEL):
	make -C sys

$(ISO_IMAGE):
	tar --owner=root -C ./initrd -cvf ./iso/boot/initrd.img .
	cp -p $(KERNEL) ./iso/boot
	grub-mkrescue -o $(ISO_IMAGE) iso
