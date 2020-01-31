CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

ENV_OBJECTS += main.o

ENV = env

all: $(ENV)

$(ENV): $(ENV_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(ENV) "$(DESTDIR)/$(PREFIX)/bin/env"
clean:
	rm -f $(ENV_OBJECTS) $(ENV)
