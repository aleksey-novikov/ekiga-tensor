/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
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
 *                         ekiga-app.cpp  -  description
 *                         -----------------------------
 *   begin                : written in Feb 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : main Ekiga GtkApplication
 *
 */

#ifndef __GTK_FRONTEND_H__
#define __GTK_FRONTEND_H__

#include <gtk/gtk.h>

#include "services.h"
#include "contact-core.h"
#include "presence-core.h"

G_BEGIN_DECLS

typedef struct _GmApplication GmApplication;
typedef struct _GmApplicationPrivate GmApplicationPrivate;
typedef struct _GmApplicationClass GmApplicationClass;


/* GObject thingies */
struct _GmApplication
{
  GtkApplication parent;
  GmApplicationPrivate *priv;
};

struct _GmApplicationClass
{
  GtkApplicationClass parent;
};


void ekiga_main (int argc,
                 char **argv);

GmApplication *gm_application_new ();

Ekiga::ServiceCore& gm_application_get_core (GmApplication *app);

void gm_application_show_ekiga_window (GmApplication *app);

void gm_application_hide_ekiga_window (GmApplication *app);

GtkWidget *gm_application_get_ekiga_window (GmApplication *app);

gboolean gm_application_show_help (GmApplication *app,
                                   const gchar *link_id);

void gm_application_show_about (GmApplication *app);

void gm_application_show_chat_window (GmApplication *app);

GtkWidget *gm_application_get_chat_window (GmApplication *app);

void gm_application_show_preferences_window (GmApplication *app);

void gm_application_show_addressbook_window (GmApplication *app);

void gm_application_show_accounts_window (GmApplication *app);

void gm_application_show_assistant_window (GmApplication *app);

/* GObject boilerplate */
#define GM_TYPE_APPLICATION (gm_application_get_type ())

#define GM_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GM_TYPE_APPLICATION, GmApplication))

#define GM_IS_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GM_TYPE_APPLICATION))

#define GM_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GM_TYPE_APPLICATION, GmApplicationClass))

#define GM_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_TYPE_APPLICATION))

#define GM_APPLICATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_TYPE_APPLICATION, GmApplicationClass))

GType gm_application_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
