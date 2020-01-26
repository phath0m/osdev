CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CHMOD_OBJECTS += main.o

CHMOD = chmod

all: $(CHMOD)

$(CHMOD): $(CHMOD_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(CHMOD) "$(DESTDIR)/$(PREFIX)/chmod"
clean:
	rm -f $(CHMOD_OBJECTS) $(CHMOD)
