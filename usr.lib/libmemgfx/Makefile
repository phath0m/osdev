CC=i686-elysium-gcc
AR=i686-elysium-ar
RANLIB=i686-elysium-ranlib

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBMEMGFX_HEADERS += canvas.h
LIBMEMGFX_HEADERS += display.h
LIBMEMGFX_HEADERS += sfn.h
LIBMEMGFX_OBJECTS += canvas.o
LIBMEMGFX_OBJECTS += display.o
LIBMEMGFX_OBJECTS += targa.o
LIBMEMGFX_OBJECTS += ssfn2.o

LIBMEMGFX = libmemgfx.a

all: $(LIBMEMGFX)

$(LIBMEMGFX): $(LIBMEMGFX_OBJECTS)
	$(AR) -rcs $@ $^
	$(RANLIB) $@
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LIBMEMGFX) "$(DESTDIR)/$(PREFIX)/lib"
	mkdir -p "$(DESTDIR)/$(PREFIX)/include/libmemgfx"
	cp $(LIBMEMGFX_HEADERS) "$(DESTDIR)/$(PREFIX)/include/libmemgfx"
clean:
	rm -f $(LIBMEMGFX_OBJECTS)
