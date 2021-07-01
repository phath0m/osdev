CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lxtc -lmemgfx

XTCCLOCK_OBJECTS += xtcclock.o

XTCCLOCK = xtcclock

all: $(XTCCLOCK)

$(XTCCLOCK): $(XTCCLOCK_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(XTCCLOCK) "$(DESTDIR)/$(PREFIX)/bin/xtcclock"
clean:
	rm -f $(XTCCLOCK_OBJECTS) $(XTCCLOCK)
