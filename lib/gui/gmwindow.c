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
 *                         gmwindow.c -  description
 *                         -------------------------
 *   begin                : 16 August 2007
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : Implementation of a GtkApplicationWindow able
 *                          to restore its position and size in a GmConf key.
 *
 */

#include "gmwindow.h"

#include <gdk/gdkkeysyms.h>
#include <stdlib.h>

/*
 * The GmWindow
 */
struct _GmWindowPrivate
{
  GtkAccelGroup *accel;
  GtkApplication *application;
  GSettings *settings;
  gboolean hide_on_esc;
  gboolean hide_on_delete;
  gboolean stay_on_top;
  gboolean state_restored;
  gchar *key;
  int x;
  int y;
  int width;
  int height;
};

enum {
  GM_WINDOW_KEY = 1,
  GM_HIDE_ON_ESC = 2,
  GM_HIDE_ON_DELETE = 3,
  GM_STAY_ON_TOP = 4,
  GM_APPLICATION = 5
};

G_DEFINE_TYPE (GmWindow, gm_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean
gm_window_delete_event_cb (GtkWidget *w,
                           gpointer data);

static void
window_realize_cb (GtkWidget *w,
		   gpointer data);

static void
window_show_cb (GtkWidget *w,
                gpointer data);

static void
window_hide_cb (GtkWidget *w,
		gpointer data);

static gboolean
gm_window_configure_event (GtkWidget *widget,
                           GdkEventConfigure *event);

/*
 * GObject stuff
 */
static void
gm_window_finalize (GObject *obj)
{
  GmWindow *self = NULL;

  self = GM_WINDOW (obj);

  g_free (self->priv->key);
  self->priv->key = NULL;

  if (self->priv->settings)
    g_clear_object (&self->priv->settings);
  self->priv->settings = NULL;

  G_OBJECT_CLASS (gm_window_parent_class)->finalize (obj);
}


static void
gm_window_get_property (GObject *obj,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *spec)
{
  GmWindow *self = NULL;

  self = GM_WINDOW (obj);
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GM_TYPE_WINDOW, GmWindowPrivate);

  switch (prop_id) {

  case GM_WINDOW_KEY:
    g_value_set_string (value, self->priv->key);
    break;

  case GM_HIDE_ON_ESC:
    g_value_set_boolean (value, self->priv->hide_on_esc);
    break;

  case GM_HIDE_ON_DELETE:
    g_value_set_boolean (value, self->priv->hide_on_delete);
    break;

  case GM_STAY_ON_TOP:
    g_value_set_boolean (value, self->priv->stay_on_top);
    break;

  case GM_APPLICATION:
    g_value_set_pointer (value, self->priv->application);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, spec);
    break;
  }
}


static void
gm_window_set_property (GObject *obj,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *spec)
{
  GmWindow *self = NULL;
  const gchar *str = NULL;

  self = GM_WINDOW (obj);
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GM_TYPE_WINDOW, GmWindowPrivate);

  switch (prop_id) {

  case GM_WINDOW_KEY:
    if (self->priv->key)
      g_free (self->priv->key);
    str = g_value_get_string (value);
    self->priv->key = g_strdup (str ? str : "");
    if (self->priv->settings)
      g_clear_object (&self->priv->settings);
    self->priv->settings = g_settings_new (self->priv->key);
    break;

  case GM_HIDE_ON_ESC:
    self->priv->hide_on_esc = g_value_get_boolean (value);
    gtk_accel_group_disconnect_key (self->priv->accel, GDK_KEY_Escape, (GdkModifierType) 0);
    if (!self->priv->hide_on_esc)
      gtk_accel_group_connect (self->priv->accel, GDK_KEY_Escape, (GdkModifierType) 0, GTK_ACCEL_LOCKED,
                               g_cclosure_new_swap (G_CALLBACK (gtk_widget_destroy), (gpointer) self, NULL));
    else
      gtk_accel_group_connect (self->priv->accel, GDK_KEY_Escape, (GdkModifierType) 0, GTK_ACCEL_LOCKED,
                               g_cclosure_new_swap (G_CALLBACK (gtk_widget_hide), (gpointer) self, NULL));
    break;

  case GM_HIDE_ON_DELETE:
    self->priv->hide_on_delete = g_value_get_boolean (value);
    break;

  case GM_STAY_ON_TOP:
    self->priv->stay_on_top = g_value_get_boolean (value);
    gtk_window_set_keep_above (GTK_WINDOW (self), self->priv->stay_on_top);
    break;

  case GM_APPLICATION:
    self->priv->application = g_value_get_pointer (value);
    if (self->priv->application)
      gtk_application_add_window (GTK_APPLICATION (self->priv->application),
                                  GTK_WINDOW (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, spec);
    break;
  }
}


