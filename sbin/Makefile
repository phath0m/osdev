export PREFIX
export DESTDIR

MAKE = make

SUBDIRS += doit
SUBDIRS += mount
SUBDIRS += newfs
SUBDIRS += newtty

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
install: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/sbin"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
