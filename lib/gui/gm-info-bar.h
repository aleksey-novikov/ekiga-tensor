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
 *                         gm-info-bar.h  -  description
 *                         -----------------------------
 *   begin                : Sat Nov 1 2014
 *   copyright            : (C) 2014 by Damien Sandras
 *   description          : Contains a GmInfoBary implementation handling
 *                          GtkInfoBar info and error messages.
 *
 */


#ifndef __GM_INFO_BAR_H__
#define __GM_INFO_BAR_H__

#include <gtk/gtk.h>


G_BEGIN_DECLS

typedef struct _GmInfoBar GmInfoBar;
typedef struct _GmInfoBarPrivate GmInfoBarPrivate;
typedef struct _GmInfoBarClass GmInfoBarClass;

/*
 * Public API
 *
 */

/** Create a new GmInfoBar.
 * @return A GmInfoBar.
 */
GtkWidget *gm_info_bar_new ();


/** Update the GmInfoBar content.
 * @param self:    A GmInfoBar
 * @param type:    The message type.
 * @param message: The non empty information message to push for display.
 */
void gm_info_bar_push_message (GmInfoBar *self,
                               GtkMessageType type,
                               const char *message);


/* GObject thingies */
struct _GmInfoBar
{
  GtkInfoBar parent;

  GmInfoBarPrivate *priv;
};

struct _GmInfoBarClass
{
  GtkInfoBarClass parent;

  void (*valid) (GmInfoBar* self);
};

#define GM_TYPE_INFO_BAR (gm_info_bar_get_type ())

#define GM_INFO_BAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GM_TYPE_INFO_BAR, GmInfoBar))

#define GM_IS_INFO_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GM_TYPE_INFO_BAR))

#define GM_INFO_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GM_TYPE_INFO_BAR, GmInfoBarClass))

#define GM_IS_INFO_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_TYPE_INFO_BAR))

#define GM_INFO_BAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_TYPE_INFO_BAR, GmInfoBarClass))

GType gm_info_bar_get_type ();

G_END_DECLS

#endif
