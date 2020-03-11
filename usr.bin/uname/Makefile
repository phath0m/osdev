CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

UNAME_OBJECTS += uname.o

UNAME = uname

all: $(UNAME)

$(UNAME): $(UNAME_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(UNAME) "$(DESTDIR)/$(PREFIX)/bin/uname"
clean:
	rm -f $(UNAME_OBJECTS) $(UNAME)
