CC=i686-pc-elysium-gcc
LD=i686-pc-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

ECHO_OBJECTS += main.o

ECHO = ../../initrd/bin/echo

all: $(ECHO)

$(ECHO): $(ECHO_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(ECHO_OBJECTS)
