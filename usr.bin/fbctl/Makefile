CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

FBCTL_OBJECTS += main.o

FBCTL = ../../build/initrd/usr/bin/fbctl

all: $(FBCTL)

$(FBCTL): $(FBCTL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(FBCTL_OBJECTS)
