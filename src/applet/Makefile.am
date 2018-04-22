bin_PROGRAMS = iapsettings-restart-connection

iapsettings_restart_connection_SOURCES = restart-connection.c
iapsettings_restart_connection_CFLAGS = $(CONNUI_CFLAGS) $(ICD_CFLAGS)
iapsettings_restart_connection_LDADD = $(CONNUI_LIBS) $(ICD_LIBS)

hildonpluginlib_LTLIBRARIES = libinternetsettings.la

libinternetsettings_la_SOURCES = internetsettings.c
libinternetsettings_la_CFLAGS = $(HILDON_CFLAGS) $(CONNUI_CFLAGS)
libinternetsettings_la_LIBADD = $(HILDON_LIBS) $(CONNUI_LIBS)
libinternetsettings_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = -Wall -Werror -DG_LOG_DOMAIN=\"$(PACKAGE)\"

hildoncpdesktopentry_DATA = osso-connectivity-ui-iapsettings.desktop
EXTRA_DIST = $(hildoncpdesktopentrydir_DATA)
