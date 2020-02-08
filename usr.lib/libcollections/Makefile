CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBCOLLECTIONS_HEADERS += dict.h
LIBCOLLECTIONS_HEADERS += list.h

LIBCOLLECTIONS_OBJECTS += dict.o
LIBCOLLECTIONS_OBJECTS += list.o

LIBCOLLECTIONS = libcollections.a

all: $(LIBCOLLECTIONS)

$(LIBCOLLECTIONS): $(LIBCOLLECTIONS_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LIBCOLLECTIONS) "$(DESTDIR)/$(PREFIX)/lib"
	mkdir -p "$(DESTDIR)/$(PREFIX)/include/collections"
	cp $(LIBCOLLECTIONS_HEADERS) "$(DESTDIR)/$(PREFIX)/include/collections"
clean:
	rm -f $(LIBCOLLECTIONS_OBJECTS)
