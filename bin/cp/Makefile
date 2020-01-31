CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

CP_OBJECTS += main.o

CP = cp

all: $(CP)

$(CP): $(CP_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(CP) "$(DESTDIR)/$(PREFIX)/bin/cp"
clean:
	rm -f $(CP_OBJECTS) $(CP)
