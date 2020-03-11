CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

PWD_OBJECTS += pwd.o

PWD = pwd

all: $(PWD)

$(PWD): $(PWD_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(PWD) "$(DESTDIR)/$(PREFIX)/bin/pwd"
clean:
	rm -f $(PWD_OBJECTS) $(PWD)
