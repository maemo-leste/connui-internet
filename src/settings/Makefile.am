AM_CFLAGS = -Wall -Werror $(LIBHILDON_CFLAGS) $(CONNUI_CFLAGS) \
	    $(MAEMOSEC_CFLAGS) $(GMODULE_CFLAGS) \
	    -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
	    -DIAPSETTINGS_PLUGIN_DIR=\"$(iapsettingslibdir)\"
AM_LDFLAGS = $(LIBHILDON_LIBS) $(CONNUI_LIBS) -Wl,--as-needed

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

connui_iapsettingsinclude_HEADERS = widgets.h \
				    stage.h \
				    wizard.h \
				    advanced.h \
				    easy-wlan.h \
				    easy-gprs.h \
				    mapper.h

connui_iapsettingsincludedir = $(includedir)/connui/iapsettings
