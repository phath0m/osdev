CC=i686-elysium-gcc
AR=i686-elysium-ar
RANLIB=i686-elysium-ranlib

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBXTC_HEADERS += libxtc.h
LIBXTC_HEADERS += xtcprotocol.h
LIBXTC_OBJECTS += xtc_client.o

LIBXTC = libxtc.a

all: $(LIBXTC)

$(LIBXTC): $(LIBXTC_OBJECTS)
	$(AR) -rcs $@ $^
	$(RANLIB) $@
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/$(PREFIX)/include"
	cp $(LIBXTC) "$(DESTDIR)/$(PREFIX)/lib"
	cp $(LIBXTC_HEADERS) "$(DESTDIR)/$(PREFIX)/include"
clean:
	rm -f $(LIBXTC_OBJECTS)
