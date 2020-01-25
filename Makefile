CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu11 -Werror -I ./include

KERNEL = ./sys/kernel.bin
ISO_IMAGE = build/os.iso

all: $(KERNEL) $(ISO)

clean:
	rm -f $(ISO_IMAGE) $(KERNEL)

userland:
	mkdir -p ./build/initrd/bin
	mkdir -p ./build/initrd/sbin
	mkdir -p ./build/initrd/dev
	mkdir -p ./build/initrd/usr/bin
	mkdir -p ./build/initrd/usr/games
	mkdir -p ./build/initrd/usr/libexec
	mkdir -p ./build/initrd/tmp
	make -C bin
	make -C sbin
	make -C usr.bin
	make -C usr.games
	make -C usr.libexec
	cp -rp etc ./build/initrd

kernel: $(KERNEL)

iso: $(KERNEL) $(ISO_IMAGE)

$(KERNEL):
	make -C sys

$(ISO_IMAGE):
	tar --owner=root -C ./build/initrd -cvf ./build/iso/boot/initrd.img .
	cp -p $(KERNEL) ./build/iso/boot
	grub2-mkrescue -o $(ISO_IMAGE) build/iso
