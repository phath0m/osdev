CC=i686-elysium-gcc
LD=i686-elysium-gcc

MKDIR_OBJECTS += mkdir.o

MKDIR = mkdir

all: $(MKDIR)

$(MKDIR): $(MKDIR_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(MKDIR) "$(DESTDIR)/$(PREFIX)/bin/mkdir"
clean:
	rm -f $(MKDIR_OBJECTS) $(MKDIR)
