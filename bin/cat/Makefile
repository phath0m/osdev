CC=i686-pc-elysium-gcc
LD=i686-pc-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CAT_OBJECTS += main.o

CAT = ../../initrd/bin/cat

all: $(CAT)

$(CAT): $(CAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(CAT_OBJECTS)
