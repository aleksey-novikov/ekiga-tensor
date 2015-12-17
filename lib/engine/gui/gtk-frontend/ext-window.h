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

#ifndef __EXT_WINDOW_H__
#define __EXT_WINDOW_H__

#include "gmwindow.h"

#include <clutter-gtk/clutter-gtk.h>

G_BEGIN_DECLS

#define EKIGA_TYPE_EXT_WINDOW			\
	(ekiga_ext_window_get_type ())
#define EKIGA_EXT_WINDOW(obj)			\
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), EKIGA_TYPE_EXT_WINDOW, EkigaExtWindow))
#define EKIGA_EXT_WINDOW_CLASS(klass)		\
	(G_TYPE_CHECK_CLASS_CAST ((klass), EKIGA_TYPE_EXT_WINDOW, EkigaExtWindowClass))
#define EKIGA_IS_EXT_WINDOW(obj)		\
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EKIGA_TYPE_EXT_WINDOW))
#define EKIGA_IS_EXT_WINDOW_CLASS(klass)	\
	(G_TYPE_CHECK_CLASS_TYPE ((klass), EKIGA_TYPE_EXT_WINDOW))

typedef struct _EkigaExtWindowPrivate EkigaExtWindowPrivate;
typedef struct _EkigaExtWindow EkigaExtWindow;
typedef struct _EkigaExtWindowClass EkigaExtWindowClass;

struct _EkigaExtWindow {
  GmWindow parent;
  EkigaExtWindowPrivate *priv;
};

struct _EkigaExtWindowClass {
  GmWindowClass parent;
};

GType ekiga_ext_window_get_type ();
ClutterActor *ekiga_ext_window_get_stage (EkigaExtWindow *cw);
void ekiga_ext_window_set_size (EkigaExtWindow *cw, int width, int height);
void ekiga_ext_window_destroy (EkigaExtWindow *ew);

G_END_DECLS

#endif /* __EXT_WINDOW_H__ */
