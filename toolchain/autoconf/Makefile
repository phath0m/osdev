AUTOCONF=autoconf-2.64
AUTOCONF_SRC=$(AUTOCONF).tar.gz
AUTOCONF_URL=https://ftp.gnu.org/gnu/autoconf/$(AUTOCONF_SRC)
PREFIX=$(shell realpath ../../sysroot)

all: $(AUTOCONF)

clean:
	rm -rf $(AUTOCONF) $(AUTOCONF_SRC) build
install:
	make -C build install
$(AUTOCONF): $(AUTOCONF_SRC)
	tar xvfz $(AUTOCONF_SRC)
	mkdir -p build
	cd build && ../$(AUTOCONF)/configure --prefix="$(PREFIX)"  --disable-werror
	cd build && make
$(AUTOCONF_SRC):
	wget $(AUTOCONF_URL)
