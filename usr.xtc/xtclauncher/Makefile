CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lxtc -lmemgfx -lcollections

XTCLAUNCHER_OBJECTS += xtclauncher.o

XTCLAUNCHER = xtclauncher

all: $(XTCLAUNCHER)

$(XTCLAUNCHER): $(XTCLAUNCHER_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(XTCLAUNCHER) "$(DESTDIR)/$(PREFIX)/bin/xtclauncher"
clean:
	rm -f $(XTCLAUNCHER_OBJECTS) $(XTCLAUNCHER)
