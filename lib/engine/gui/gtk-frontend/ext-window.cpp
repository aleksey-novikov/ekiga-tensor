/* Ekiga -- A VoIP application
 * Copyright (C) 2012, Xunta de Galicia <ocfloss@xunta.es>
 *
 * Author: Victor Jaquez, Igalia S.L., AGASOL. <vjaquez@igalia.com>
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

#include "ext-window.h"
#include "ekiga-settings.h"

#ifndef WIN32
#include <gdk/gdkx.h>
#else
#include "platform/winpaths.h"
#include <gdk/gdkwin32.h>
#endif

#define STAGE_WIDTH 640
#define STAGE_HEIGHT 480

G_DEFINE_TYPE (EkigaExtWindow, ekiga_ext_window, GTK_TYPE_WINDOW);

struct _EkigaExtWindowPrivate {
#ifndef WIN32
  GC gc;
#endif
  GtkWidget *zin, *zout, *event_box;
  ClutterActor *stage;
};

static void
set_zoom_buttons_sensitive (EkigaExtWindow *ew, guint zoom)
{
  gtk_widget_set_sensitive (ew->priv->zin, zoom != 200);
  gtk_widget_set_sensitive (ew->priv->zout, zoom != 50);
}

static void
stay_on_top_changed_cb (GSettings *settings,
                        gchar *key,
                        gpointer self)

{
  bool val = false;

  g_return_if_fail (self != NULL);

  val = g_settings_get_boolean (settings, key);
  gdk_window_set_keep_above (GDK_WINDOW (gtk_widget_get_window (GTK_WIDGET (self))), val);
}

static inline void
clear_display_info (EkigaExtWindow *ew)
{
  /*
  Ekiga::DisplayInfo info;

  info.x = 0;
  info.y = 0;
  info.widget_info_set = false;
  info.zoom = 0;
  info.mode = Ekiga::VO_MODE_UNSET;
  info.config_info_set = false;
#ifdef WIN32
  info.hwnd = 0;
#else
  info.gc = 0;
  info.window = 0;
#endif
*/
  // FIXME
  //ew->priv->vocore->set_ext_display_info (info);
}

static GObject *
constructor (GType type, guint n_properties, GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (ekiga_ext_window_parent_class)->constructor (type,
                                                                        n_properties,
                                                                        params);

  return object;
}

static void
finalize (GObject* gobject)
{
  EkigaExtWindow *ew = EKIGA_EXT_WINDOW (gobject);

  delete ew->priv;
  ew->priv = NULL;

  G_OBJECT_CLASS (ekiga_ext_window_parent_class)->finalize (gobject);
}

static gboolean
focus_in_event (GtkWidget *widget, GdkEventFocus *event)
{
  if (gtk_window_get_urgency_hint (GTK_WINDOW (widget)))
    gtk_window_set_urgency_hint (GTK_WINDOW (widget), false);

  return GTK_WIDGET_CLASS (ekiga_ext_window_parent_class)->focus_in_event (widget,
                                                                           event);
}

static void
show (GtkWidget *widget)
{
  EkigaExtWindow *ew = EKIGA_EXT_WINDOW (widget);
  GdkWindow *w = gtk_widget_get_window (widget);

  if (w)
    gdk_window_set_keep_above (w, true);

  GTK_WIDGET_CLASS (ekiga_ext_window_parent_class)->show (widget);

  gtk_widget_queue_draw (widget);
}

static void
ekiga_ext_window_class_init (EkigaExtWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = constructor;
  object_class->finalize = finalize;

  widget_class->show = show;
  widget_class->draw = draw_event;
  widget_class->focus_in_event = focus_in_event;
}

static void
ekiga_ext_window_init (EkigaExtWindow *ew)
{
  ew->priv = new EkigaExtWindowPrivate;
#ifndef WIN32
  ew->priv->gc = NULL;
#endif
}

ClutterActor *
ekiga_ext_window_get_stage (EkigaExtWindow *ew)
{
  g_return_val_if_fail (EKIGA_IS_EXT_WINDOW (ew), NULL);

  return ew->priv->stage;
}

void
ekiga_ext_window_destroy (EkigaExtWindow *ew)
{
  clear_display_info (ew);

  /* dirty cheats done dirt cheap: if gtk_widget_destroy it crashes */
  gtk_widget_hide (GTK_WIDGET (ew));
}
