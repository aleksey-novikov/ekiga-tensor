AC_INIT([ekiga],[4.1.0],[http://bugzilla.gnome.org/enter_bug.cgi?product=ekiga])

AC_CANONICAL_TARGET
AC_PREREQ([2.53])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/main.cpp)
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz tar-ustar subdir-objects])

AM_MAINTAINER_MODE([enable])
AC_CONFIG_HEADERS([config.h])

dnl use "make V=1" if you want to see the long awful lines
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_LANG(C++)

dnl ###########################################################################
dnl   Definitions
dnl ###########################################################################

MAJOR_VERSION=4
MINOR_VERSION=1
BUILD_TYPE=ReleaseCode
BUILD_NUMBER=0

PTLIB_REC_VERSION="2.15.1"
OPAL_REC_VERSION="3.15.1"

AC_DEFINE_UNQUOTED(MAJOR_VERSION, $MAJOR_VERSION,[fix])
AC_DEFINE_UNQUOTED(MINOR_VERSION, $MINOR_VERSION,[fix])
AC_DEFINE_UNQUOTED(BUILD_TYPE, $BUILD_TYPE,[fix])
AC_DEFINE_UNQUOTED(BUILD_NUMBER, $BUILD_NUMBER,[fix])

dnl ###########################################################################
dnl   Disable / Enable Main Ekiga Components
dnl ###########################################################################

PKG_PROG_PKG_CONFIG

LT_INIT([dlopen disable-static])

AC_PROG_CC_C99
AC_PROG_CXX
AC_SEARCH_LIBS([strerror],[cposix])
AC_HEADER_STDC

dnl Enable compiler warnings
if test ".$ac_cv_c_compiler_gnu" = .yes; then
   CFLAGS="$CFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow"
   CXXFLAGS="$CXXFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow"
fi

dnl Check the library path
case $host in

  *-*-linux*)
    # Test if the compiler is 64bit
    echo 'int i;' > conftest.$ac_ext
    ekiga_cv_cc_64bit_output=no

    if AC_TRY_EVAL(ac_compile); then
      case `/usr/bin/file conftest.$ac_objext` in
        *"ELF 64"*)
          ekiga_cv_cc_64bit_output=yes
          ;;
      esac
    fi
    rm -rf conftest*
  ;;
esac

case $host_cpu:$ekiga_cv_cc_64bit_output in

  powerpc64:yes | s390x:yes | sparc64:yes | x86_64:yes)
    libname="lib64"
    ;;

  *:*)
    libname="lib"
    ;;
esac

dnl Check the OS type (more types can be added)
AC_MSG_CHECKING(whether OS type is supported)
win32=0

case $target_os in

  solaris*)
    gm_platform="solaris"
    ;;

  linux-gnulp | linux-gnu | linux-gnuspe | linux-gnuabi* | linux-gnueabi* | linux | Linux)
    gm_platform="linux"
    ;;

  gnu* )
    gm_platform="gnu"
    ;;

  FreeBSD* | freebsd* | kfreebsd* | NetBSD* | netbsd* | OpenBSD* | openbsd* )
    gm_platform="freebsd"
    ;;

  darwin* )
    gm_platform="macosx"
    ;;

  mingw* )
    gm_platform="mingw"
    CFLAGS="$CFLAGS -Wl,--enable-auto-import"
    CXXFLAGS="$CXXFLAGS -Wl,--enable-auto-import"
    win32=1
    ;;

  *)
    AC_MSG_ERROR([Only Linux, Hurd, FreeBSD, NetBSD, Solaris and Mac OS X are supported. Please contact the author to know how you can add more OS types.])
    ;;
esac
AC_MSG_RESULT([yes])

AM_CONDITIONAL(WIN32, test "x${win32}" = "x1")



dnl ###############################
dnl   Mandatory intltool support
dnl ###############################
IT_PROG_INTLTOOL([0.35.0])


dnl ###############################
dnl   Mandatory GTK+ support
dnl ###############################
# on Windows the icons are manually copied in the installer
if test "x${gm_platform}" = "xmingw" ; then
   PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.12.0])
