hildondesktoplib_LTLIBRARIES = \
	connui-internet-status-menu-item.la

connui_internet_status_menu_item_ladir = $(hildondesktoplibdir)
connui_internet_status_menu_item_la_SOURCES = \
	status-menu-item.c

connui_internet_status_menu_item_la_CFLAGS = $(LIBHILDON_CFLAGS) $(HILDON_CFLAGS) $(CONNUI_CFLAGS) -Werror
connui_internet_status_menu_item_la_LIBADD = $(LIBHILDON_LIBS) $(HILDON_LIBS) $(CONNUI_LIBS)

connui_internet_status_menu_item_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = \
		$(ACCOUNTS_CFLAGS)
		-Wall -Werror \
		-DG_LOG_DOMAIN=\"$(PACKAGE)\"

AM_LIBS = $(ACCOUNTS_LIBS)