static void
gm_window_class_init (GmWindowClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *spec = NULL;

  g_type_class_add_private (klass, sizeof (GmWindowPrivate));

  gobject_class->finalize = gm_window_finalize;
  gobject_class->get_property = gm_window_get_property;
  gobject_class->set_property = gm_window_set_property;

  spec = g_param_spec_string ("key", "Key", "Key",
                              NULL, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, GM_WINDOW_KEY, spec);

  spec = g_param_spec_boolean ("hide_on_esc", "Hide on Escape", "Hide on Escape",
                               TRUE, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, GM_HIDE_ON_ESC, spec);

  spec = g_param_spec_boolean ("hide_on_delete", "Hide on delete-event",
                               "Hide on delete-event (or just relay the event)",
			       TRUE, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, GM_HIDE_ON_DELETE, spec);

  spec = g_param_spec_boolean ("stay_on_top", "Stay on top",
                               "Indicates if the window should stay on top of other windows",
                               FALSE, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, GM_STAY_ON_TOP, spec);

  spec = g_param_spec_pointer ("application", "GtkApplication",
                               "GtkApplication to which the GtkApplicationWindow is associated",
                               (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class, GM_APPLICATION, spec);
}


static void
gm_window_init (GmWindow* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GM_TYPE_WINDOW, GmWindowPrivate);
  self->priv->application = NULL;
  self->priv->settings = NULL;
  self->priv->key = g_strdup ("");
  self->priv->hide_on_esc = TRUE;
  self->priv->hide_on_delete = TRUE;
  self->priv->state_restored = FALSE;

  self->priv->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (self), self->priv->accel);

  g_signal_connect (self, "delete-event",
		    G_CALLBACK (gm_window_delete_event_cb), NULL);

  g_signal_connect (self, "show",
                    G_CALLBACK (window_show_cb), self);

  g_signal_connect (self, "realize",
                    G_CALLBACK (window_realize_cb), self);

  g_signal_connect (self, "hide",
                    G_CALLBACK (window_hide_cb), self);

  g_signal_connect (self, "configure-event",
                    G_CALLBACK (gm_window_configure_event), self);
}


/*
 * Our own stuff
 */
static gboolean
gm_window_delete_event_cb (GtkWidget *w,
                           G_GNUC_UNUSED gpointer data)
{
  GmWindow* self = NULL;

  self = GM_WINDOW (w);

  if (self->priv->hide_on_delete) {
    gtk_widget_hide (w);
    return TRUE;
  }

  return FALSE;
}


static void
window_realize_cb (GtkWidget *w,
		   G_GNUC_UNUSED gpointer data)
{
  GmWindow *self = NULL;

  self = GM_WINDOW (w);

  g_return_if_fail (self);

  gm_window_restore (self);
  self->priv->state_restored = TRUE;

  gtk_widget_realize (GTK_WIDGET (w));
}


static void
window_show_cb (GtkWidget *w,
                G_GNUC_UNUSED gpointer data)
{
  GmWindow *self = NULL;

  self = GM_WINDOW (w);

  g_return_if_fail (self);

  if (!self->priv->state_restored)
    gm_window_restore (self);
}



static void
window_hide_cb (GtkWidget *w,
                G_GNUC_UNUSED gpointer data)
{
  GmWindow *self = NULL;

  g_return_if_fail (w != NULL);

  self = GM_WINDOW (w);

  gm_window_save (self);
  self->priv->state_restored = FALSE;
}


static gboolean
gm_window_configure_event (GtkWidget *self,
                           GdkEventConfigure *event)
{
  gtk_window_get_position (GTK_WINDOW (self), &GM_WINDOW (self)->priv->x, &GM_WINDOW (self)->priv->y);

  GM_WINDOW (self)->priv->width = event->width;
  GM_WINDOW (self)->priv->height = event->height;

  return FALSE;
}


/*
 * Public API
 */
GtkWidget *
gm_window_new ()
{
  return GTK_WIDGET (g_object_new (GM_TYPE_WINDOW, NULL));
}


GtkWidget *
gm_window_new_with_key (const char *key)
{
  g_return_val_if_fail (key != NULL, NULL);

  return GTK_WIDGET (g_object_new (GM_TYPE_WINDOW, "key", key, NULL));
}


