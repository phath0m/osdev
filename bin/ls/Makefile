CC=i686-pc-elysium-gcc
LD=i686-pc-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

LS_OBJECTS += main.o

LS = ../../initrd/bin/ls

all: $(LS)

$(LS): $(LS_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(LS_OBJECTS)
