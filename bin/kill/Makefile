CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

KILL_OBJECTS += main.o

KILL = kill

all: $(KILL)

$(KILL): $(KILL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(KILL) "$(DESTDIR)/$(PREFIX)/bin/kill"
clean:
	rm -f $(KILL_OBJECTS) $(KILL)