void
gm_window_save (GmWindow *self)
{
  gchar *size = NULL;
  gchar *position = NULL;

  g_return_if_fail (g_strcmp0 (self->priv->key, "") || self);

  position = g_strdup_printf ("%d,%d", self->priv->x, self->priv->y);
  g_settings_set_string (self->priv->settings, "position", position);
  g_free (position);

  if (gtk_window_get_resizable (GTK_WINDOW (self))) {

    size = g_strdup_printf ("%d,%d", self->priv->width, self->priv->height);
    g_settings_set_string (self->priv->settings, "size", size);
    g_free (size);
  }
}


void
gm_window_restore (GmWindow *self)
{
  int x = 0;
  int y = 0;

  gchar *size = NULL;
  gchar *position = NULL;
  gchar **couple = NULL;

  g_return_if_fail (g_strcmp0 (self->priv->key, "") && self);

  if (gtk_window_get_resizable (GTK_WINDOW (self))) {

    size = g_settings_get_string (self->priv->settings, "size");
    if (size)
      couple = g_strsplit (size, ",", 0);

    if (couple && couple [0])
      x = atoi (couple [0]);
    if (couple && couple [1])
      y = atoi (couple [1]);

    if (x > 0 && y > 0) {
      gtk_window_resize (GTK_WINDOW (self), x, y);
    }

    g_strfreev (couple);
    g_free (size);
  }

  position = g_settings_get_string (self->priv->settings, "position");
  if (position)
    couple = g_strsplit (position, ",", 0);

  if (couple && couple [0])
    x = atoi (couple [0]);
  if (couple && couple [1])
    y = atoi (couple [1]);

  if (x != 0 && y != 0)
    gtk_window_move (GTK_WINDOW (self), x, y);

  g_strfreev (couple);
  couple = NULL;
  g_free (position);
}


void
gm_window_get_size (GmWindow *self,
                    int *x,
                    int *y)
{
  gchar *conf_key_size = NULL;
  gchar *size = NULL;
  gchar **couple = NULL;

  g_return_if_fail (GM_IS_WINDOW (self) && x != NULL && y != NULL);

  conf_key_size = g_strdup_printf ("%s/size", self->priv->key);
  size = g_settings_get_string (self->priv->settings, "size");
  if (size)
    couple = g_strsplit (size, ",", 0);

  if (x && couple && couple [0])
    *x = atoi (couple [0]);
  if (y && couple && couple [1])
    *y = atoi (couple [1]);

  g_free (conf_key_size);
  g_free (size);
  g_strfreev (couple);
}


void
gm_window_set_hide_on_delete (GmWindow *window,
			      gboolean hide_on_delete)
{
  g_return_if_fail (GM_IS_WINDOW (window));

  g_object_set (window, "hide_on_delete", hide_on_delete, NULL);
}


gboolean
gm_window_get_hide_on_delete (GmWindow *window)
{
  g_return_val_if_fail (GM_IS_WINDOW (window), FALSE);

  return window->priv->hide_on_delete;
}


void
gm_window_set_hide_on_escape (GmWindow *window,
			      gboolean hide_on_esc)
{
  g_return_if_fail (GM_IS_WINDOW (window));

  g_object_set (window, "hide_on_esc", hide_on_esc, NULL);
}


gboolean
gm_window_get_hide_on_escape (GmWindow *window)
{
  g_return_val_if_fail (GM_IS_WINDOW (window), FALSE);

  return window->priv->hide_on_esc;
}


void
gm_window_set_stay_on_top (GmWindow *window,
                           gboolean stay_on_top)
{
  g_return_if_fail (GM_IS_WINDOW (window));

  g_object_set (window, "stay_on_top", stay_on_top, NULL);
}


gboolean
gm_window_get_stay_on_top (GmWindow *window)
{
  g_return_val_if_fail (GM_IS_WINDOW (window), FALSE);

  return window->priv->stay_on_top;
}


void
gm_window_set_application (GmWindow *window,
                           GtkApplication *application)
{
  g_return_if_fail (GM_IS_WINDOW (window));

  g_object_set (window, "application", application, NULL);
}


GtkApplication *
gm_window_get_application (GmWindow *window)
{
  g_return_val_if_fail (GM_IS_WINDOW (window), NULL);

  return window->priv->application;
}

gboolean
gm_window_is_visible (GtkWidget* w)
{
  return (gtk_widget_get_visible (w) && !(gdk_window_get_state (gtk_widget_get_window (w)) & GDK_WINDOW_STATE_ICONIFIED));
}
