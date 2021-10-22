SUBDIRS += terminal

install: $(SUBDIRS)
	mkdir -p $(DESTDIR)$(PREFIX)/share
	$(foreach subdir,$(SUBDIRS),cp -rp $(subdir) $(DESTDIR)$(PREFIX)/share;)
