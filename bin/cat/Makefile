CC=i686-elysium-gcc
LD=i686-elysium-gcc

CAT_OBJECTS += cat.o

CAT = cat

all: $(CAT)

$(CAT): $(CAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

install:
	cp $(CAT) "$(DESTDIR)/$(PREFIX)/bin/cat"

clean:
	rm -f $(CAT_OBJECTS) $(CAT)
