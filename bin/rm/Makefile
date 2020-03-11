CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

RM_OBJECTS += rm.o

RM = rm

all: $(RM)

$(RM): $(RM_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(RM) "$(DESTDIR)/$(PREFIX)/bin/rm"
clean:
	rm -f $(RM_OBJECTS) $(RM)
