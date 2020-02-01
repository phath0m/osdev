CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror
PREFIX=$(shell realpath ../../sysroot/usr)

LIBAUTH_HEADERS += authlib.h
LIBAUTH_OBJECTS += auth.o
LIBAUTH_OBJECTS += sha1.o

LIBAUTH = libauth.a

all: $(LIBAUTH)

$(LIBAUTH): $(LIBAUTH_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LIBAUTH) "$(PREFIX)/lib"
	cp $(LIBAUTH_HEADERS) "$(PREFIX)/include"
clean:
	rm -f $(LIBAUTH_OBJECTS)
