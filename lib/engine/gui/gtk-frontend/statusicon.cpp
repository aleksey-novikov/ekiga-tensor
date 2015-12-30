/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * Ekiga is licensed under the GPL license and as a special exception,
 * you have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination,
 * without applying the requirements of the GNU GPL to the OPAL, OpenH323
 * and PWLIB programs, as long as you do follow the requirements of the
 * GNU GPL for all the rest of the software thus combined.
 */


/*
 *                         statusicon.cpp  -  description
 *                         --------------------------
 *   begin                : Thu Jan 12 2006
 *   copyright            : (C) 2000-2007 by Damien Sandras
 *   description          : High level tray api implementation
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "statusicon.h"

#include "services.h"
#include "ekiga-app.h"
#include "call-core.h"
#include "notification-core.h"
#include "personal-details.h"
#include "scoped-connections.h"

#ifdef HAVE_DBUS
#include <dbus/dbus-glib.h>
#endif

/*
 * The StatusIcon
 */
struct _StatusIconPrivate
{
  GmApplication *app;

  Ekiga::scoped_connections connections;

  int blink_id;
  std::string status;
  bool missed_calls;
  bool blinking;

  gchar *blink_image;

  GtkWidget *popup_menu;
  GtkWidget *ekiga_window;
};

static GObjectClass *parent_class = NULL;
static guint signals = { 0 };

/*
 * Declaration of Callbacks
 */
static void
on_menu_item_activated (GtkWidget *widget,
                        gpointer data);

static void
show_popup_menu_cb (GtkStatusIcon *icon,
                    guint button,
                    guint activate_time,
                    gpointer data);

#ifdef WIN32
static gint
hide_popup_menu_cb (GtkWidget *widget,
                    GdkEventButton *event,
                    gpointer data);
#endif

static void
statusicon_activated_cb (GtkStatusIcon *icon,
                         gpointer data);

static void
status_icon_clicked_cb (G_GNUC_UNUSED GtkWidget* widget,
                        gpointer data);

static void
missed_count_cb (GtkWidget *widget,
		 guint calls,
		 gpointer data);

static gboolean
statusicon_blink_cb (gpointer data);


/*
 * Declaration of local functions
 */
static void
statusicon_start_blinking (StatusIcon *icon,
                           const char *stock_id);

static void
statusicon_stop_blinking (StatusIcon *icon);

static void
statusicon_set_status (StatusIcon *widget,
                       const std::string & presence);

static void
statusicon_set_inacall (StatusIcon *widget,
                        bool inacall);

static void
established_call_cb (boost::shared_ptr<Ekiga::Call> call,
                     gpointer self);

static void
cleared_call_cb (boost::shared_ptr<Ekiga::Call> call,
                 std::string reason,
                 gpointer self);


/*
 * GObject stuff
 */
static void
statusicon_dispose (GObject *obj)
{
  StatusIcon *icon = NULL;

  icon = STATUSICON (obj);

  if (icon->priv->popup_menu) {

    g_object_unref (icon->priv->popup_menu);
    icon->priv->popup_menu = NULL;
  }

  if (icon->priv->blink_image) {

    g_free (icon->priv->blink_image);
    icon->priv->blink_image = NULL;
  }

  parent_class->dispose (obj);
}


static void
statusicon_finalize (GObject *obj)
{
  StatusIcon *self = NULL;

  self = STATUSICON (obj);

  if (self->priv->blink_image)
    g_free (self->priv->blink_image);

  delete self->priv;

  parent_class->finalize (obj);
}


