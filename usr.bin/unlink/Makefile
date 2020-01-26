CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

UNLINK_OBJECTS += main.o

UNLINK = unlink

all: $(UNLINK)

$(UNLINK): $(UNLINK_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(UNLINK) "$(DESTDIR)/$(PREFIX)/bin/unlink"
clean:
	rm -f $(UNLINK_OBJECTS) $(UNLINK)
