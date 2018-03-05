AM_CFLAGS = -Wall -Werror $(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) \
	    -DG_LOG_DOMAIN=\"$(PACKAGE)\"
AM_LDFLAGS = $(LIBHILDON_LIBS) $(CONNUI_LIBS) -module -avoid-version

conndlgslib_LTLIBRARIES = \
	libiap_conndlg.la \
	libiap_dialog_disconnect.la

libiap_conndlg_la_SOURCES = iap-conndlg.c
libiap_conndlg_la_CFLAGS = $(AM_CFLAGS) $(MCE_CFLAGS)
libiap_conndlg_la_LIBADD = $(LIBHILDON_LIBS) $(CONNUI_LIBS) $(MCE_LIBS)

libiap_dialog_disconnect_la_SOURCES = iap-dialog-disconnect.c
