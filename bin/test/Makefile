CC=i686-elysium-gcc
LD=i686-elysium-gcc

TEST_OBJECTS += test.o

TEST = test

all: $(TEST)

$(TEST): $(TEST_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(TEST) "$(DESTDIR)/$(PREFIX)/bin/test"
clean:
	rm -f $(TEST_OBJECTS) $(TEST)
