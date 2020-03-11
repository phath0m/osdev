CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

SLEEP_OBJECTS += sleep.o

SLEEP = sleep

all: $(SLEEP)

$(SLEEP): $(SLEEP_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(SLEEP) "$(DESTDIR)/$(PREFIX)/bin/sleep"
clean:
	rm -f $(SLEEP_OBJECTS) $(SLEEP)
