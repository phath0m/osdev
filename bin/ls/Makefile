CC=i686-elysium-gcc
LD=i686-elysium-gcc

LS_OBJECTS += ls.o

LS = ls

all: $(LS)

$(LS): $(LS_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LS) "$(DESTDIR)/$(PREFIX)/bin/ls"
clean:
	rm -f $(LS_OBJECTS) $(LS)
