CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

ECHO_OBJECTS += echo.o

ECHO = echo

all: $(ECHO)

$(ECHO): $(ECHO_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(ECHO) "$(DESTDIR)/$(PREFIX)/bin/echo"
clean:
	rm -f $(ECHO_OBJECTS) $(ECHO)
