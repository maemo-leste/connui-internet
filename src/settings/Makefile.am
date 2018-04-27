AM_CFLAGS = -Wall -Werror $(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) \
	    $(MAEMOSEC_CFLAGS) $(GMODULE_CFLAGS) \
	    -DG_LOG_DOMAIN=\"$(PACKAGE)\" -DOSSOLOG_COMPILE=1
AM_LDFLAGS = $(LIBHILDON_LIBS) $(CONNUI_LIBS)

lib_LTLIBRARIES = libconnui_iapsettings.la
libconnui_iapsettings_la_SOURCES = widgets.c \
				   stage.c \
				   mapper.c \
				   easy-wlan.c \
				   easy-gprs.c \
				   advanced.c \
				   wizard.c

libconnui_iapsettings_la_LIBADD = $(LIBHILDON_LIBS) $(CONNUI_LIBS) \
				  $(MCE_LIBS) $(MAEMOSEC_LIBS) \
				  $(GMODULE_LIBS)
