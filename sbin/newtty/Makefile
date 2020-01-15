CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

NEWTTY_OBJECTS += main.o

NEWTTY = ../../build/initrd/sbin/newtty

all: $(NEWTTY)

$(NEWTTY): $(NEWTTY_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(NEWTTY_OBJECTS)
