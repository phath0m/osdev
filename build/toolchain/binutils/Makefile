BINUTILS=binutils-2.26
BINUTILS_SRC=$(BINUTILS).tar.gz
BINUTILS_URL=https://ftp.gnu.org/gnu/binutils/$(BINUTILS_SRC)
PREFIX="/"
SYSROOT=$(shell realpath ../../sysroot)

all: $(BINUTILS)

clean:
	rm -rf $(BINUTILS) $(BINUTILS_SRC) build
install:
	DESTDIR="${SYSROOT}" make -C build install
$(BINUTILS): $(BINUTILS_SRC)
	tar xvfz $(BINUTILS_SRC)
	patch -p0 -i binutils.patch
	cp ./elf_i386_elysium.sh "$(BINUTILS)/ld/emulparams/elf_i386_elysium.sh"
	cp ./elf_x86_64_elysium.sh "$(BINUTILS)/ld/emulparams/elf_x86_64_elysium.sh"
	cd "$(BINUTILS)/ld" && autoreconf
	mkdir -p build
	cd build && ../$(BINUTILS)/configure --target=i686-elysium --prefix="$(PREFIX)" --with-sysroot="$(SYSROOT)" --disable-werror
	cd build && make
$(BINUTILS_SRC):
	wget $(BINUTILS_URL)
