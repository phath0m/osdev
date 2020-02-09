CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

ID_OBJECTS += main.o

ID = id

all: $(ID)

$(ID): $(ID_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(ID) "$(DESTDIR)/$(PREFIX)/bin/id"
clean:
	rm -f $(ID_OBJECTS) $(ID)