static void
statusicon_class_init (gpointer g_class,
                       G_GNUC_UNUSED gpointer class_data)
{
  GObjectClass *gobject_class = NULL;

  parent_class = (GObjectClass *) g_type_class_peek_parent (g_class);

  gobject_class = (GObjectClass *) g_class;
  gobject_class->dispose = statusicon_dispose;
  gobject_class->finalize = statusicon_finalize;

  signals =
    g_signal_new ("clicked",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (StatusIconClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}


GType
statusicon_get_type ()
{
  static GType result = 0;

  if (result == 0) {

    static const GTypeInfo info = {
      sizeof (StatusIconClass),
      NULL,
      NULL,
      statusicon_class_init,
      NULL,
      NULL,
      sizeof (StatusIcon),
      0,
      NULL,
      NULL
    };

    result = g_type_register_static (GTK_TYPE_STATUS_ICON,
				     "StatusIcon",
				     &info, (GTypeFlags) 0);
  }

  return result;
}


/*
 * Our own stuff
 */

/*
 * Callbacks
 */
static void
on_menu_item_activated (GtkWidget * /*widget*/,
                        gpointer data)
{
  GActionMap *map = G_ACTION_MAP (g_application_get_default());
  if (!map)
    return;

  GAction *action = g_action_map_lookup_action (map, (const char*)data);
  if (action)
    g_action_activate (action, NULL);
}

static void
show_popup_menu_cb (GtkStatusIcon *icon,
                    guint button,
                    guint activate_time,
                    gpointer data)
{
  GtkWidget *popup = NULL;

  popup = GTK_WIDGET (data);

  gtk_menu_popup (GTK_MENU (popup),
                  NULL, NULL,
                  (GtkMenuPositionFunc)gtk_status_icon_position_menu, icon,
                  button, activate_time);
}

#ifdef WIN32
static gint
hide_popup_menu_cb (G_GNUC_UNUSED GtkWidget *widget,
                    G_GNUC_UNUSED GdkEventButton *event,
                    gpointer data)
{
  GtkWidget *popup = GTK_WIDGET (data);

  if (gtk_widget_get_visible (popup)) {
    gtk_menu_popdown (GTK_MENU (popup));
    return TRUE;
  }
  else
    return FALSE;
}
#endif

static void
statusicon_activated_cb (G_GNUC_UNUSED GtkStatusIcon *icon,
                         gpointer data)
{
  StatusIcon *self = STATUSICON (data);

  // No missed calls => signal the gtk+ frontend
  // (should hide/present the main window)
  if (!self->priv->missed_calls) {

    g_signal_emit (self, signals, 0, NULL);
  }
  else {

    // Missed calls => show main window and call history tab (FIXME)
    g_signal_emit (self, signals, 0, NULL);
  }

  // Remove warnings from statusicon
  statusicon_set_status (STATUSICON (data), STATUSICON (data)->priv->status);
  gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (self), NULL);
}

static void
status_icon_clicked_cb (G_GNUC_UNUSED GtkWidget* widget,
                        gpointer data)
{
  StatusIcon *self = STATUSICON (data);
  GtkWidget *window = gm_application_get_ekiga_window (GM_APPLICATION (self->priv->app));

  if (!gtk_widget_get_visible (window)
      || (gdk_window_get_state (GDK_WINDOW (gtk_widget_get_window (window)))
          & GDK_WINDOW_STATE_ICONIFIED)) {
    gtk_widget_show (window);
  }
  else {

    if (gtk_window_has_toplevel_focus (GTK_WINDOW (window)))
      gtk_widget_hide (window);
    else
      gtk_window_present (GTK_WINDOW (window));
  }
}

static void
missed_count_cb (G_GNUC_UNUSED GtkWidget *widget,
		 guint calls,
		 gpointer data)
{
  StatusIcon *self = STATUSICON (data);

  gchar *message = NULL;

  if (calls > 0)
    statusicon_start_blinking (self, "im-message"); // FIXME
  else
    statusicon_stop_blinking (self);

  if (calls > 0) {

    message = g_strdup_printf (ngettext ("You have %d call",
					 "You have %d calls",
					 calls), calls);
    gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (self), message);
    g_free (message);
  }
  else
    gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (self), NULL);

  self->priv->missed_calls = (calls > 0);
}


