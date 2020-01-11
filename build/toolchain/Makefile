AUTOCONF = ../sysroot/bin/autoconf
AUTOMAKE = ../sysroot/bin/automake
BINUTILS = ../sysroot/bin/i686-elysium-as
GCC = ../sysroot/bin/i686-elysium-gcc

export PREFIX=$(shell realpath ../sysroot)

all: $(AUTOCONF) $(AUTOMAKE) $(BINUTILS) $(GCC)

$(AUTOCONF):
	PATH="$(PREFIX)/bin:$$PATH" make -C autoconf && make -C autoconf install
$(AUTOMAKE):
	PATH="$(PREFIX)/bin:$$PATH" make -C automake && make -C automake install
$(BINUTILS):
	PATH="$(PREFIX)/bin:$$PATH" make -C binutils && make -C binutils install
$(GCC):
	PATH="$(PREFIX)/bin:$$PATH" make -C gcc install
