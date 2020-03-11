CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lxtc -lmemgfx -lthread -lvt

XTCTERM_OBJECTS += xtcterm.o

XTCTERM = xtcterm

all: $(XTCTERM)

$(XTCTERM): $(XTCTERM_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(XTCTERM) "$(DESTDIR)/$(PREFIX)/bin/xtcterm"
clean:
	rm -f $(XTCTERM_OBJECTS) $(XTCTERM)
