CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

DOIT_OBJECTS += main.o

DOIT = ../../build/initrd/sbin/doit

all: $(DOIT)

$(DOIT): $(DOIT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(DOIT_OBJECTS)
