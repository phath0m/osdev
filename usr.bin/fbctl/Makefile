CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

FBCTL_OBJECTS += fbctl.o

FBCTL = fbctl

all: $(FBCTL)

$(FBCTL): $(FBCTL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(FBCTL) "$(DESTDIR)/$(PREFIX)/bin/fbctl"
clean:
	rm -f $(FBCTL_OBJECTS) $(FBCTL)
