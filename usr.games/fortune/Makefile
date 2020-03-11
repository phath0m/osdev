CC=i686-elysium-gcc
LD=i686-elysium-gcc

CFLAGS = -c -std=gnu99 -Wall -Werror
LDFLAGS =

FORTUNE_OBJECTS += fortune.o

FORTUNE = fortune
FORTUNE_DB = fortunes.txt

all: $(FORTUNE)

$(FORTUNE): $(FORTUNE_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/$(PREFIX)/share/games"
	cp $(FORTUNE) "$(DESTDIR)/$(PREFIX)/games/fortune"
	cp $(FORTUNE_DB) "$(DESTDIR)/$(PREFIX)/share/games/fortunes.txt"
clean:
	rm -f $(FORTUNE_OBJECTS) $(FORTUNE)
