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
 *                        gm-text-anchored-tag.c  -  description
 *                         --------------------------------
 *   begin                : written in july 2008 by Julien Puydt
 *   copyright            : (C) 2008 by Julien Puydt
 *   description          : Implementation of an anchor-based text decorator
 *
 */

#include "gm-text-anchored-tag.h"

struct _GmTextAnchoredTagPrivate {

  gchar* anchor;
  GtkTextTag* tag;
  gboolean opening;
};

/* declaration of the GmTextBufferEnhancerHelperInterface code */

static void enhancer_helper_check (GmTextBufferEnhancerHelper* self,
				   const gchar* full_text,
				   gint from,
				   gint* start,
				   gint* length);

static void enhancer_helper_enhance (GmTextBufferEnhancerHelper* self,
				     GtkTextBuffer* buffer,
				     GtkTextIter* iter,
				     GSList** tags,
				     const gchar* full_text,
				     gint* start,
				     gint length);

static void enhancer_helper_interface_init (GmTextBufferEnhancerHelperInterface* iface);

G_DEFINE_TYPE_EXTENDED (GmTextAnchoredTag, gm_text_anchored_tag, G_TYPE_OBJECT, 0,
			G_IMPLEMENT_INTERFACE (GM_TYPE_TEXT_BUFFER_ENHANCER_HELPER,
					       enhancer_helper_interface_init));

/* implementation of the GmTextBufferEnhancerHelperInterface code */

static void
enhancer_helper_check (GmTextBufferEnhancerHelper* self,
		       const gchar* full_text,
		       gint from,
		       gint* start,
		       gint* length)
{
  GmTextAnchoredTagPrivate* priv = GM_TEXT_ANCHORED_TAG (self)->priv;
  char* found = NULL;

  found = g_strstr_len (full_text + from, -1, priv->anchor);

  if (found != NULL) {

    *start = found - full_text;
    *length = g_utf8_strlen (priv->anchor, -1);
  } else
    *length = 0;
}

static void
enhancer_helper_enhance (GmTextBufferEnhancerHelper* self,
			 G_GNUC_UNUSED GtkTextBuffer* buffer,
			 G_GNUC_UNUSED GtkTextIter* iter,
			 GSList** tags,
			 G_GNUC_UNUSED const gchar* full_text,
			 gint* start,
			 gint length)
{
  GmTextAnchoredTagPrivate* priv = GM_TEXT_ANCHORED_TAG (self)->priv;

  if (priv->opening)
    *tags = g_slist_prepend (*tags, priv->tag);
  else
    *tags = g_slist_remove (*tags, priv->tag);

  *start = *start + length;
}

static void
enhancer_helper_interface_init (GmTextBufferEnhancerHelperInterface* iface)
{
  iface->do_check = enhancer_helper_check;
  iface->do_enhance = enhancer_helper_enhance;
}

/* GObject boilerplate */

static void
gm_text_anchored_tag_dispose (GObject* obj)
{
  GmTextAnchoredTagPrivate* priv = GM_TEXT_ANCHORED_TAG (obj)->priv;

  if (priv->tag != NULL) {

    g_object_unref (priv->tag);
    priv->tag = NULL;
  }

  G_OBJECT_CLASS (gm_text_anchored_tag_parent_class)->dispose (obj);
}

static void
gm_text_anchored_tag_finalize (GObject* obj)
{
  GmTextAnchoredTagPrivate* priv = GM_TEXT_ANCHORED_TAG (obj)->priv;

  if (priv->anchor != NULL) {

    g_free (priv->anchor);
    priv->anchor = NULL;
  }

  G_OBJECT_CLASS (gm_text_anchored_tag_parent_class)->finalize (obj);
}

static void
gm_text_anchored_tag_class_init (GmTextAnchoredTagClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gm_text_anchored_tag_dispose;
  gobject_class->finalize = gm_text_anchored_tag_finalize;

  g_type_class_add_private (klass, sizeof (GmTextAnchoredTagPrivate));
}

static void
gm_text_anchored_tag_init (GmTextAnchoredTag* obj)
{
  obj->priv = G_TYPE_INSTANCE_GET_PRIVATE (obj, GM_TYPE_TEXT_ANCHORED_TAG,
					   GmTextAnchoredTagPrivate);

  obj->priv->anchor = NULL;
  obj->priv->tag = NULL;
  obj->priv->opening = TRUE;
}

/* Implementation of the public api */

GmTextBufferEnhancerHelper*
gm_text_anchored_tag_new (const gchar* anchor,
			  GtkTextTag* tag,
			  gboolean opening)
{
  GmTextAnchoredTag* result = NULL;

  g_return_val_if_fail (anchor != NULL && GTK_IS_TEXT_TAG (tag), NULL);

  result = (GmTextAnchoredTag*)g_object_new (GM_TYPE_TEXT_ANCHORED_TAG, NULL);

  result->priv->anchor = g_strdup (anchor);

  g_object_ref (tag);
  result->priv->tag = tag;

  result->priv->opening = opening;

  return GM_TEXT_BUFFER_ENHANCER_HELPER (result);
}
