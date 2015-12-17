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
 *                          assistant-window.h  -  description
 *                          ---------------------------
 *   begin                : Mon May 1 2002
 *   copyright            : (C) 2000-2008 by Damien Sandras
 *                          (C) 2008 by Steve Frécinaux
 *   description          : This file contains all the functions needed to
 *                          build the assistant.
 */


#ifndef __ASSISTANT_WINDOW_H__
#define __ASSISTANT_WINDOW_H__

#include <gtk/gtk.h>
#include "ekiga-app.h"

G_BEGIN_DECLS


#define ASSISTANT_WINDOW_TYPE (assistant_window_get_type ())
#define ASSISTANT_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), ASSISTANT_WINDOW_TYPE, AssistantWindow))
#define ASSISTANT__WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), ASSISTANT_WINDOW_TYPE, AssistantWindowClass))
#define IS_ASSISTANT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ASSISTANT_WINDOW_TYPE))
#define IS_ASSISTANT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), ASSISTANT_WINDOW_TYPE))

typedef struct _AssistantWindowPrivate AssistantWindowPrivate;
typedef struct _AssistantWindow AssistantWindow;

struct _AssistantWindow {
  GtkAssistant            parent;
  AssistantWindowPrivate* priv;
};

typedef GtkAssistantClass AssistantWindowClass;

GType assistant_window_get_type   ();
GtkWidget* assistant_window_new (GmApplication *app);

G_END_DECLS

#endif /* __ASSISTANT_WINDOW_H__ */
