/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2012 Damien Sandras <dsandras@seconix.com>
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
 *                         call_window.h  -  description
 *                         -----------------------------
 *   begin                : Wed Dec 28 2012
 *   copyright            : (C) 2000-2012 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          build the call window.
 */


#ifndef __CALL_WINDOW_H__
#define __CALL_WINDOW_H__

#include "gmwindow.h"
#include "ekiga-app.h"
#include "call-core.h"

G_BEGIN_DECLS

#define EKIGA_TYPE_CALL_WINDOW               (ekiga_call_window_get_type ())
#define EKIGA_CALL_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), EKIGA_TYPE_CALL_WINDOW, EkigaCallWindow))
#define EKIGA_CALL_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), EKIGA_TYPE_CALL_WINDOW, EkigaCallWindowClass))
#define EKIGA_IS_CALL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EKIGA_TYPE_CALL_WINDOW))
#define EKIGA_IS_CALL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), EKIGA_TYPE_CALL_WINDOW))

typedef struct _EkigaCallWindowPrivate       EkigaCallWindowPrivate;
typedef struct _EkigaCallWindow              EkigaCallWindow;

struct _EkigaCallWindow {
  GmWindow                parent;
  EkigaCallWindowPrivate *priv;
};

typedef GmWindowClass EkigaCallWindowClass;

GType        ekiga_call_window_get_type   ();


GtkWidget *call_window_new (GmApplication *app);

void call_window_present (EkigaCallWindow *call_window);

// Add a call to handle to the CallWindow.
// The call is supposed to be in "setup" mode. The reason
// is that the CallWindow should not handle calls that do
// not reach the "setup" phase because they are rejected,
// or forwarded.
void call_window_add_call (GtkWidget *call_window,
                           boost::shared_ptr<Ekiga::Call> call);

G_END_DECLS
#endif
