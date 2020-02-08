export PREFIX
export DESTDIR

MAKE = make

SUBDIRS += newlib
SUBDIRS += libauth
SUBDIRS += libcollections
SUBDIRS += libini

all: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/lib"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) && $(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)

