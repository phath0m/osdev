CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS = -lauth

LOGIN_OBJECTS += main.o

LOGIN = login

all: $(LOGIN)

$(LOGIN): $(LOGIN_OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS) -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(LOGIN) "$(DESTDIR)/$(PREFIX)/bin/login"
clean:
	rm -f $(LOGIN_OBJECTS) $(LOGIN)
