CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lmemgfx -linput -lauth

XTCLOGIN_OBJECTS += xtclogin.o

XTCLOGIN = xtclogin
XTCLOGIN_SERVICE=xtclogin.service

all: $(XTCLOGIN)

$(XTCLOGIN): $(XTCLOGIN_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/etc/doit.d"
	mkdir -p "$(DESTDIR)/$(PREFIX)/bin"
	cp $(XTCLOGIN_SERVICE) "$(DESTDIR)/etc/doit.d/$(XTCLOGIN_SERVICE)"
	cp $(XTCLOGIN) "$(DESTDIR)/$(PREFIX)/bin/xtclogin"
clean:
	rm -f $(XTCLOGIN_OBJECTS) $(XTCLOGIN)
