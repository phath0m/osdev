CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBVT_HEADERS += libvt.h
LIBVT_OBJECTS += libvt.o

LIBVT = libvt.a

all: $(LIBVT)

$(LIBVT): $(LIBVT_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LIBVT) "$(DESTDIR)/$(PREFIX)/lib"
	cp $(LIBVT_HEADERS) "$(DESTDIR)/$(PREFIX)/include"
clean:
	rm -f $(LIBVT_OBJECTS)
