CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

COWSAY_OBJECTS += main.o

COWSAY = ../../initrd/usr/games/cowsay

all: $(COWSAY)

$(COWSAY): $(COWSAY_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(COWSAY_OBJECTS)
