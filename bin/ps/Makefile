CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lcollections

PS_OBJECTS += main.o

PS = ps

all: $(PS)

$(PS): $(PS_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(PS) "$(DESTDIR)/$(PREFIX)/bin/ps"
clean:
	rm -f $(PS_OBJECTS) $(PS)
