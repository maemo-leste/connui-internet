hildondesktoplib_LTLIBRARIES = \
	connui-internet-status-menu-item.la

connui_internet_status_menu_item_la_SOURCES = \
	status-menu-item.c

connui_internet_status_menu_item_la_CFLAGS = \
	$(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS)
connui_internet_status_menu_item_la_LIBADD = \
	$(LIBHILDON_LIBS) $(CONNUI_LIBS)

connui_internet_status_menu_item_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = -Wall -Werror -DG_LOG_DOMAIN=\"$(PACKAGE)\"

hildonstatusmenudesktopentry_DATA = connui-internet.desktop
EXTRA_DIST = $(hildonstatusmenudesktopentry_DATA)
