PREFIX = /usr

all:
	

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install compiz-deskmenu $(DESTDIR)$(PREFIX)/bin
	install compiz-deskmenu-backend $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/dbus-1/services/
	install org.compiz_fusion.deskmenu.service $(DESTDIR)$(PREFIX)/share/dbus-1/services
	
uninstall:
	-rm $(DESTDIR)$(PREFIX)/bin/compiz-deskmenu
	-rm $(DESTDIR)$(PREFIX)/bin/compiz-deskmenu-backend
	-rm $(DESTDIR)$(PREFIX)/share/dbus-1/services/org.compiz_fusion.deskmenu.service
