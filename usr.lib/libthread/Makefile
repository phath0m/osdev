CC=i686-elysium-gcc
AR=i686-elysium-ar

CFLAGS = -c -std=gnu99 -Wall -Werror

LIBTHREAD_HEADERS += thread.h
LIBTHREAD_OBJECTS += cond.o
LIBTHREAD_OBJECTS += thread.o
LIBTHREAD_OBJECTS += spinlock.o
LIBTHREAD_OBJECTS += syscalls.o

LIBTHREAD = libthread.a

all: $(LIBTHREAD)

$(LIBTHREAD): $(LIBTHREAD_OBJECTS)
	$(AR) -rcs $@ $^
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	mkdir -p "$(DESTDIR)/$(PREFIX)/include"
	cp $(LIBTHREAD) "$(DESTDIR)/$(PREFIX)/lib"
	cp $(LIBTHREAD_HEADERS) "$(DESTDIR)/$(PREFIX)/include"
clean:
	rm -f $(LIBTHREAD_OBJECTS)
