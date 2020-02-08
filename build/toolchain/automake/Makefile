AUTOMAKE=automake-1.12
AUTOMAKE_SRC=$(AUTOMAKE).tar.gz
AUTOMAKE_URL=https://ftp.gnu.org/gnu/automake/$(AUTOMAKE_SRC)

all: $(AUTOMAKE)

clean:
	rm -rf $(AUTOMAKE) $(AUTOMAKE_SRC) build
install:
	make -C build install
$(AUTOMAKE): $(AUTOMAKE_SRC)
	tar xvfz $(AUTOMAKE_SRC)
	mkdir -p build
	cd build && ../$(AUTOMAKE)/configure --prefix="$(PREFIX)"  --disable-werror
	cd build && make
$(AUTOMAKE_SRC):
	wget $(AUTOMAKE_URL)
