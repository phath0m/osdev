CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

KSTAT_OBJECTS += main.o

KSTAT = ../../build/initrd/usr/bin/kstat

all: $(KSTAT)

$(KSTAT): $(KSTAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(KSTAT_OBJECTS)
