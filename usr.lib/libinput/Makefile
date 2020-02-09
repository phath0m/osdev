CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBINPUT_HEADERS += mouse.h
LIBINPUT_OBJECTS += mouse.o

LIBINPUT = libinput.a

all: $(LIBINPUT)

$(LIBINPUT): $(LIBINPUT_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/$(PREFIX)/include/libinput"
	cp $(LIBINPUT) "$(DESTDIR)/$(PREFIX)/lib"
	cp $(LIBINPUT_HEADERS) "$(DESTDIR)/$(PREFIX)/include/libinput"
clean:
	rm -f $(LIBINPUT_OBJECTS)
