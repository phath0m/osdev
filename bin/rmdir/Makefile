CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

RMDIR_OBJECTS += rmdir.o

RMDIR = rmdir

all: $(RMDIR)

$(RMDIR): $(RMDIR_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(RMDIR) "$(DESTDIR)/$(PREFIX)/bin/rmdir"
clean:
	rm -f $(RMDIR_OBJECTS) $(RMDIR)