else
   PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.12.0 gnome-icon-theme >= 3.0.0])
fi
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.40.0 gmodule-2.0 gobject-2.0 gthread-2.0 gio-2.0])

PKG_CHECK_MODULES([GTK_SIDEBAR], [gtk+-3.0 >=  3.16.0], [have_sidebar="yes"], [have_sidebar="no"])
if test "x${have_sidebar}" = "xyes" ; then
  AC_DEFINE(HAVE_SIDEBAR,1,[GtkSideBar support])
fi
AM_CONDITIONAL(HAVE_SIDEBAR, test "x${have_sidebar}" = "xyes")

AC_ARG_ENABLE([gtk-debug],
              [AS_HELP_STRING([--enable-gtk-debug],[enable GTK+ debug flags (default is disabled)])],
              [if test "x$enableval" = "xyes"; then
                  GTK_CFLAGS="$GTK_CFLAGS -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES"
                  GLIB_CFLAGS="$GLIB_CFLAGS -DG_DISABLE_DEPRECATED"
              fi])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)


dnl ##########################################
dnl   Mandatory GST Clutter & Clutter support
dnl ##########################################
PKG_CHECK_MODULES([CLUTTER], [clutter-1.0])
PKG_CHECK_MODULES([CLUTTER_GTK], [clutter-gtk-1.0])
PKG_CHECK_MODULES([CLUTTER_GST], [clutter-gst-2.0 gstreamer-app-1.0 gstreamer-plugins-base-1.0])

AC_SUBST(CLUTTER_CFLAGS)
AC_SUBST(CLUTTER_LIBS)
AC_SUBST(CLUTTER_GTK_CFLAGS)
AC_SUBST(CLUTTER_GTK_LIBS)
AC_SUBST(CLUTTER_GST_CFLAGS)
AC_SUBST(CLUTTER_GST_LIBS)


dnl ###############################
dnl   Mandatory BOOST support
dnl ###############################
AX_BOOST_BASE([1.53])

CPPFLAGS_save="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_CHECK_HEADER(boost/signals2.hpp,, found_signals2=no)
CPPFLAGS="$CPPFLAGS_save"

if test "x$found_signals2" = "xno"; then
   AC_MSG_ERROR([Could not find BOOST signals2 headers])
fi


dnl ###############################
dnl   Mandatory libXML support
dnl ###############################
PKG_CHECK_MODULES([XML], [libxml-2.0])
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)


dnl ###############################
dnl   GSettings support
dnl ###############################

GLIB_GSETTINGS

dnl ###############################
dnl   GUDev support
dnl ###############################

GUDEV="disabled"
if test "x${gm_platform}" == "xlinux" ; then
  AC_ARG_ENABLE(gudev, AS_HELP_STRING([--enable-gudev],[enable GUDev support (default is enabled)]),
[if test "x$enableval" = "xyes"; then
    enable_gudev=yes
fi],enable_gudev=yes)

  if test "x$enable_gudev" = "xyes"; then
    PKG_CHECK_MODULES([GUDEV], [gudev-1.0], [found_gudev=yes])
    AC_SUBST(GUDEV_CFLAGS)
    AC_SUBST(GUDEV_LIBS)
    GUDEV="enabled"
    AC_DEFINE(HAVE_GUDEV,1,[GUDev support])
  fi
fi

AM_CONDITIONAL(HAVE_GUDEV, test "x$found_gudev" = "xyes")


AM_PROG_AS


dnl ###############################
dnl   DBUS Support
dnl ###############################
DBUS="disabled"
DBUS_SERVICE="disabled"

