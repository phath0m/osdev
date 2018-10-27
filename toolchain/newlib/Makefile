NEWLIB=newlib-2.5.0
NEWLIB_SRC=$(NEWLIB).tar.gz
NEWLIB_URL=ftp://sourceware.org/pub/newlib/$(NEWLIB_SRC)
PREFIX=/usr
SYSROOT=$(shell realpath ../../sysroot)
TARGET=i686-elysium

all: $(NEWLIB)
install:
	cd build && DESTDIR=${SYSROOT} make install
	cp -ar ${SYSROOT}${PREFIX}/${TARGET}/* ${SYSROOT}${PREFIX}
clean:
	rm -rf $(NEWLIB) $(NEWLIB_SRC) build
$(NEWLIB): $(NEWLIB_SRC)
	tar xvfz $(NEWLIB_SRC)
	patch -p0 -i newlib.patch
	mkdir -p "$(NEWLIB)/newlib/libc/sys/elysium"
	cp libc/*.c "$(NEWLIB)/newlib/libc/sys/elysium"
	cp libc/*.asm "$(NEWLIB)/newlib/libc/sys/elysium"
	cp include/sys/*.h "$(NEWLIB)/newlib/libc/include/sys"
	cp include/machine/*.h "$(NEWLIB)/newlib/libc/include/machine"
	cp libc/Makefile.am "$(NEWLIB)/newlib/libc/sys/elysium"
	cp libc/configure.in "$(NEWLIB)/newlib/libc/sys/elysium"
	cd "$(NEWLIB)/newlib/libc/sys" && autoreconf && autoconf
	cd "$(NEWLIB)/newlib/libc/sys/elysium" && autoreconf
	mkdir -p build/i686-elysium/newlib/libc/sys/elysium
	nasm -f elf32 crt/crt0.asm -o "build/i686-elysium/newlib/libc/sys/elysium/crt0.o"
	cd build && ../$(NEWLIB)/configure --prefix="$(PREFIX)" --target=$(TARGET) && make all
$(NEWLIB_SRC):
	wget $(NEWLIB_URL)
