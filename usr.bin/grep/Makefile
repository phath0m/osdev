CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

GREP_OBJECTS += main.o

GREP = grep

all: $(GREP)

$(GREP): $(GREP_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(GREP) "$(DESTDIR)/$(PREFIX)/bin/grep"
clean:
	rm -f $(GREP_OBJECTS) $(GREP)
