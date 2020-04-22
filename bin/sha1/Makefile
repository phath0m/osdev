CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

SHA1_OBJECTS += sha1.o
SHA1_OBJECTS += sha1_impl.o

SHA1 = sha1

all: $(SHA1)

$(SHA1): $(SHA1_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(SHA1) "$(DESTDIR)/$(PREFIX)/bin/sha1"
clean:
	rm -f $(SHA1_OBJECTS) $(SHA1)
