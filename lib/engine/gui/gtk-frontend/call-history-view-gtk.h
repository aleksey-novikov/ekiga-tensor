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
 *                         call-history-view-gtk.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Julien Puydt
 *   copyright            : (c) 2008 by Julien Puydt
 *   description          : declaration of a call history view widget
 *
 */

#ifndef __CALL_HISTORY_VIEW_GTK_H__
#define __CALL_HISTORY_VIEW_GTK_H__

#include <gtk/gtk.h>
#include "history-book.h"

typedef struct _CallHistoryViewGtk CallHistoryViewGtk;
typedef struct _CallHistoryViewGtkPrivate CallHistoryViewGtkPrivate;
typedef struct _CallHistoryViewGtkClass CallHistoryViewGtkClass;

/*
 * Public API
 */

/* creating the widget, connected to an History::Book object */
GtkWidget *call_history_view_gtk_new (boost::shared_ptr<History::Book> book,
                                      boost::shared_ptr<Ekiga::CallCore> call_core,
                                      boost::shared_ptr<Ekiga::ContactCore> contact_core);

void call_history_view_gtk_get_selected (CallHistoryViewGtk* self,
                                         History::Contact** contact);

/* GObject thingies */
struct _CallHistoryViewGtk
{
  GtkScrolledWindow parent;

  CallHistoryViewGtkPrivate* priv;
};

struct _CallHistoryViewGtkClass
{
  GtkScrolledWindowClass parent;

  void (*clicked) (CallHistoryViewGtk* self);
};

#define CALL_HISTORY_VIEW_GTK_TYPE (call_history_view_gtk_get_type ())

#define CALL_HISTORY_VIEW_GTK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALL_HISTORY_VIEW_GTK_TYPE, CallHistoryViewGtk))

#define IS_CALL_HISTORY_VIEW_GTK(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALL_HISTORY_VIEW_GTK_TYPE))

#define CALL_HISTORY_VIEW_GTK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CALL_HISTORY_VIEW_GTK_TYPE, CallHistoryViewGtkClass))

#define IS_CALL_HISTORY_VIEW_GTK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CALL_HISTORY_VIEW_GTK_TYPE))

#define CALL_HISTORY_VIEW_GTK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CALL_HISTORY_VIEW_GTK_TYPE, CallHistoryViewGtkClass))

GType call_history_view_gtk_get_type ();

#endif
