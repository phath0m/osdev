CC=i686-elysium-gcc
LD=i686-elysium-gcc

DMESG_OBJECTS += dmesg.o

DMESG = dmesg

all: $(DMESG)

$(DMESG): $(DMESG_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

install:
	cp $(DMESG) "$(DESTDIR)/$(PREFIX)/bin/dmesg"

clean:
	rm -f $(DMESG_OBJECTS) $(DMESG)
