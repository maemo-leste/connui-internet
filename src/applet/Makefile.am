AM_CFLAGS = -Wall -Werror -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
	    -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = iapsettings-restart-connection

iapsettings_restart_connection_SOURCES = restart-connection.c
iapsettings_restart_connection_CFLAGS = $(CONNUI_CFLAGS) $(ICD_CFLAGS) \
					-DOSSOLOG_COMPILE=1
iapsettings_restart_connection_LDADD = $(CONNUI_LIBS) $(ICD_LIBS)
iapsettings_restart_connection_LDFLAGS = -Wl,--as-needed

hildonpluginlib_LTLIBRARIES = libinternetsettings.la

libinternetsettings_la_SOURCES = applet_main.c connections.c settings.c
libinternetsettings_la_CFLAGS = $(HILDON_CFLAGS) $(CONNUI_CFLAGS) $(AM_CFLAGS) \
				$(ICD_CFLAGS) -I../settings -DOSSOLOG_COMPILE=1

libinternetsettings_la_LIBADD = $(HILDON_LIBS) $(CONNUI_LIBS) \
				$(top_builddir)/src/settings/libconnui_iapsettings.la

libinternetsettings_la_LDFLAGS = -module -avoid-version -Wl,--no-undefined \
				 -Wl,--as-needed

hildoncpdesktopentry_DATA = osso-connectivity-ui-iapsettings.desktop
EXTRA_DIST = $(hildoncpdesktopentrydir_DATA)
