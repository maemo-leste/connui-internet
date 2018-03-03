conndlgslib_LTLIBRARIES = \
	libiap_conndlg.la

libiap_conndlg_la_SOURCES = \
	iap-conndlg.c

libiap_conndlg_la_CFLAGS = \
	$(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) $(MCE_CFLAGS)
libiap_conndlg_la_LIBADD = \
	$(LIBHILDON_LIBS) $(CONNUI_LIBS) $(MCE_LIBS)

libiap_conndlg_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = \
		-Wall -Werror \
		-DG_LOG_DOMAIN=\"$(PACKAGE)\"

