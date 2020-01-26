CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

KSTAT_OBJECTS += main.o

KSTAT = kstat

all: $(KSTAT)

$(KSTAT): $(KSTAT_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(KSTAT) "$(DESTDIR)/$(PREFIX)/bin/kstat"
clean:
	rm -f $(KSTAT_OBJECTS) $(KSTAT)
