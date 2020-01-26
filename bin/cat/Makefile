CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CAT_OBJECTS += main.o

CAT = cat

all: $(CAT)

$(CAT): $(CAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

install:
	cp $(CAT) "$(DESTDIR)/$(PREFIX)/bin/cat"

clean:
	rm -f $(CAT_OBJECTS)
