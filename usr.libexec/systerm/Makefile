CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lthread -lvt

SYSTERM_OBJECTS += systerm.o

SYSTERM = systerm
SINGLEUSER_SERVICE = systerm.singleuser.service
MULTIUSER_SERVICE = systerm.multiuser.service

all: $(SYSTERM)

$(SYSTERM): $(SYSTERM_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/etc/doit.d"
	cp $(SINGLEUSER_SERVICE) $(MULTIUSER_SERVICE) "$(DESTDIR)/etc/doit.d"
	cp $(SYSTERM) "$(DESTDIR)/$(PREFIX)/libexec/systerm"
clean:
	rm -f $(SYSTERM_OBJECTS) $(SYSTERM)
