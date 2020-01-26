CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu11 -Werror -I ./include

KERNEL = ./sys/kernel.bin
INITRD = build/iso/boot/initrd.img
BUILDROOT = $(shell realpath build/initrd)
ISO_IMAGE = build/os.iso

all: $(KERNEL) $(ISO)

clean:
	rm -f $(ISO_IMAGE) $(KERNEL) $(INITRD)

userland:
	mkdir -p ./build/initrd/dev
	mkdir -p ./build/initrd/tmp
	make -C bin
	make -C sbin
	make -C usr.bin
	make -C usr.games
	make -C usr.libexec
	cp -rp etc ./build/initrd

kernel: $(KERNEL)

iso: $(KERNEL) $(INITRD) $(ISO_IMAGE)

$(KERNEL):
	make -C sys

$(INITRD):
	make -C bin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C sbin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C usr.bin PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.games PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.libexec PREFIX=/usr DESTDIR=$(BUILDROOT) install
	tar --owner=root -C $(BUILDROOT) -cvf $(INITRD) .

$(ISO_IMAGE):
	cp -p $(KERNEL) ./build/iso/boot
	grub2-mkrescue -o $(ISO_IMAGE) build/iso
