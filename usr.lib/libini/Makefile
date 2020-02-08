CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBINI_HEADERS += libini.h
LIBINI_OBJECTS += ini.o

LIBINI = libini.a

all: $(LIBINI)

$(LIBINI): $(LIBINI_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LIBINI) "$(DESTDIR)/$(PREFIX)/lib"
	cp $(LIBINI_HEADERS) "$(DESTDIR)/$(PREFIX)/include"
clean:
	rm -f $(LIBINI_OBJECTS)
