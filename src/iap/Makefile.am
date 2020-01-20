AM_CFLAGS = -Wall -Werror $(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) \
	    -DG_LOG_DOMAIN=\"$(PACKAGE)\"
AM_LDFLAGS = -Wl,--as-needed $(LIBHILDON_LIBS) $(CONNUI_LIBS) -module -avoid-version

conndlgslib_LTLIBRARIES = \
	libiap_conndlg.la \
	libiap_dialog_disconnect.la \
	libiap_dialog_password.la \
	libiap_dialog_save.la \
	libiap_dialog_others.la

libiap_conndlg_la_SOURCES = iap-conndlg.c
libiap_conndlg_la_CFLAGS = $(AM_CFLAGS) $(MCE_CFLAGS) -I../settings
libiap_conndlg_la_LIBADD = $(LIBHILDON_LIBS) $(CONNUI_LIBS) $(MCE_LIBS) \
			   $(top_builddir)/src/settings/libconnui_iapsettings.la

libiap_dialog_disconnect_la_SOURCES = iap-dialog-disconnect.c

libiap_dialog_save_la_SOURCES = iap-dialog-save.c

libiap_dialog_password_la_SOURCES = iap-dialog-password.c

libiap_dialog_others_la_SOURCES = iap-dialog-others.c
libiap_dialog_others_la_CFLAGS = $(AM_CFLAGS) $(MCE_CFLAGS) $(CONBTUI_CFLAGS)
libiap_dialog_others_la_LIBADD = \
				$(LIBHILDON_LIBS) $(CONNUI_LIBS) \
				$(MCE_LIBS) $(CONBTUI_LIBS) \
				$(top_builddir)/src/settings/libconnui_iapsettings.la
