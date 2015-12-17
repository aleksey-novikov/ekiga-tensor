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
 *                         gm-cell-renderer-bitext.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2006 by Julien Puydt, but very directly
 *                          inspired by the code of GossipCellRendererText in
 *                          imendio's gossip instant messaging client
 *   copyright            : (c) 2004 by Imendio AB
 *                          (c) 2006-2007 by Julien Puydt
 *   description          : implementation of a cell renderer showing two texts
 *
 */

#include <string.h>

#include "gm-cell-renderer-bitext.h"

struct _GmCellRendererBitextPrivate
{

  gchar *primary_text;
  gchar *secondary_text;
};

enum
{

  GM_CELL_RENDERER_BITEXT_PROP_PRIMARY_TEXT = 1,
  GM_CELL_RENDERER_BITEXT_PROP_SECONDARY_TEXT
};

G_DEFINE_TYPE (GmCellRendererBitext, gm_cell_renderer_bitext, GTK_TYPE_CELL_RENDERER_TEXT);

/* helper functions */

static void
gm_cell_renderer_bitext_update_style (GmCellRendererBitext* renderer,
				      GtkWidget* widget)
{
  GtkStyleContext *style = NULL;
  GtkStateFlags state;
  PangoAttrList *attr_list = NULL;
  PangoAttribute *attr_size = NULL;
  const PangoFontDescription* font = NULL;

  style = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);

  attr_list = pango_attr_list_new ();

  /* we want the secondary text smaller */
  gtk_style_context_get (style, state,
			 "font", &font,
			 NULL);
  attr_size = pango_attr_size_new ((int) (pango_font_description_get_size (font) * 0.8));
  attr_size->start_index = strlen (renderer->priv->primary_text) + 1;
  attr_size->end_index = (guint) - 1;
  pango_attr_list_insert (attr_list, attr_size);

  g_object_set (renderer,
		"visible", TRUE,
		"weight", PANGO_WEIGHT_NORMAL,
		"attributes", attr_list,
		NULL);
  pango_attr_list_unref (attr_list);
}

static void
gm_cell_renderer_bitext_update_text (GmCellRendererBitext* renderer)
{
  gchar *str = NULL;

  if (renderer->priv->secondary_text && g_strcmp0 (renderer->priv->secondary_text, ""))
    str = g_strdup_printf ("%s\n%s",
                           renderer->priv->primary_text,
                           renderer->priv->secondary_text);
  else
    str = g_strdup_printf ("%s",
                           renderer->priv->primary_text);

  g_object_set (renderer,
		"text", str,
		NULL);
  g_free (str);
}

/* override inherited methods */

static void
gm_cell_renderer_bitext_get_preferred_height_for_width (GtkCellRenderer* cell,
							GtkWidget* widget,
							gint width,
							gint *minimum_size,
							gint* natural_size)
{
  GtkCellRendererClass* parent
    = GTK_CELL_RENDERER_CLASS (gm_cell_renderer_bitext_parent_class);

  gm_cell_renderer_bitext_update_style (GM_CELL_RENDERER_BITEXT (cell),
					widget);

  parent->get_preferred_height_for_width (cell, widget, width, minimum_size, natural_size);
}

static void
gm_cell_renderer_bitext_render (GtkCellRenderer* cell,
				cairo_t* cr,
				GtkWidget* widget,
				const GdkRectangle* background_area,
				const GdkRectangle* cell_area,
				GtkCellRendererState flags)
{
  GtkCellRendererClass* parent
    = GTK_CELL_RENDERER_CLASS (gm_cell_renderer_bitext_parent_class);

  gm_cell_renderer_bitext_update_style (GM_CELL_RENDERER_BITEXT (cell),
					widget);

  parent->render (cell, cr, widget, background_area, cell_area, flags);
}

/* GObject code */

static void
gm_cell_renderer_bitext_finalize (GObject *obj)
{
  GmCellRendererBitext *self = NULL;

  self = (GmCellRendererBitext *)obj;

  g_free (self->priv->primary_text);
  g_free (self->priv->secondary_text);

  G_OBJECT_CLASS (gm_cell_renderer_bitext_parent_class)->finalize (obj);
}

static void
gm_cell_renderer_bitext_get_property (GObject *obj,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *spec)
{
  GmCellRendererBitext *renderer = NULL;

  renderer = (GmCellRendererBitext *)obj;

  switch (prop_id) {

  case GM_CELL_RENDERER_BITEXT_PROP_PRIMARY_TEXT:
    g_value_set_string (value, renderer->priv->primary_text);
    break;

  case GM_CELL_RENDERER_BITEXT_PROP_SECONDARY_TEXT:
    g_value_set_string (value, renderer->priv->secondary_text);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, spec);
    break;
  }
}

static void
gm_cell_renderer_bitext_set_property (GObject *obj,
				      guint prop_id,
				      const GValue *value,
				      GParamSpec *spec)
{
  GmCellRendererBitext *renderer = NULL;
  const gchar *str = NULL;
  gboolean needs_update = FALSE;

  renderer = (GmCellRendererBitext *)obj;

  switch (prop_id) {

  case GM_CELL_RENDERER_BITEXT_PROP_PRIMARY_TEXT:
    g_free (renderer->priv->primary_text);
    str = g_value_get_string (value);
    renderer->priv->primary_text = g_strdup (str ? str : "");
    (void) g_strdelimit (renderer->priv->primary_text, "\n\r\t", ' ');
    needs_update = TRUE;
    break;

  case GM_CELL_RENDERER_BITEXT_PROP_SECONDARY_TEXT:
    g_free (renderer->priv->secondary_text);
    str = g_value_get_string (value);
    renderer->priv->secondary_text = g_strdup (str ? str : "");
    (void) g_strdelimit (renderer->priv->secondary_text, "\n\r\t", ' ');
    needs_update = TRUE;
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, spec);
    break;
  }

  if (needs_update)
    gm_cell_renderer_bitext_update_text (renderer);
}

static void
gm_cell_renderer_bitext_init (GmCellRendererBitext* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
					    GM_TYPE_CELL_RENDERER_BITEXT,
					    GmCellRendererBitextPrivate);
  self->priv->primary_text = g_strdup ("");
  self->priv->secondary_text = g_strdup ("");
}

static void
gm_cell_renderer_bitext_class_init (GmCellRendererBitextClass* klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass* cell_class = GTK_CELL_RENDERER_CLASS (klass);
  GParamSpec *spec = NULL;

  g_type_class_add_private (klass, sizeof (GmCellRendererBitextPrivate));

  cell_class->get_preferred_height_for_width = gm_cell_renderer_bitext_get_preferred_height_for_width;
  cell_class->render = gm_cell_renderer_bitext_render;

  gobject_class->finalize = gm_cell_renderer_bitext_finalize;
  gobject_class->get_property = gm_cell_renderer_bitext_get_property;
  gobject_class->set_property = gm_cell_renderer_bitext_set_property;

  spec = g_param_spec_string ("primary-text",
			      "Primary text",
			      "Primary text",
			      NULL, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   GM_CELL_RENDERER_BITEXT_PROP_PRIMARY_TEXT,
				   spec);

  spec = g_param_spec_string ("secondary-text",
			      "Secondary text",
			      "Secondary text",
			      NULL, (GParamFlags) G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
				   GM_CELL_RENDERER_BITEXT_PROP_SECONDARY_TEXT,
				   spec);
}

/* public api */

GtkCellRenderer *
gm_cell_renderer_bitext_new ()
{
  return GTK_CELL_RENDERER (g_object_new (GM_TYPE_CELL_RENDERER_BITEXT, NULL));
}
