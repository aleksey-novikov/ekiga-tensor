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
 *                        gm-text-buffer-enhancer.h  -  description
 *                        --------------------------------
 *   begin                : written in july 2008 by Julien Puydt
 *   copyright            : (C) 2008-2014 by Julien Puydt
 *   description          : Declaration of the interface of a text decorator
 *                          (for use with GmTextBufferEnhancer)
 *
 */

#ifndef __GM_TEXT_BUFFER_ENHANCER_H__
#define __GM_TEXT_BUFFER_ENHANCER_H__

#include "gm-text-buffer-enhancer-helper-interface.h"

G_BEGIN_DECLS

/* convenience macros */
#define GM_TYPE_TEXT_BUFFER_ENHANCER             (gm_text_buffer_enhancer_get_type())
#define GM_TEXT_BUFFER_ENHANCER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),GM_TYPE_TEXT_BUFFER_ENHANCER,GmTextBufferEnhancer))
#define GM_TEXT_BUFFER_ENHANCER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),GM_TYPE_TEXT_BUFFER_ENHANCER,GObject))
#define GM_IS_TEXT_BUFFER_ENHANCER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),GM_TYPE_TEXT_BUFFER_ENHANCER))
#define GM_IS_TEXT_BUFFER_ENHANCER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),GM_TYPE_TEXT_BUFFER_ENHANCER))
#define GM_TEXT_BUFFER_ENHANCER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),GM_TYPE_TEXT_BUFFER_ENHANCER,GmTextBufferEnhancerClass))

typedef struct _GmTextBufferEnhancer      GmTextBufferEnhancer;
typedef struct _GmTextBufferEnhancerClass GmTextBufferEnhancerClass;

struct _GmTextBufferEnhancer {
  GObject parent;
};

struct _GmTextBufferEnhancerClass {
  GObjectClass parent_class;
};

/* member functions */
GType gm_text_buffer_enhancer_get_type () G_GNUC_CONST;

GmTextBufferEnhancer* gm_text_buffer_enhancer_new (GtkTextBuffer* buffer);

void gm_text_buffer_enhancer_add_helper (GmTextBufferEnhancer* self,
					 GmTextBufferEnhancerHelper* helper);

void gm_text_buffer_enhancer_insert_text (GmTextBufferEnhancer* self,
					  GtkTextIter* iter,
					  const gchar* text,
					  gint len);

G_END_DECLS

#endif /* __GM_TEXT_BUFFER_ENHANCER_H__ */
