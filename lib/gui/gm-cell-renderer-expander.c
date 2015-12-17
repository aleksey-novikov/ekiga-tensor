/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
 * Copyright (C) 2006-2007 Imendio AB
 * Copyright (C) 2007-2011 Collabora Ltd.
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
 *                         gm-cell-renderer-expander.c -  description
 *                         ------------------------------------------
 *   authors:             : Kristian Rietveld <kris@imendio.com>
 *                          Damien Sandras <dsandras@beip.be>
 *   description          : Implementation of an Expander GtkCellRenderer
 *
 */

#include <gtk/gtk.h>

#include "gm-cell-renderer-expander.h"

typedef struct {
	guint expander_size;
	guint activatable : 1;
} GmCellRendererExpanderPriv;

enum {
	PROP_0,
	PROP_EXPANDER_SIZE,
	PROP_ACTIVATABLE
};

static void gm_cell_renderer_expander_get_property (GObject *object,
                                                    guint param_id,
                                                    GValue *value,
                                                    GParamSpec *pspec);

static void gm_cell_renderer_expander_set_property (GObject *object,
                                                    guint param_id,
                                                    const GValue *value,
                                                    GParamSpec *pspec);

static void gm_cell_renderer_expander_finalize (GObject *object);

static void gm_cell_renderer_expander_get_size (GtkCellRenderer *cell,
                                                GtkWidget *widget,
                                                const GdkRectangle *cell_area,
                                                gint *x_offset,
                                                gint *y_offset,
                                                gint *width,
                                                gint *height);

static void gm_cell_renderer_expander_render (GtkCellRenderer *cell,
                                              cairo_t *cr,
                                              GtkWidget *widget,
                                              const GdkRectangle *background_area,
                                              const GdkRectangle *cell_area,
                                              GtkCellRendererState flags);

static gboolean gm_cell_renderer_expander_activate (GtkCellRenderer *cell,
                                                    GdkEvent *event,
                                                    GtkWidget *widget,
                                                    const gchar *path,
                                                    const GdkRectangle *background_area,
                                                    const GdkRectangle *cell_area,
                                                    GtkCellRendererState flags);

G_DEFINE_TYPE (GmCellRendererExpander, gm_cell_renderer_expander, GTK_TYPE_CELL_RENDERER)

static void
gm_cell_renderer_expander_init (GmCellRendererExpander *expander)
{
  GmCellRendererExpanderPriv *priv =
    G_TYPE_INSTANCE_GET_PRIVATE (expander,
                                 GM_TYPE_CELL_RENDERER_EXPANDER,
                                 GmCellRendererExpanderPriv);

  expander->priv = priv;
  priv->expander_size = 0;
  priv->activatable = TRUE;

  g_object_set (expander,
                "xpad", 6,
                "ypad", 6,
                "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                "is-expander", TRUE,
                NULL);
}


