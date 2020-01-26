CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

ECHO_OBJECTS += main.o

ECHO = echo

all: $(ECHO)

$(ECHO): $(ECHO_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(ECHO) "$(DESTDIR)/$(PREFIX)/echo"
clean:
	rm -f $(ECHO_OBJECTS)
