CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu11 -Werror -I ./include

KERNEL = ./sys/kernel.bin
ISO_IMAGE = os.iso

all: $(KERNEL) $(ISO)

clean:
	rm -f $(ISO_IMAGE) $(KERNEL)

userland:
	mkdir -p ./initrd/bin
	mkdir -p ./initrd/sbin
	make -C bin
	make -C sbin
	cp -rp etc ./initrd

kernel: $(KERNEL)

iso: $(KERNEL) $(ISO_IMAGE)

$(KERNEL):
	make -C sys

$(ISO_IMAGE):
	tar --owner=root -C ./initrd -cvf ./iso/boot/initrd.img .
	cp -p $(KERNEL) ./iso/boot
	grub2-mkrescue -o $(ISO_IMAGE) iso
