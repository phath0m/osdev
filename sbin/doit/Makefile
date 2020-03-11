CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lcollections -lini

DOIT_OBJECTS += doit.o

DOIT = doit

all: $(DOIT)

$(DOIT): $(DOIT_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(DOIT) "$(DESTDIR)/$(PREFIX)/sbin/doit"
clean:
	rm -f $(DOIT_OBJECTS) $(DOIT)
