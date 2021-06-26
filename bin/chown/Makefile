CC=i686-elysium-gcc
LD=i686-elysium-gcc

CHOWN_OBJECTS += chown.o

CHOWN = chown

all: $(CHOWN)

$(CHOWN): $(CHOWN_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(CHOWN) "$(DESTDIR)/$(PREFIX)/bin/chown"
clean:
	rm -f $(CHOWN_OBJECTS) $(CHOWN)