if test "x${gm_platform}" != "xmingw" ; then
  AC_ARG_ENABLE(dbus, AS_HELP_STRING([--enable-dbus],[enable DBUS support (default is enabled)]),
[if test "x$enableval" = "xyes"; then
  enable_dbus=yes
fi],enable_dbus=yes)
  AC_ARG_ENABLE(dbus-service, AS_HELP_STRING([--enable-dbus-service],[enable DBUS service installation (default is enabled)]),
[if test "x$enableval" = "xyes"; then
  enable_dbus_service=yes
fi],enable_dbus_service=yes)

  if test "x$enable_dbus" = "xyes"; then
    PKG_CHECK_MODULES([DBUS], [dbus-1 >= 0.36 dbus-glib-1 >= 0.36], [found_dbus=yes])
    AC_SUBST(DBUS_CFLAGS)
    AC_SUBST(DBUS_LIBS)
    DBUS="enabled"
    AC_DEFINE(HAVE_DBUS,1,[DBUS support])

    if test "x$found_dbus" = "xyes" -a "x$enable_dbus_service" = "xyes"; then
      DBUS_SERVICE="enabled"
    fi
  fi
fi

AM_CONDITIONAL(HAVE_DBUS, test "x$found_dbus" = "xyes")
AM_CONDITIONAL(INSTALL_DBUS_SERVICE, test "x$DBUS_SERVICE" = "xenabled")


dnl ###########################################################################
dnl PTLib + OPAL Headers and Libraries
dnl ###########################################################################
PKG_CHECK_MODULES([PTLIB], [ptlib >= ${PTLIB_REC_VERSION}])
PKG_CHECK_MODULES([OPAL], [opal >= ${OPAL_REC_VERSION}])

# expat is mandatory for presence (at least)
AC_MSG_CHECKING(for PTLIB support for expat)
PTLIB_EXPAT=`$PKG_CONFIG ptlib --variable=PTLIB_EXPAT`
AC_MSG_RESULT($PTLIB_EXPAT)
if test "x$PTLIB_EXPAT" != "xyes"; then
  AC_MSG_ERROR([You need ptlib expat support to compile ekiga])
fi

# video support is mandatory (too many changes in ekiga to support without video)
AC_MSG_CHECKING(for PTLIB support for video)
PTLIB_VIDEO=`$PKG_CONFIG ptlib --variable=PTLIB_VIDEO`
AC_MSG_RESULT($PTLIB_VIDEO)
if test "x$PTLIB_VIDEO" != "xyes"; then
  AC_MSG_ERROR([You need ptlib video support to compile ekiga])
fi

# AEC is mandatory for good quality
AC_MSG_CHECKING(for OPAL support for AEC)
OPAL_AEC=`$PKG_CONFIG opal --variable=OPAL_AEC`
AC_MSG_RESULT($OPAL_AEC)
if test "x$OPAL_AEC" != "xyes"; then
  AC_MSG_ERROR([You need opal aec support to compile ekiga])
fi

# Make sure ptlib and opal don't force no-exceptions,
# as we need them for boost's signals2
PTLIB_CFLAGS="$PTLIB_CFLAGS -fexceptions"
AC_SUBST(PTLIB_CFLAGS)
OPAL_CFLAGS="$OPAL_CFLAGS -fexceptions"
AC_SUBST(OPAL_CFLAGS)

SUFFIX=
AC_ARG_ENABLE([opal-debug],
              [AS_HELP_STRING([--enable-opal-debug],[link to debug versions of opal and ptlib (opal_d and ptlib_d) (default is disabled)])],
              [if test "x$enableval" = "xyes"; then
                SUFFIX="_d"
              fi])

AC_ARG_ENABLE([static-libs],
              [AS_HELP_STRING([--enable-static-libs],[link to opal and ptlib static libraries (default is disabled)])],
              [if test "x$enableval" = "xyes"; then
                SUFFIX="${SUFFIX}_s"
               fi
              ])

if test "x$SUFFIX" != x; then
    OPAL_LIBS=`$PKG_CONFIG opal --define-variable=suffix=$SUFFIX --libs`
    PTLIB_LIBS=`$PKG_CONFIG ptlib --define-variable=suffix=$SUFFIX --libs`
fi

AC_SUBST(PTLIB_CFLAGS)
AC_SUBST(PTLIB_LIBS)
AC_SUBST(OPAL_CFLAGS)
AC_SUBST(OPAL_LIBS)

