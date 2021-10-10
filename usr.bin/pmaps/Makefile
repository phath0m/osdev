CC=i686-elysium-gcc
LD=i686-elysium-gcc

PMAPS_OBJECTS += pmaps.o

PMAPS = pmaps

all: $(PMAPS)

$(PMAPS): $(PMAPS_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@
install:
	cp $(PMAPS) "$(DESTDIR)/$(PREFIX)/bin/pmaps"
clean:
	rm -f $(PMAPS_OBJECTS) $(PMAPS)
