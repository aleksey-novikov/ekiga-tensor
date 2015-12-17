/* Ekiga -- A VoIP and Video-Conferencing application
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
 *                         pref_window.h  -  description
 *                         -----------------------------
 *   begin                : Tue Dec 26 2000
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          create the preferences window and all its callbacks
 *   Additional code      : Miguel Rodríguez Pérez  <migrax@terra.es> 
 */


#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <gtk/gtk.h>

#include "services.h"

#include "audioinput-core.h"
#include "audiooutput-core.h"

#include "gmwindow.h"

#include "ekiga-app.h"

typedef struct _PreferencesWindow PreferencesWindow;
typedef struct _PreferencesWindowPrivate PreferencesWindowPrivate;
typedef struct _PreferencesWindowClass PreferencesWindowClass;

/* GObject thingies */
struct _PreferencesWindow
{
  GmWindow parent;

  PreferencesWindowPrivate *priv;
};

struct _PreferencesWindowClass
{
  GmWindowClass parent;
};


#define PREFERENCES_WINDOW_TYPE (preferences_window_get_type ())

#define PREFERENCES_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PREFERENCES_WINDOW_TYPE, PreferencesWindow))

#define IS_PREFERENCES_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PREFERENCES_WINDOW_TYPE))

#define PREFERENCES_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PREFERENCES_WINDOW_TYPE, PreferencesWindowClass))

#define IS_PREFERENCES_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PREFERENCES_WINDOW_TYPE))

#define PREFERENCES_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PREFERENCES_WINDOW_TYPE, PreferencesWindowClass))

GType preferences_window_get_type ();


/* Public API */

/* DESCRIPTION  :  /
 * BEHAVIOR     :  It builds the preferences window.
 * PRE          :  /
 */
GtkWidget *preferences_window_new (GmApplication *app);

#endif
