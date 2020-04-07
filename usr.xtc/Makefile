export CC=i686-elysium-gcc
export LD=i686-elysium-gcc
export DESTDIR
export PREFIX

MAKE = make

SUBDIRS += xtclauncher
SUBDIRS += xtcterm
SUBDIRS += xtclogin
SUBDIRS += xtcwm

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
install: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/bin"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
