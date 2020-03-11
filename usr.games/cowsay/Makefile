CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

COWSAY_OBJECTS += cowsay.o

COWSAY = cowsay

all: $(COWSAY)

$(COWSAY): $(COWSAY_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(COWSAY) "$(DESTDIR)/$(PREFIX)/games/cowsay"
clean:
	rm -f $(COWSAY_OBJECTS) $(COWSAY)
