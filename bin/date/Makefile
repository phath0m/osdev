CC=i686-elysium-gcc
LD=i686-elysium-gcc

DATE_OBJECTS += date.o

DATE = date

all: $(DATE)

$(DATE): $(DATE_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

install:
	cp $(DATE) "$(DESTDIR)/$(PREFIX)/bin/date"

clean:
	rm -f $(DATE_OBJECTS) $(DATE)
