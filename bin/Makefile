export CC=i686-elysium-gcc
export LD=i686-elysium-gcc
export PREFIX
export DESTDIR

MAKE = make

SUBDIRS += cat
SUBDIRS += chmod
SUBDIRS += chown
SUBDIRS += echo
SUBDIRS += kill
SUBDIRS += ls
SUBDIRS += sh
SUBDIRS += ps
SUBDIRS += mkdir
SUBDIRS += rmdir

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
install: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/bin"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
