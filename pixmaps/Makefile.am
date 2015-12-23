icondir = $(datadir)/icons/gnome
pixmapdir = $(datadir)/pixmaps/@PACKAGE_NAME@

more_icons = @PACKAGE_NAME@-logo.png @PACKAGE_NAME@-full-icon.png

theme_icons = \
	gnome_actions_scalable_call-hold-symbolic.svg           \
	gnome_actions_scalable_call-transfer-symbolic.svg       \
	gnome_actions_scalable_call-transfer-symbolic-rtl.svg   \
        gnome_actions_scalable_mute-video-symbolic.svg          \
        gnome_actions_16x16_phone-hang-up.png                   \
        gnome_actions_16x16_phone-pick-up.png                   \
        gnome_actions_24x24_phone-hang-up.png                   \
        gnome_actions_24x24_phone-pick-up.png                   \
        gnome_status_16x16_im-message.png                       \
        gnome_status_16x16_user-inacall.png                     \
        gnome_status_128x128_call-cleared.png                   \
        gnome_status_128x128_call-incoming-answered.png         \
        gnome_status_128x128_call-incoming.png                  \
        gnome_status_128x128_call-on-hold.png                   \
        gnome_status_128x128_call-outgoing-answered.png         \
        gnome_status_128x128_call-outgoing-busy.png             \
        gnome_status_128x128_call-outgoing-unreachable.png      \
        gnome_status_128x128_call-outgoing-wrong.png            \
        gnome_status_128x128_call-outgoing.png                  \
        hicolor_apps_16x16_@PACKAGE_NAME@.png                   \
        hicolor_apps_22x22_@PACKAGE_NAME@.png                   \
        hicolor_apps_24x24_@PACKAGE_NAME@.png                   \
        hicolor_apps_32x32_@PACKAGE_NAME@.png                   \
        hicolor_apps_48x48_@PACKAGE_NAME@.png                   \
        hicolor_apps_64x64_@PACKAGE_NAME@.png                   \
        hicolor_apps_72x72_@PACKAGE_NAME@.png                   \
        hicolor_apps_128x128_@PACKAGE_NAME@.png

EXTRA_DIST= $(more_icons) $(theme_icons)

install-data-hook: install-icons update-icon-cache

uninstall-hook: uninstall-icons update-icon-cache

install-icons:
	for icon in $(theme_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;
	for icon in $(more_icons); do \
		mkdir -p $(DESTDIR)$(datadir)/pixmaps/@PACKAGE_NAME@; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/pixmaps/@PACKAGE_NAME@/; \
	done;

uninstall-icons:
	for icon in $(theme_icons); do \
		THEME=`echo $$icon | cut -d_ -f1`; \
		CONTEXT=`echo $$icon | cut -d_ -f2`; \
		SIZE=`echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;
	for icon in $(more_icons); do \
		rm -f $(srcdir)/$$icon $(DESTDIR)$(datadir)/pixmaps/@PACKAGE_NAME@/$$icon; \
	done;

gtk_update_icon_cache = gtk-update-icon-cache-3.0 -f -t $(DESTDIR)$(datadir)/icons/gnome/

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