static gboolean
statusicon_blink_cb (gpointer data)
{
  StatusIcon *statusicon = STATUSICON (data);

  g_return_val_if_fail (data != NULL, false);

  if (statusicon->priv->blinking)
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "im-message");
  else
    statusicon_set_status (statusicon, statusicon->priv->status);

  statusicon->priv->blinking = !statusicon->priv->blinking;

  return true;
}


static void
personal_details_updated_cb (StatusIcon* self,
			     boost::shared_ptr<Ekiga::PersonalDetails> details)
{
  statusicon_set_status (self, details->get_presence ());
}


static void
established_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                     gpointer self)
{
  statusicon_set_inacall (STATUSICON (self), true);
}


static void
cleared_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                 std::string /*reason*/,
                 gpointer self)
{
  statusicon_set_inacall (STATUSICON (self), false);
}


/*
 * Local functions
 */
static void
statusicon_start_blinking (StatusIcon *icon,
                           const char *icon_name)
{
  g_return_if_fail (icon != NULL);

  icon->priv->blink_image = g_strdup (icon_name);
  if (icon->priv->blink_id == -1)
    icon->priv->blink_id = g_timeout_add_seconds (1, statusicon_blink_cb, icon);
}


static void
statusicon_stop_blinking (StatusIcon *self)
{
  if (self->priv->blink_image) {

    g_free (self->priv->blink_image);
    self->priv->blink_image = NULL;
  }

  if (self->priv->blink_id != -1) {

    g_source_remove (self->priv->blink_id);
    self->priv->blink_id = -1;
    self->priv->blinking = false;
  }

  statusicon_set_status (STATUSICON (self), self->priv->status);
}


static void
statusicon_set_status (StatusIcon *statusicon,
                       const std::string & presence)
{
  g_return_if_fail (statusicon != NULL);

  /* Update the status icon */
  if (presence == "away")
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "user-away");
  else if (presence == "busy")
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "user-busy");
  else if (presence == "offline")
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "user-offline");
  else
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "user-available");

  statusicon->priv->status = presence;
}


static void
statusicon_set_inacall (StatusIcon *statusicon,
                        bool inacall)
{
  g_return_if_fail (statusicon != NULL);

  /* Update the status icon */
  if (inacall)
    gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (statusicon), "user-inacall");
  else
    statusicon_set_status (statusicon, statusicon->priv->status);
}

static void
statusicon_on_notification_added (boost::shared_ptr<Ekiga::Notification> notification,
                                  gpointer data)
{
  StatusIcon *self = STATUSICON (data);
  GdkPixbuf* pixbuf = gtk_widget_render_icon_pixbuf (self->priv->ekiga_window,
						     GTK_STOCK_DIALOG_WARNING,
						     GTK_ICON_SIZE_MENU);

  gchar *current_tooltip = gtk_status_icon_get_tooltip_text (GTK_STATUS_ICON (self));
  gchar *tooltip = NULL;
  if (current_tooltip != NULL)
    tooltip = g_strdup_printf ("%s\n%s", current_tooltip, notification->get_title ().c_str ());
  else
    tooltip = g_strdup (notification->get_title ().c_str ());

  gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (self), pixbuf);
  gtk_status_icon_set_tooltip_text (GTK_STATUS_ICON (self), tooltip);
  g_object_unref (pixbuf);

  g_free (current_tooltip);
  g_free (tooltip);
}


static bool
statusicon_should_run (void)
{
  bool shell_running = false;

#ifdef HAVE_DBUS
  DBusGConnection *connection = NULL;
  GError *error = NULL;
  DBusGProxy *proxy = NULL;
  char **name_list = NULL;
  char **name_list_ptr = NULL;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL) {
    g_error_free (error);
    return true;
  }

  /* Create a proxy object for the "bus driver" (name "org.freedesktop.DBus") */
  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  /* Call ListNames method, wait for reply */
  error = NULL;
  if (!dbus_g_proxy_call (proxy, "ListNames", &error, G_TYPE_INVALID,
                          G_TYPE_STRV, &name_list, G_TYPE_INVALID)) {
      g_error_free (error);
      return true;
  }

  /* Print the results */
  for (name_list_ptr = name_list; *name_list_ptr; name_list_ptr++) {
    if (!g_strcmp0 (*name_list_ptr, "org.gnome.Shell")) {
      shell_running = true;
      break;
    }
  }
  g_strfreev (name_list);
  g_object_unref (proxy);
