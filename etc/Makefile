SUBDIRS = doit.d
FILES += group
FILES += motd
FILES += passwd
FILES += shadow
FILES += shrc

install: $(SUBDIRS) $(FILES)
	mkdir -p $(DESTDIR)$(PREFIX)/etc
	$(foreach subdir,$(SUBDIRS),cp -rp $(subdir) $(DESTDIR)$(PREFIX)/etc;)
	$(foreach file,$(FILES),cp $(file) $(DESTDIR)$(PREFIX)/etc;)
