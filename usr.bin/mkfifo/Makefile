CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

MKFIFO_OBJECTS += main.o

MKFIFO = mkfifo

all: $(MKFIFO)

$(MKFIFO): $(MKFIFO_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(MKFIFO) "$(DESTDIR)/$(PREFIX)/bin/mkfifo"
clean:
	rm -f $(MKFIFO_OBJECTS) $(MKFIFO)