#endif

  return !shell_running;
}


/*
 * Public API
 */
StatusIcon *
status_icon_new (GmApplication *app)
{
  StatusIcon *self = NULL;
  GtkWidget *item = NULL;

  g_return_val_if_fail (GM_IS_APPLICATION (app), NULL);

  if (!statusicon_should_run ())
    return self;

  Ekiga::ServiceCore& core = gm_application_get_core (app);

  boost::signals2::connection conn;

  self = STATUSICON (g_object_new (STATUSICON_TYPE, NULL));
  self->priv = new StatusIconPrivate;

  self->priv->blink_id = -1;
  self->priv->blinking = false;
  self->priv->blink_image = NULL;
  self->priv->missed_calls = false;
  self->priv->app = app;

  boost::shared_ptr<Ekiga::PersonalDetails> details =
    core.get<Ekiga::PersonalDetails> ("personal-details");
  boost::shared_ptr<Ekiga::CallCore> call_core =
    core.get<Ekiga::CallCore> ("call-core");
  boost::shared_ptr<Ekiga::NotificationCore> notification_core =
    core.get<Ekiga::NotificationCore> ("notification-core");

  self->priv->ekiga_window = gm_application_get_ekiga_window (app);

  self->priv->popup_menu = gtk_menu_new ();

  // FIXME: rewrite to GtkBuilder
  item = gtk_menu_item_new_with_label (_("Account"));
  g_signal_connect (item, "activate", G_CALLBACK (on_menu_item_activated), (gpointer)"account");
  gtk_menu_shell_append(GTK_MENU_SHELL (self->priv->popup_menu), item);

  item = gtk_menu_item_new_with_label (_("Preferences"));
  g_signal_connect (item, "activate", G_CALLBACK (on_menu_item_activated), (gpointer)"preferences");
  gtk_menu_shell_append(GTK_MENU_SHELL (self->priv->popup_menu), item);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append(GTK_MENU_SHELL (self->priv->popup_menu), item);

  item = gtk_menu_item_new_with_label (_("Quit"));
  g_signal_connect (item, "activate", G_CALLBACK (on_menu_item_activated), (gpointer)"quit");
  gtk_menu_shell_append(GTK_MENU_SHELL (self->priv->popup_menu), item);

  gtk_widget_show_all (GTK_WIDGET (self->priv->popup_menu));

  statusicon_set_status (self, details->get_presence ());
  notification_core->notification_added.connect (boost::bind (statusicon_on_notification_added,
                                                              _1, (gpointer) self));

  conn = details->updated.connect (boost::bind (&personal_details_updated_cb,
                                                self, details));
  self->priv->connections.add (conn);

  conn = call_core->established_call.connect (boost::bind (&established_call_cb,
                                                           _1, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call_core->cleared_call.connect (boost::bind (&cleared_call_cb,
                                                       _1, _2, (gpointer) self));
  self->priv->connections.add (conn);

  g_signal_connect (self, "popup-menu",
                    G_CALLBACK (show_popup_menu_cb),
                    self->priv->popup_menu);

#ifdef WIN32
  // hide the popup menu when right-click on the icon
  // this should have been done in GTK code in my opinion...
  g_signal_connect (self, "button_press_event",
                    G_CALLBACK (hide_popup_menu_cb), self->priv->popup_menu);
#endif

  g_signal_connect (self, "activate",
                    G_CALLBACK (statusicon_activated_cb), self);

  // FIXME (ekiga_window has no signal now)
  if (self->priv->ekiga_window)
    g_signal_connect (self->priv->ekiga_window, "missed-count",
                      G_CALLBACK (missed_count_cb), self);

  g_signal_connect (self, "clicked",
                    G_CALLBACK (status_icon_clicked_cb), self);

  return self;
}
