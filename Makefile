# Makefile for Badwolf, no need for meson/ninja or ./configure

PREFIX = /usr/local
PKGCONFIG = pkg-config
CC = cc
CFLAGS = -g -O2 -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wconversion -Wsign-conversion -Werror=implicit-function-declaration -Werror=implicit-int -Werror=vla \
         -DDATADIR=\"$(PREFIX)/share/badwolf\" -DPACKAGE=\"Badwolf\" -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L -DVERSION=\"1.3.0\"
LDFLAGS =
ED = false
MANDOC = true
SHELLCHECK = true
FLAWFINDER = true
REUSE = true

DEPS_CFLAGS = -I/usr/include/gtk-3.0 -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/sysprof-6 -I/usr/include/harfbuzz -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/x86_64-linux-gnu -I/usr/include/webp -I/usr/include/gio-unix-2.0 -I/usr/include/cloudproviders -I/usr/include/atk-1.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/webkitgtk-4.1 -I/usr/include/libsoup-3.0 -pthread
DEPS_LIBS = -lwebkit2gtk-4.1 -lgtk-3 -lgdk-3 -lz -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lsoup-3.0 -lgmodule-2.0 -pthread -lglib-2.0 -lgio-2.0 -ljavascriptcoregtk-4.1 -lgobject-2.0 -lglib-2.0

.PHONY: all clean install uninstall

all: badwolf

badwolf: userscripts.c fmt.c uri.c keybindings.c downloads.c badwolf.c
	$(CC) $(CFLAGS) $(DEPS_CFLAGS) -o $@ $^ $(LDFLAGS) $(DEPS_LIBS)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -p badwolf $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -p badwolf.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	mkdir -p $(DESTDIR)$(PREFIX)/share/badwolf
	cp -p interface.css $(DESTDIR)$(PREFIX)/share/badwolf/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	cp -p badwolf.desktop $(DESTDIR)$(PREFIX)/share/applications/
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/badwolf-1.3.0
	cp -p usr.bin.badwolf README.md $(DESTDIR)$(PREFIX)/share/doc/badwolf-1.3.0/
	@echo
	@echo "Note: An example AppArmor profile has been installed at '$(DESTDIR)$(PREFIX)/share/doc/badwolf-1.3.0/usr.bin.badwolf'"

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/badwolf
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/badwolf.1
	rm -rf $(DESTDIR)$(PREFIX)/share/badwolf
	rm -f $(DESTDIR)$(PREFIX)/share/applications/badwolf.desktop
	rm -rf $(DESTDIR)$(PREFIX)/share/doc/badwolf-1.3.0

clean:
	rm -f badwolf
