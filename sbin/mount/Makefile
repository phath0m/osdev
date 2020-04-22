CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

MOUNT_OBJECTS += mount.o

MOUNT = mount

all: $(MOUNT)

$(MOUNT): $(MOUNT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(MOUNT) "$(DESTDIR)/$(PREFIX)/bin/mount"
clean:
	rm -f $(MOUNT_OBJECTS) $(MOUNT)
