AC_PREREQ([2.52])

AC_INIT([connectivity-settings], patsubst(esyscmd([dpkg-parsechangelog | sed -n '/^Version: \(.*\)$/ {s//\1/;p}']), [
]), [], [connectivity-settings])

LT_CURRENT=1
LT_REVISION=0
LT_AGE=0
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
	Makefile
	connui-iapsettings.pc
	src/Makefile
	src/status-menu-item/Makefile
	src/settings/Makefile
	src/iap/Makefile
	src/applet/Makefile
	data/Makefile
])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AM_GCONF_SOURCE_2

PKG_CHECK_MODULES(OSSO_AF_SETTINGS, osso-af-settings)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)

PKG_CHECK_MODULES(LIBHILDON, libhildondesktop-1)
AC_SUBST(LIBHILDON_CFLAGS)
AC_SUBST(LIBHILDON_LIBS)

PKG_CHECK_MODULES(CONNUI, connui)
AC_SUBST(CONNUI_CFLAGS)
AC_SUBST(CONNUI_LIBS)

PKG_CHECK_MODULES(MCE, mce)
AC_SUBST(MCE_CFLAGS)
AC_SUBST(MCE_LIBS)

PKG_CHECK_MODULES(CONBTUI, conbtui)
AC_SUBST(CONBTUI_CFLAGS)
AC_SUBST(CONBTUI_LIBS)

PKG_CHECK_MODULES(MAEMOSEC, maemosec >= 0.0.3 maemosec-certman >= 0.0.3 )
AC_SUBST(MAEMOSEC_CFLAGS)
AC_SUBST(MAEMOSEC_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES(HILDON, hildon-control-panel >= 0.10.0)
AC_SUBST(HILDON_CFLAGS)
AC_SUBST(HILDON_LIBS)

PKG_CHECK_MODULES(ICD, icd2)
AC_SUBST(ICD_CFLAGS)
AC_SUBST(ICD_LIBS)

dnl Localization
GETTEXT_PACKAGE=osso-connectivity-ui
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "${GETTEXT_PACKAGE}", [gettext package])

dnl Installation directories

iapsettingslibdir=$libdir/iapsettings
AC_SUBST(iapsettingslibdir)


conndlgslibdir="`$PKG_CONFIG --variable=pluginlibdir connui`"
AC_SUBST(conndlgslibdir)

hildondesktoplibdir="`$PKG_CONFIG --variable=hildondesktoplibdir osso-af-settings`"
AC_SUBST(hildondesktoplibdir)

hildonstatusmenudesktopentrydir="`$PKG_CONFIG --variable=hildonstatusmenudesktopentrydir libhildondesktop-1`"
AC_SUBST(hildonstatusmenudesktopentrydir)

hildonpluginlibdir=`$PKG_CONFIG --variable=pluginlibdir hildon-control-panel`
AC_SUBST(hildonpluginlibdir)

hildoncpdesktopentrydir=`$PKG_CONFIG --variable=plugindesktopentrydir hildon-control-panel`
AC_SUBST(hildoncpdesktopentrydir)

AC_ARG_ENABLE(cast-checks,  [  --disable-cast-checks   compile with GLIB cast checks disabled],[cchecks=${enableval}],cchecks=yes)
if test "x$cchecks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE(asserts,      [  --disable-asserts       compile with GLIB assertions disabled],[asserts=${enableval}],asserts=yes)
if test "x$asserts" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_ASSERTS"
fi

AC_ARG_ENABLE(checks,       [  --disable-checks        compile with GLIB checks disabled],[checks=${enableval}],checks=yes)
if test "x$checks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
fi

AC_ARG_ENABLE(debug,     [  --enable-debug          enable printing of debug messages],[ddebug=${enableval}],ddebug=no)
if test "x$ddebug" != "xyes"; then
    CFLAGS="$CFLAGS -DG_DEBUG_DISABLE"
fi

AC_OUTPUT
