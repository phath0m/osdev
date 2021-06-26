CC=i686-elysium-gcc
LD=i686-elysium-gcc

CP_OBJECTS += cp.o

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
