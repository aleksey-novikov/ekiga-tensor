SUBDIRS = man sounds pixmaps lib src plugins po

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

### Don't execute make in the help subdirectory if BULD_DOC is set
if HAVE_GNOME_DOC_UTILS
SUBDIRS += help
endif


### Intltool on the desktop, schemas and appdata files
@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

DESKTOP_IN_FILE=org.gnome.@PACKAGE_NAME@.desktop.in.in
DESKTOP_FILE=org.gnome.@PACKAGE_NAME@.desktop

gsettings_SCHEMAS = org.gnome.@PACKAGE_NAME@.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = @PACKAGE_NAME@.convert

@GSETTINGS_RULES@

appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = ekiga.appdata.xml.in
@INTLTOOL_XML_RULE@


### Extra dist
EXTRA_DIST = \
	$(appdata_in_files)		\
	$(DESKTOP_FILE)			\
	doc/using_dbus.html		\
	gnome-doc-utils.make	\
	README					\
	LICENSE         	    \
	$(gsettings_SCHEMAS) 	\
	$(SCHEMAS_FILE) 		\
	$(WIN32_DIST)

Applicationsdir = $(datadir)/applications
Applications_DATA = $(DESKTOP_FILE)

BUILT_SOURCES = revision.h

GIT_REVISION=\"$$(git describe)\"
CACHED_REVISION=$$(cat revision.h 2>/dev/null | cut -c24-)

revision.h:
	@if test -d ".git" -a -n "$$(which git)"; then \
          if test "x$(GIT_REVISION)" != "x$(CACHED_REVISION)"; then \
            echo "#define EKIGA_REVISION $(GIT_REVISION)" > revision.h; \
          fi \
        else \
          if !(test -e "./revision.h"); then\
            echo "#define EKIGA_REVISION \"unknown\"" > revision.h; \
          fi \
        fi

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.ekiga.service

org.gnome.ekiga.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; echo 'Name=org.gnome.ekiga'; echo 'Exec=${bindir}/ekiga --gapplication-service') > $@.tmp && mv $@.tmp $@

### Dist Clear
DISTCLEANFILES=gnome-doc-utils.make org.gnome.ekiga.desktop org.gnome.ekiga.gschema revision.h $(appdata_DATA)

MAINTAINERCLEANFILES= \
	Makefile.in aclocal.m4 compile config.guess config.sub \
	depcomp install-sh ltmain.sh missing config.h.in configure \
	m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 \
	m4/lt~obsolete.m4 m4/gnome-doc-utils.m4
