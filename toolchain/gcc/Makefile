GCC=gcc-4.9.4
#GCC=gcc-8.2.0
GCC_SRC=$(GCC).tar.gz
GCC_URL=https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC_SRC)
PREFIX="/usr"
SYSROOT="$(shell realpath ../../sysroot)"

all: $(GCC)

clean:
	rm -rf $(GCC) $(GCC_SRC) build
install:
	cd build && DESTDIR="${SYSROOT}" make install-gcc && DESTDIR="${SYSROOT}" make install-target-libgcc
$(GCC): $(GCC_SRC)
	tar xvfz $(GCC_SRC)
	patch -p0 -i gcc.patch
	cp ./elysium.h "$(GCC)/gcc/config/elysium.h"
	cd "$(GCC)/libstdc++-v3" && autoconf
	mkdir -p build
	cd build && ../$(GCC)/configure --target=i686-pc-elysium --prefix="$(PREFIX)" --with-sysroot="$(SYSROOT)" --disable-nls --enable-languages=c,c++ --disable-libssp --disable-werror --enable-languages=c,c++
	cd build && make all-gcc all-target-libgcc
$(GCC_SRC):
	wget $(GCC_URL)
