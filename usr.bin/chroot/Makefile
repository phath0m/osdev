CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CHROOT_OBJECTS += chroot.o

CHROOT = chroot

all: $(CHROOT)

$(CHROOT): $(CHROOT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(CHROOT) "$(DESTDIR)/$(PREFIX)/bin/chroot"
clean:
	rm -f $(CHROOT_OBJECTS) $(CHROOT)
