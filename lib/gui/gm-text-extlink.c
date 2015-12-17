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
 *                        gm-text-extlink.c  -  description
 *                         --------------------------------
 *   begin                : written in july 2008 by Julien Puydt
 *   copyright            : (C) 2008 by Julien Puydt
 *   description          : Implementation of a text decorator for external links
 *
 */

#include "gm-text-extlink.h"

#include <string.h>
#include <regex.h>

struct _GmTextExtlinkPrivate {

  regex_t* regex;
  GtkTextTag* tag;
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

G_DEFINE_TYPE_EXTENDED (GmTextExtlink, gm_text_extlink, G_TYPE_OBJECT, 0,
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
  GmTextExtlinkPrivate* priv = GM_TEXT_EXTLINK (self)->priv;
  gint match;
  regmatch_t regmatch;

  match = regexec (priv->regex, full_text + from, 1, &regmatch, 0);
  if (!match) {

    *start = from + regmatch.rm_so;
    *length = regmatch.rm_eo - regmatch.rm_so;
  } else {

    *length = 0;
  }
}

static void
enhancer_helper_enhance (GmTextBufferEnhancerHelper* self,
			 GtkTextBuffer* buffer,
			 GtkTextIter* iter,
			 G_GNUC_UNUSED GSList** tags,
			 const gchar* full_text,
			 gint* start,
			 gint length)
{
  GmTextExtlinkPrivate* priv = GM_TEXT_EXTLINK (self)->priv;
  gchar* lnk = NULL;

  lnk = g_malloc0 (length + 1);

  strncpy (lnk, full_text + *start, length);

  gtk_text_buffer_insert_with_tags (buffer, iter, lnk, length,
				    priv->tag, NULL);
  g_free (lnk);

  *start = *start + length;
}

static void
enhancer_helper_interface_init (GmTextBufferEnhancerHelperInterface* iface)
{
  iface->do_check = &enhancer_helper_check;
  iface->do_enhance = &enhancer_helper_enhance;
}

/* GObject boilerplate */

static void
gm_text_extlink_dispose (GObject* obj)
{
  GmTextExtlinkPrivate* priv = GM_TEXT_EXTLINK(obj)->priv;

  if (priv->tag != NULL) {

    g_object_unref (priv->tag);
    priv->tag = NULL;
  }

  G_OBJECT_CLASS (gm_text_extlink_parent_class)->dispose (obj);
}

static void
gm_text_extlink_finalize (GObject* obj)
{
  GmTextExtlinkPrivate* priv = GM_TEXT_EXTLINK(obj)->priv;

  if (priv->regex != NULL) {

    regfree (priv->regex);
    priv->regex = NULL;
  }

  G_OBJECT_CLASS (gm_text_extlink_parent_class)->finalize (obj);
}

static void
gm_text_extlink_init (GmTextExtlink* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GM_TYPE_TEXT_EXTLINK,
					    GmTextExtlinkPrivate);
  self->priv->tag = NULL;
  self->priv->regex = NULL;
}

static void
gm_text_extlink_class_init (GmTextExtlinkClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gm_text_extlink_dispose;
  gobject_class->finalize = gm_text_extlink_finalize;

  g_type_class_add_private (klass, sizeof (GmTextExtlinkPrivate));
}

/* public api */

GmTextBufferEnhancerHelper*
gm_text_extlink_new (const gchar* regex,
		     GtkTextTag* tag)
{
  GmTextExtlink* result = NULL;

  g_return_val_if_fail (regex != NULL, NULL);

  result = GM_TEXT_EXTLINK (g_object_new(GM_TYPE_TEXT_EXTLINK, NULL));

  g_object_ref (tag);
  result->priv->tag = tag;

  result->priv->regex = (regex_t*)g_malloc0 (sizeof(regex_t));
  if (regcomp (result->priv->regex, regex, REG_EXTENDED) != 0) {

    regfree (result->priv->regex);
    result->priv->regex = NULL;
    g_object_unref (result);
    result = NULL;
  }

  return GM_TEXT_BUFFER_ENHANCER_HELPER (result);
}
