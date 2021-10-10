CC=i686-elysium-gcc
LD=i686-elysium-gcc

PFILES_OBJECTS += pfiles.o

PFILES = pfiles

all: $(PFILES)

$(PFILES): $(PFILES_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@
install:
	cp $(PFILES) "$(DESTDIR)/$(PREFIX)/bin/pfiles"
clean:
	rm -f $(PFILES_OBJECTS) $(PFILES)
