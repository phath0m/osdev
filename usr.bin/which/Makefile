CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

WHICH_OBJECTS += main.o

WHICH = which

all: $(WHICH)

$(WHICH): $(WHICH_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(WHICH) "$(DESTDIR)/$(PREFIX)/bin/which"
clean:
	rm -f $(WHICH_OBJECTS) $(WHICH)
