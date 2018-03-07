AM_CFLAGS = -Wall -Werror $(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) \
	    -DG_LOG_DOMAIN=\"$(PACKAGE)\"
AM_LDFLAGS = $(LIBHILDON_LIBS) $(CONNUI_LIBS)

lib_LTLIBRARIES = libconnui_iapsettings.la

libconnui_iapsettings_la_SOURCES = widgets.c
libconnui_iapsettings_la_LIBADD = $(LIBHILDON_LIBS) $(CONNUI_LIBS) $(MCE_LIBS)
