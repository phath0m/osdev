CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CAT_OBJECTS += main.o

CAT = ../../build/initrd/bin/cat

all: $(CAT)

$(CAT): $(CAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(CAT_OBJECTS)
