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
 *                         ekiga-window.h  -  description
 *                         ------------------------------
 *   begin                : Mon Mar 26 2001
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          build the main window.
 */


#ifndef __EKIGA_WINDOW_H__
#define __EKIGA_WINDOW_H__

#include "gmwindow.h"
#include "ekiga-app.h"

G_BEGIN_DECLS

#define EKIGA_TYPE_WINDOW               (ekiga_window_get_type ())
#define EKIGA_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), EKIGA_TYPE_WINDOW, EkigaWindow))
#define EKIGA_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), EKIGA_TYPE_WINDOW, EkigaWindowClass))
#define EKIGA_IS_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EKIGA_TYPE_WINDOW))
#define EKIGA_IS_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), EKIGA_TYPE_WINDOW))

typedef struct _EkigaWindowPrivate       EkigaWindowPrivate;
typedef struct _EkigaWindow              EkigaWindow;

struct _EkigaWindow {
  GmWindow            parent;
  EkigaWindowPrivate *priv;
};

typedef GmWindowClass EkigaWindowClass;

GType        ekiga_window_get_type   ();
GtkWidget   *ekiga_window_new        (Ekiga::ServiceCore *core);

G_END_DECLS

/* DESCRIPTION  :  /
 * BEHAVIOR     :  Builds the main window and adds the popup to the image.
 * PRE          :  /
 */
GtkWidget *gm_ekiga_window_new (GmApplication *app);
#endif
