CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

MKDIR_OBJECTS += main.o

MKDIR = mkdir

all: $(MKDIR)

$(MKDIR): $(MKDIR_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(MKDIR) "$(DESTDIR)/$(PREFIX)/mkdir"
clean:
	rm -f $(MKDIR_OBJECTS)
