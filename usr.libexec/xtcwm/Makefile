CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lcollections -lthread -lmemgfx -linput

XTCWM_OBJECTS += main.o
XTCWM_OBJECTS += server.o
XTCWM_OBJECTS += window.o

XTCWM = xtcwm

all: $(XTCWM)

$(XTCWM): $(XTCWM_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(XTCWM) "$(DESTDIR)/$(PREFIX)/libexec/xtcwm"
clean:
	rm -f $(XTCWM_OBJECTS) $(XTCWM)
