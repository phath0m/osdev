CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lcollections

SHELL_OBJECTS += parser.o
SHELL_OBJECTS += shell.o
SHELL_OBJECTS += tokenizer.o

SH = sh

all: $(SH)

$(SH): $(SHELL_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(SH) "$(DESTDIR)/$(PREFIX)/bin/sh"
clean:
	rm -f $(SHELL_OBJECTS)
