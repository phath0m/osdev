KERNEL = ./sys/kernel.bin
INITRD = build/iso/boot/initrd.img
BUILDROOT = $(shell realpath build/initrd)
TOOLROOT = $(shell realpath build/toolroot)
ISO_IMAGE = build/os.iso
PATH := "$(TOOLROOT)/bin:$(PATH)"

.PHONY: userland clean ports

CC=$(TOOLROOT)/bin/i686-elysium-gcc

all: toolchain kernel userland-libraries userland iso
clean:
	rm -f $(ISO_IMAGE) $(KERNEL) $(INITRD)

iso: $(KERNEL) $(INITRD) $(ISO_IMAGE)

$(ISO_IMAGE):
	cp -p $(KERNEL) ./build/iso/boot
	grub2-mkrescue -o $(ISO_IMAGE) build/iso

$(INITRD):
	tar --owner=root -C $(BUILDROOT) -cvf $(INITRD) .

kernel: $(KERNEL)

$(KERNEL):
	PATH=$(PATH) make -C sys
	mkdir -p $(TOOLROOT)/usr/include/elysium/sys
	cp -r sys/sys/*.h $(TOOLROOT)/usr/include/elysium/sys

ports:
	PATH="$(PATH)" make -C ports

userland: 
	mkdir -p $(BUILDROOT)/dev
	mkdir -p $(BUILDROOT)/tmp
	mkdir -p $(BUILDROOT)/var/run
	mkdir -p $(BUILDROOT)/var/adm
	PATH=$(PATH) make -C bin
	PATH=$(PATH) make -C sbin
	PATH=$(PATH) make -C usr.bin
	PATH=$(PATH) make -C usr.games
	PATH=$(PATH) make -C usr.libexec
	make -C bin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C sbin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C usr.bin PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.games PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.libexec PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C etc PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C usr.share PREFIX=/usr DESTDIR=$(BUILDROOT) install

userland-libraries:
	PATH=$(PATH) make DESTDIR=$(TOOLROOT) PREFIX=/usr -C usr.lib

toolchain: $(CC)

$(CC):
	mkdir -p $(TOOLROOT)
	PATH=$(PATH) make -C build/toolchain

