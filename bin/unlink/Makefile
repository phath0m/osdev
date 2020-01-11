CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

UNLINK_OBJECTS += main.o

UNLINK = ../../initrd/bin/unlink

all: $(UNLINK)

$(UNLINK): $(UNLINK_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(UNLINK_OBJECTS)
