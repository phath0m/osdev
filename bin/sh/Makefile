CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

SHELL_OBJECTS += dict.o
SHELL_OBJECTS += list.o
SHELL_OBJECTS += parser.o
SHELL_OBJECTS += shell.o
SHELL_OBJECTS += tokenizer.o

SH = ../../initrd/bin/sh

all: $(SH)

$(SH): $(SHELL_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f $(SHELL_OBJECTS)
