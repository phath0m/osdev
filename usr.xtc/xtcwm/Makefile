CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lcollections -lthread -lmemgfx -linput

XTCWM_OBJECTS += server.o
XTCWM_OBJECTS += window.o
XTCWM_OBJECTS += wm.o

XTCWM = xtcwm
XTCWM_RC = xtcwm.rc
XTCWM_RC_LOCAL = xtcwm.rc.local
XTCSESSION_RC = xtcsession.rc

all: $(XTCWM)

$(XTCWM): $(XTCWM_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/etc/xtc"
	cp $(XTCWM) $(XTCWM_RC) $(XTCWM_RC_LOCAL) $(XTCSESSION_RC) "$(DESTDIR)/etc/xtc"
	cp $(XTCWM) "$(DESTDIR)/$(PREFIX)/bin/xtcwm"
clean:
	rm -f $(XTCWM_OBJECTS) $(XTCWM)
