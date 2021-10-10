CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

NEWFS_OBJECTS += newfs.o

NEWFS = newfs

all: $(NEWFS)

$(NEWFS): $(NEWFS_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(NEWFS) "$(DESTDIR)/$(PREFIX)/sbin/newfs"
clean:
	rm -f $(NEWFS_OBJECTS) $(NEWFS)