# check for mandatory SIP support in opal
SIP=`$PKG_CONFIG --variable=sip opal`
if test "x$SIP" = "x1"; then
  SIP="yes"
else
  SIP=`$PKG_CONFIG --variable=OPAL_SIP opal`
fi

if test "x$SIP" != "xyes"; then
  AC_MSG_ERROR([You have to compile opal with SIP support. SIP support is mandatory.])
fi


dnl ###############################
dnl DEBUG support
dnl ###############################
has_debug=
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],[compile with debug support (default is disabled)])],
              [if test "x$enableval" = "xyes"; then
                CPPFLAGS="-DDEBUG $CPPFLAGS"
                has_debug="yes"
              fi])
AM_CONDITIONAL(DEBUG, test "x$has_debug" = "xyes")


dnl #########################################################################
dnl  Support for internationalization
dnl ########################################################################
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)  # needed by gettext in po directory
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[fix])
AM_GLIB_GNU_GETTEXT


dnl ###########################################################################
dnl  Data and configuration directories for the system
dnl ###########################################################################
gnomedatadir=`eval "echo ${datadir}"`
gnomeconfdir=`eval "echo ${sysconfdir}"`
AC_SUBST(gnomedatadir)
AC_SUBST(gnomeconfdir)

# on Windows, data_dir is obtained at run-time, since it depends on
#   the installation directory, so do not define it (to avoids bugs)
if test "x${gm_platform}" != "xmingw" ; then
   if test "x${prefix}" = "xNONE"; then
      AC_DEFINE_UNQUOTED(DATA_DIR, "${ac_default_prefix}/${DATADIRNAME}",[fix])
   else
      AC_DEFINE_UNQUOTED(DATA_DIR, "${prefix}/${DATADIRNAME}",[fix])
   fi
fi

dnl ###########################################################################
dnl  Plugin support
dnl ###########################################################################

case "$host" in
*-mingw*)
        # libtool really installs in ../bin, so we have to add the dummy!
        # and we have to set PLUGIN_DIR to that directory
        bindir='${exec_prefix}'
        stacklibdir='${exec_prefix}'
        plugindir='${exec_prefix}/plugins'
        PLUGIN_DIR='plugins'
        ;;
*)
        versioned_libdir='${libdir}/AC_PACKAGE_NAME/AC_PACKAGE_VERSION'
        stacklibdir=${versioned_libdir}
        plugindir=${versioned_libdir}/plugins
        PLUGIN_DIR='${plugindir}'
        ;;
esac

PLUGINS_LIBTOOL_FLAGS="-no-undefined -avoid-version -module"
STACKLIB_LDFLAGS='-shared -module -no-undefined -avoid-version'

AC_SUBST(bindir)
AC_SUBST(stacklibdir)
AC_SUBST(plugindir)
AC_SUBST(PLUGINS_LIBTOOL_FLAGS)
AC_SUBST(PLUGIN_DIR)
AC_SUBST(STACKLIB_LDFLAGS)

dnl ###########################################################################
dnl  Output the different Makefiles
dnl ###########################################################################
AC_CONFIG_FILES([Makefile
Doxyfile
org.gnome.ekiga.gschema.xml.in
ekiga.convert
org.gnome.ekiga.desktop.in
lib/Makefile
po/Makefile.in
src/Makefile
pixmaps/Makefile
sounds/Makefile
plugins/Makefile
])
AC_OUTPUT

dnl ###########################################################################
dnl Summary
dnl ###########################################################################

echo ""
echo "================ Final configuration ==================="
echo "          Installing into prefix  :  $prefix"
echo ""
if test "x${gm_platform}" != "xmingw" ; then
echo "                    DBUS support  :  $DBUS"
echo "            DBUS service support  :  $DBUS_SERVICE"
fi
echo ""
echo "                         OS Type  :  $target_os"
echo "                    Machine Type  :  $target_cpu"
echo ""
echo " If all settings are OK, type make and make install"
echo "========================================================"
