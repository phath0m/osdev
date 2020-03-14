AR=i686-elysium-ar
CC=i686-elysium-gcc
LD=i686-elysium-ld

KILO=kilo/kilo
KILO_SRC=kilo
KILO_URL=https://github.com/antirez/kilo

all: $(KILO)

$(KILO): $(KILO_SRC)
	gmake -C $(KILO_SRC) CC=$(CC)
$(KILO_SRC):
	git clone $(KILO_URL)
clean:
	rm -rf $(KILO) $(KILO_SRC) build
install:
	cp $(KILO) "$(DESTDIR)/$(PREFIX)/bin/kilo"
