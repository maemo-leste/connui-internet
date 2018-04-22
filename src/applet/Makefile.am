hildonpluginlib_LTLIBRARIES = \
	libinternetsettings.la

libinternetsettings_la_SOURCES = internetsettings.c

libinternetsettings_la_CFLAGS = $(HILDON_CFLAGS) $(CONNUI_CFLAGS)
libinternetsettings_la_LIBADD = $(HILDON_LIBS) $(CONNUI_LIBS)
libinternetsettings_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = \
		-Wall -Werror \
		-DG_LOG_DOMAIN=\"$(PACKAGE)\"

