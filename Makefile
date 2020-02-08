KERNEL = ./sys/kernel.bin
INITRD = build/iso/boot/initrd.img
BUILDROOT = $(shell realpath build/initrd)
TOOLROOT = $(shell realpath build/toolroot)
ISO_IMAGE = build/os.iso

CC=$(TOOLROOT)/bin/i686-elysium-gcc

all: toolchain userland-libraries userland kernel iso

clean:
	rm -f $(ISO_IMAGE) $(KERNEL) $(INITRD)

iso: $(KERNEL) $(INITRD) $(ISO_IMAGE)

$(ISO_IMAGE):
	cp -p $(KERNEL) ./build/iso/boot
	grub2-mkrescue -o $(ISO_IMAGE) build/iso


$(INITRD):
	make -C bin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C sbin PREFIX=/ DESTDIR=$(BUILDROOT) install
	make -C usr.bin PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.games PREFIX=/usr DESTDIR=$(BUILDROOT) install
	make -C usr.libexec PREFIX=/usr DESTDIR=$(BUILDROOT) install
	tar --owner=root -C $(BUILDROOT) -cvf $(INITRD) .

kernel: $(KERNEL)

$(KERNEL):
	make -C sys

userland: 
	mkdir -p $(BUILDROOT)/dev
	mkdir -p $(BUILDROOT)/tmp
	mkdir -p $(BUILDROOT)/var/run
	mkdir -p $(BUILDROOT)/var/adm
	make -C bin
	make -C sbin
	make -C usr.bin
	make -C usr.games
	make -C usr.libexec
	cp -rp etc $(BUILDROOT)

userland-libraries:
	make DESTDIR=$(TOOLROOT) PREFIX=/usr -C usr.lib

toolchain: $(CC)

$(CC):
	mkdir -p $(TOOLROOT)
	make -C build/toolchain

