CC=i686-elysium-gcc
LD=i686-elysium-gcc

CHMOD_OBJECTS += chmod.o

CHMOD = chmod

all: $(CHMOD)

$(CHMOD): $(CHMOD_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(CHMOD) "$(DESTDIR)/$(PREFIX)/bin/chmod"
clean:
	rm -f $(CHMOD_OBJECTS) $(CHMOD)
