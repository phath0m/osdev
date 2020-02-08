SUBDIRS = doit.d
FILES += motd
FILES += passwd
FILES += shadow

install: $(SUBDIRS) $(FILES)
	mkdir -p $(DESTDIR)$(PREFIX)/etc
	$(foreach subdir,$(SUBDIRS),cp -rp $(subdir) $(DESTDIR)$(PREFIX)/etc;)
	$(foreach file,$(FILES),cp $(file) $(DESTDIR)$(PREFIX)/etc;)
