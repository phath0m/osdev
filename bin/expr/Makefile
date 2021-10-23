CC=i686-elysium-gcc
LD=i686-elysium-gcc

EXPR_OBJECTS += expr.o

EXPR = expr

all: $(EXPR)

$(EXPR): $(EXPR_OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -lgcc
%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@
install:
	cp $(EXPR) "$(DESTDIR)/$(PREFIX)/bin/expr"
clean:
	rm -f $(EXPR_OBJECTS) $(EXPR)
