CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

LOGIN_OBJECTS += main.o

LOGIN = ../../build/initrd/usr/bin/login

all: $(LOGIN)

$(LOGIN): $(LOGIN_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(LOGIN_OBJECTS)
