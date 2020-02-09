CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

MKNOD_OBJECTS += main.o

MKNOD = mkdir

all: $(MKNOD)

$(MKNOD): $(MKNOD_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(MKNOD) "$(DESTDIR)/$(PREFIX)/bin/mknod"
clean:
	rm -f $(MKNOD_OBJECTS) $(MKNOD)
