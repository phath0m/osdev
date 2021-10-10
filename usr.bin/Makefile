export CC=i686-elysium-gcc
export LD=i686-elysium-gcc
export DESTDIR
export PREFIX

MAKE = make

SUBDIRS += chroot
SUBDIRS += env
SUBDIRS += fbctl
SUBDIRS += id
SUBDIRS += kstat
SUBDIRS += unlink
SUBDIRS += grep
SUBDIRS += login
SUBDIRS += mkfifo
SUBDIRS += mknod
SUBDIRS += pfiles
SUBDIRS += pmaps
SUBDIRS += sleep
SUBDIRS += uname
SUBDIRS += w
SUBDIRS += which

SUBDIRS += gallium/gallium

all: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir);)
install: $(SUBDIRS)
	mkdir -p "$(DESTDIR)/$(PREFIX)/bin"
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) install;)
clean: $(SUBDIRS)
	$(foreach subdir,$(SUBDIRS),$(MAKE) -C $(subdir) clean;)
