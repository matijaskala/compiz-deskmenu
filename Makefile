PREFIX := /usr

CPPFLAGS := `pkg-config --cflags dbus-glib-1 gdk-2.0 gtk+-2.0 libwnck-1.0`
CPPFLAGS_CLIENT := `pkg-config --cflags dbus-glib-1`
WARNINGS := -Wall -Wextra -Wno-unused-parameter
CFLAGS := -O2 $(WARNINGS)
LDFLAGS := `pkg-config --libs dbus-glib-1 gdk-2.0 gtk+-2.0 libwnck-1.0`
LDFLAGS_CLIENT := `pkg-config --libs dbus-glib-1`

all: compiz-deskmenu-menu compiz-deskmenu

compiz-deskmenu: deskmenu.c deskmenu-common.h
	$(CC) $(CPPFLAGS_CLIENT) $(CFLAGS) $(LDFLAGS_CLIENT) -o $@ $<

compiz-deskmenu-menu: deskmenu-menu.c deskmenu-wnck.c deskmenu-wnck.h deskmenu-glue.h deskmenu-common.h deskmenu-menu.h

	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ deskmenu-menu.c deskmenu-wnck.c

deskmenu-glue.h: deskmenu-service.xml
	dbus-binding-tool --mode=glib-server --prefix=deskmenu --output=$@ $^

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install compiz-deskmenu $(DESTDIR)$(PREFIX)/bin/
	install compiz-deskmenu-menu $(DESTDIR)$(PREFIX)/bin/
	install compiz-deskmenu-editor $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)/etc/xdg/compiz/deskmenu/
	install menu.xml $(DESTDIR)/etc/xdg/compiz/deskmenu/
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install org.compiz_fusion.deskmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/

clean:
	rm -f compiz-deskmenu compiz-deskmenu-menu deskmenu-glue.h

