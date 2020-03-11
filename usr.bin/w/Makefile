CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

W_OBJECTS += w.o

W = w

all: $(W)

$(W): $(W_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(W) "$(DESTDIR)/$(PREFIX)/bin/w"
clean:
	rm -f $(W_OBJECTS) $(W)
