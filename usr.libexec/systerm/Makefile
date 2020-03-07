CC=i686-elysium-gcc
LD=i686-elysium-gcc
#CC=gcc
#LD=gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lthread -lvt

SYSTERM_OBJECTS += main.o

SYSTERM = systerm

all: $(SYSTERM)

$(SYSTERM): $(SYSTERM_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(SYSTERM) "$(DESTDIR)/$(PREFIX)/libexec/systerm"
clean:
	rm -f $(SYSTERM_OBJECTS) $(SYSTERM)