static void
gm_cell_renderer_expander_class_init (GmCellRendererExpanderClass *klass)
{
  GObjectClass *object_class = NULL;
  GtkCellRendererClass *cell_class = NULL;

  object_class = G_OBJECT_CLASS (klass);
  cell_class = GTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize = gm_cell_renderer_expander_finalize;

  object_class->get_property = gm_cell_renderer_expander_get_property;
  object_class->set_property = gm_cell_renderer_expander_set_property;

  cell_class->get_size = gm_cell_renderer_expander_get_size;
  cell_class->render = gm_cell_renderer_expander_render;
  cell_class->activate = gm_cell_renderer_expander_activate;

  g_object_class_install_property (object_class,
                                   PROP_EXPANDER_SIZE,
                                   g_param_spec_int ("expander-size",
                                                     "Expander Size",
                                                     "The size of the expander",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_ACTIVATABLE,
                                   g_param_spec_boolean ("activatable",
                                                         "Activatable",
                                                         "The expander can be activated",
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GmCellRendererExpanderPriv));
}


static void
gm_cell_renderer_expander_get_property (GObject *object,
                                        guint param_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
  GmCellRendererExpander *expander = NULL;
  GmCellRendererExpanderPriv *priv = NULL;

  expander = GM_CELL_RENDERER_EXPANDER (object);
  priv = expander->priv;

  switch (param_id) {
  case PROP_EXPANDER_SIZE:
    g_value_set_int (value, priv->expander_size);
    break;

  case PROP_ACTIVATABLE:
    g_value_set_boolean (value, priv->activatable);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}


static void
gm_cell_renderer_expander_set_property (GObject *object,
                                        guint param_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  GmCellRendererExpander *expander = NULL;
  GmCellRendererExpanderPriv *priv = NULL;

  expander = GM_CELL_RENDERER_EXPANDER (object);
  priv = expander->priv;

  switch (param_id) {
  case PROP_EXPANDER_SIZE:
    priv->expander_size = g_value_get_int (value);
    break;

  case PROP_ACTIVATABLE:
    priv->activatable = g_value_get_boolean (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}


static void
gm_cell_renderer_expander_finalize (GObject *object)
{
  (* G_OBJECT_CLASS (gm_cell_renderer_expander_parent_class)->finalize) (object);
}


static void
gm_cell_renderer_expander_get_size (GtkCellRenderer *cell,
                                    GtkWidget *widget,
                                    const GdkRectangle *cell_area,
                                    gint *x_offset,
                                    gint *y_offset,
                                    gint *width,
                                    gint *height)
{
  GmCellRendererExpander *expander = NULL;
  GmCellRendererExpanderPriv *priv = NULL;
  gfloat xalign, yalign = 0;
  guint xpad, ypad = 0;

  expander = (GmCellRendererExpander *) cell;
  priv = expander->priv;

  if (priv->expander_size == 0)
    gtk_widget_style_get (widget, "expander_size", &priv->expander_size, NULL);

  g_object_get (cell,
                "xalign", &xalign,
                "yalign", &yalign,
                "xpad", &xpad,
                "ypad", &ypad,
                NULL);

  if (cell_area) {
    if (x_offset) {
      *x_offset = xalign * (cell_area->width - (priv->expander_size + (2 * xpad)));
      *x_offset = MAX (*x_offset, 0);
    }

    if (y_offset) {
      *y_offset = yalign * (cell_area->height - (priv->expander_size + (2 * ypad)));
      *y_offset = MAX (*y_offset, 0);
    }
  } else {
    if (x_offset)
      *x_offset = 0;

    if (y_offset)
      *y_offset = 0;
  }

  if (width)
    *width = xpad * 2 + priv->expander_size;

  if (height)
    *height = ypad * 2 + priv->expander_size;
}


static void
gm_cell_renderer_expander_render (GtkCellRenderer *cell,
				  cairo_t *cr,
				  GtkWidget *widget,
				  G_GNUC_UNUSED const GdkRectangle *background_area,
				  const GdkRectangle *cell_area,
				  GtkCellRendererState flags)
{
  GmCellRendererExpander *expander = NULL;
  GmCellRendererExpanderPriv *priv = NULL;
  gint x_offset, y_offset;
  guint xpad, ypad;
  gboolean expanded;
  GtkStyleContext *style;
  GtkStateFlags state;

  expander = (GmCellRendererExpander *) cell;
  priv = expander->priv;
  if (priv->expander_size == 0)
    gtk_widget_style_get (widget, "expander_size", &priv->expander_size, NULL);

  gm_cell_renderer_expander_get_size (cell, widget,
                                      (GdkRectangle *) cell_area,
                                      &x_offset, &y_offset,
                                      NULL, NULL);

  g_object_get (cell,
                "is-expanded", &expanded,
                "xpad", &xpad,
                "ypad", &ypad,
                NULL);

  style = gtk_widget_get_style_context (widget);

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, GTK_STYLE_CLASS_EXPANDER);

  state = gtk_cell_renderer_get_state (cell, widget, flags);

  if (!expanded)
    state |= GTK_STATE_FLAG_NORMAL;
  else
#if GTK_CHECK_VERSION(3,13,7)
    state |= GTK_STATE_FLAG_CHECKED;
#else
    state |= GTK_STATE_FLAG_ACTIVE;
#endif

  gtk_style_context_set_state (style, state);

  gtk_render_expander (style,
                       cr,
                       cell_area->x + x_offset + xpad,
                       cell_area->y + y_offset + ypad,
                       priv->expander_size,
                       priv->expander_size);

  gtk_style_context_restore (style);
}


static gboolean
gm_cell_renderer_expander_activate (GtkCellRenderer *cell,
                                    G_GNUC_UNUSED GdkEvent *event,
                                    GtkWidget *widget,
                                    const gchar *path_string,
                                    G_GNUC_UNUSED const GdkRectangle *background_area,
                                    G_GNUC_UNUSED const GdkRectangle *cell_area,
                                    G_GNUC_UNUSED GtkCellRendererState flags)
{
  GmCellRendererExpander *expander = NULL;
  GmCellRendererExpanderPriv *priv = NULL;
  GtkTreePath *path = NULL;

  expander = (GmCellRendererExpander *) cell;
  priv = expander->priv;

  if (!GTK_IS_TREE_VIEW (widget) || !priv->activatable)
    return FALSE;

  path = gtk_tree_path_new_from_string (path_string);

  if (gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget), path)) {
    gtk_tree_view_collapse_row (GTK_TREE_VIEW (widget), path);
  } else {
    gtk_tree_view_expand_row (GTK_TREE_VIEW (widget), path, FALSE);
  }

  gtk_tree_path_free (path);

  return TRUE;
}


GtkCellRenderer *
gm_cell_renderer_expander_new (void)
{
  return g_object_new (GM_TYPE_CELL_RENDERER_EXPANDER, NULL);
}
