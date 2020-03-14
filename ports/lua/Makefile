AR=i686-elysium-ar
CC=i686-elysium-gcc
LD=i686-elysium-ld

LUA=lua-5.3.5
LUA_SRC=$(LUA).tar.gz
LUA_URL=https://www.lua.org/ftp/$(LUA_SRC)

all: $(LUA)

$(LUA): $(LUA_SRC)
	tar xvfz $(LUA_SRC)
	make -C $(LUA) CC="$(CC) -std=gnu99" AR="$(AR) rcu" LD="$(LD)" PLAT=generic
$(LUA_SRC):
	wget $(LUA_URL)
clean:
	rm -rf $(LUA) $(LUA_SRC) build
install:
	make -C $(LUA) INSTALL_TOP="$(DESTDIR)/$(PREFIX)" install
