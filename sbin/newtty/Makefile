CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

NEWTTY_OBJECTS += newtty.o

NEWTTY = newtty

all: $(NEWTTY)

$(NEWTTY): $(NEWTTY_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(NEWTTY) "$(DESTDIR)/$(PREFIX)/sbin/newtty"
clean:
	rm -f $(NEWTTY_OBJECTS) $(NEWTTY)
