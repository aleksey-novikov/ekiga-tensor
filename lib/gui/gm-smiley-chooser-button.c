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
 *                         gm-smiley-chooser-button.c  -  description
 *                         ------------------------------------------
 *   begin                : August 2008 by Jan Schampera
 *   copyright            : (C) 2008 by Jan Schampera
 *   description          : Implementation of a popup window to choose a smiley
 *
 */

/*
 * Thanks to (alphabetical):
 * Alphonso, Fabrice
 * Defais, Yannick
 * Puydt, Julien
 * Sandras, Damien
 */

/* #include "config.h" */
#define _(x) x

#include "gm-smiley-chooser-button.h"
#include "gm-smileys.h"

#include <math.h>

enum {
  SIG_SMILEY_SELECTED,
  SIG_LAST
};

/* these are used for readability */
enum {
  HANDLER_CONFIGURE,
  HANDLER_SCREEN_CHANGED,
  HANDLER_HIDE,
  HANDLER_DELETE_EVENT,
  HANDLER_NUM
};

static guint signals[SIG_LAST] = { 0 };

struct _GmSmileyChooserButtonPrivate {

  gulong toplevel_window_handler[HANDLER_NUM];
  gchar** smiley_set;

  gboolean popped_up;

  GtkWidget* popup_window;
  GtkWidget* frame;
  GtkWidget* grid;
};

G_DEFINE_TYPE (GmSmileyChooserButton, gm_smiley_chooser_button, GTK_TYPE_TOGGLE_BUTTON);

/* Prototyping of the internal API */

/* GObjecting */
static void gm_smiley_chooser_button_class_init (GmSmileyChooserButtonClass* klass);

static void gm_smiley_chooser_button_init (GmSmileyChooserButton* self);

static void gm_smiley_chooser_button_dispose (GObject*);

static void gm_smiley_chooser_button_finalize (GObject*);

/* internal callbacks */

static void on_smiley_chooser_button_toggled (GtkToggleButton*,
					      gpointer);

static void on_button_hierarchy_changed (GtkWidget*,
					 GtkWidget*,
					 gpointer);

static gboolean on_toplevel_configure_event (GtkWidget*,
					     GdkEventConfigure*,
					     gpointer);

static void on_toplevel_screen_changed (GtkWidget*,
					GdkScreen*,
					gpointer);

static void on_toplevel_hide (GtkWidget*,
			      gpointer);

static gboolean on_toplevel_delete_event (GtkWidget*,
					  GdkEvent*,
					  gpointer);

static void on_smiley_image_clicked (GtkButton*,
				     gpointer);

static gboolean on_popup_button_press_event (GtkWidget*,
					     GdkEventButton*,
					     gpointer);


/* real internal API */
static void gm_smiley_chooser_button_reposition_popup (GmSmileyChooserButton*,
						       gint,
						       gint);

static void gm_smiley_chooser_button_set_smiley_set (GmSmileyChooserButton*,
						     const gchar**);

static void gm_smiley_chooser_button_reload_smiley_set (GmSmileyChooserButton*);

static void gm_smiley_chooser_button_destroy_view (GmSmileyChooserButton*);

static void gm_smiley_chooser_build_view (GmSmileyChooserButton*);

static void gm_smiley_chooser_button_popup (GmSmileyChooserButton*);

static void gm_smiley_chooser_button_popdown (GmSmileyChooserButton*);


/* Implementation of the internal API */

static void
gm_smiley_chooser_button_class_init (GmSmileyChooserButtonClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gm_smiley_chooser_button_dispose;
  gobject_class->finalize = gm_smiley_chooser_button_finalize;

  /* the "smiley_selected" signal */
  signals[SIG_SMILEY_SELECTED] =
    g_signal_new ("smiley_selected",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GmSmileyChooserButtonClass, smiley_selected),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  g_type_class_add_private (klass, sizeof (GmSmileyChooserButtonPrivate));
}

static void
gm_smiley_chooser_button_init (GmSmileyChooserButton* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
					    GM_TYPE_SMILEY_CHOOSER_BUTTON,
					    GmSmileyChooserButtonPrivate);

  self->priv->popped_up = FALSE;
  self->priv->toplevel_window_handler[HANDLER_CONFIGURE] = 0;
  self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED] = 0;
  self->priv->toplevel_window_handler[HANDLER_HIDE] = 0;
  self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT] = 0;
  self->priv->smiley_set = NULL;
  self->priv->popup_window = NULL;
  self->priv->frame = NULL;
  self->priv->grid = NULL;

  g_signal_connect (self, "toggled",
		    G_CALLBACK (on_smiley_chooser_button_toggled), NULL);
}

static void
gm_smiley_chooser_button_dispose (GObject* object)
{
  GmSmileyChooserButton* self = NULL;

  self = GM_SMILEY_CHOOSER_BUTTON (object);

  if (self->priv->popped_up)
    gm_smiley_chooser_button_popdown (self);

  gm_smiley_chooser_button_destroy_view (self);
}

static void
gm_smiley_chooser_button_finalize (GObject* object)
{
  GmSmileyChooserButton* self = NULL;

  self = GM_SMILEY_CHOOSER_BUTTON (object);

  if (self->priv->smiley_set) {

    g_strfreev (self->priv->smiley_set);
    self->priv->smiley_set = NULL;
  }
}

/* internal callbacks */

static void
on_smiley_chooser_button_toggled (GtkToggleButton* toggle_button,
				  G_GNUC_UNUSED gpointer data)
{
  GmSmileyChooserButton* self = NULL;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (toggle_button));

  self = GM_SMILEY_CHOOSER_BUTTON (toggle_button);

  if (gtk_toggle_button_get_active (toggle_button))
    {
      /* popup */
      gm_smiley_chooser_button_popup (self);
    }
  else
    {
      /* popdown */
      gm_smiley_chooser_button_popdown (self);
    }
}

static void
on_button_hierarchy_changed (GtkWidget* widget,
			     GtkWidget* old_toplevel,
			     gpointer data)
{
  GmSmileyChooserButton* self = NULL;
  GtkWidget* new_toplevel = NULL;

  g_return_if_fail (data != NULL && GM_IS_SMILEY_CHOOSER_BUTTON (data));

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  if (old_toplevel &&
      self->priv->toplevel_window_handler[HANDLER_CONFIGURE]) {

      g_signal_handler_disconnect (G_OBJECT (old_toplevel),
				   self->priv->toplevel_window_handler[HANDLER_CONFIGURE]);
      self->priv->toplevel_window_handler[HANDLER_CONFIGURE] = 0;
    }

  if (old_toplevel &&
      self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED]) {

      g_signal_handler_disconnect (G_OBJECT (old_toplevel),
				   self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED]);
      self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED] = 0;
    }

  if (old_toplevel &&
      self->priv->toplevel_window_handler[HANDLER_HIDE]) {

      g_signal_handler_disconnect (G_OBJECT (old_toplevel),
				   self->priv->toplevel_window_handler[HANDLER_HIDE]);
      self->priv->toplevel_window_handler[HANDLER_HIDE] = 0;
    }

  if (old_toplevel &&
      self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT]) {

      g_signal_handler_disconnect (G_OBJECT (old_toplevel),
				   self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT]);
      self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT] = 0;
    }

  if (old_toplevel) {

      gtk_window_set_transient_for (GTK_WINDOW (self->priv->popup_window), NULL);
      g_object_unref (G_OBJECT (old_toplevel));
    }

  new_toplevel = gtk_widget_get_toplevel (widget);

  if (GTK_IS_WINDOW (new_toplevel)) {

      g_object_ref_sink (G_OBJECT (new_toplevel));
      self->priv->toplevel_window_handler[HANDLER_CONFIGURE] =
	g_signal_connect (new_toplevel, "configure-event",
			  G_CALLBACK (on_toplevel_configure_event), self);
      self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED] =
	g_signal_connect (new_toplevel, "screen-changed",
			  G_CALLBACK (on_toplevel_screen_changed), self);
      self->priv->toplevel_window_handler[HANDLER_HIDE] =
	g_signal_connect (new_toplevel, "hide",
			  G_CALLBACK (on_toplevel_hide), self);
      self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT] =
	g_signal_connect (new_toplevel, "delete-event",
			  G_CALLBACK (on_toplevel_delete_event), self);
      gtk_window_set_transient_for (GTK_WINDOW (self->priv->popup_window),
				    GTK_WINDOW (new_toplevel));
    }
}


static gboolean
on_toplevel_configure_event (G_GNUC_UNUSED GtkWidget* widget,
			     GdkEventConfigure* event,
			     gpointer data)
{
  GmSmileyChooserButton* self = NULL;

  g_return_val_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data), FALSE);

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  /* reposition, if we show our popup */
  if (self->priv->popped_up &&
      event->type == GDK_CONFIGURE)
    gm_smiley_chooser_button_reposition_popup (self,
					       event->x, event->y);

  return FALSE;
}

static void
on_toplevel_screen_changed (G_GNUC_UNUSED GtkWidget* widget,
			    G_GNUC_UNUSED GdkScreen* old_screen,
			    gpointer data)
{
  GmSmileyChooserButton* self = NULL;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data));

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  if (self->priv->popped_up)
    {
      gm_smiley_chooser_button_popdown (self);
      gm_smiley_chooser_button_popup (self);
    }
}

static void
on_toplevel_hide (G_GNUC_UNUSED GtkWidget* window,
		  gpointer data)
{
  GmSmileyChooserButton* self = NULL;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data));

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  gm_smiley_chooser_button_popdown (self);
}

static gboolean
on_toplevel_delete_event (G_GNUC_UNUSED GtkWidget* window,
			  G_GNUC_UNUSED GdkEvent* event,
			  gpointer data)
{
  GmSmileyChooserButton* self = NULL;

  g_return_val_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data), FALSE);

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  gm_smiley_chooser_button_popdown (self);
  /* FIXME - delete us! */

  return FALSE;
}

static void
on_smiley_image_clicked (GtkButton* button,
                                     gpointer data)
{
  GmSmileyChooserButton* self = NULL;
  gchar* smiley_characters = NULL;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data));

  self = GM_SMILEY_CHOOSER_BUTTON (data);

  smiley_characters = (gchar*) g_object_get_data (G_OBJECT (button),
						  "smiley_characters");

  gm_smiley_chooser_button_popdown (self);

  g_signal_emit_by_name (self, "smiley_selected",
			 g_strdup (smiley_characters));
}

static gboolean
on_popup_button_press_event (G_GNUC_UNUSED GtkWidget* widget,
			     GdkEventButton* event,
			     gpointer data)
{
  g_return_val_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (data), FALSE);

  if (event->type == GDK_BUTTON_PRESS)
    gm_smiley_chooser_button_popdown (GM_SMILEY_CHOOSER_BUTTON (data));

  return FALSE;
}


/* real internal API */

static void
gm_smiley_chooser_button_reposition_popup (GmSmileyChooserButton* self,
					   gint ref_x,
					   gint ref_y)
{
  gint final_x = 0;
  gint final_y = 0;
  GtkAllocation allocation;
  GtkAllocation popup_allocation;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self));

  /* return if nothing useful to do */
  if (!self->priv->popped_up)
    return;
  if (!self->priv->popup_window)
    return;

  gtk_widget_get_allocation (GTK_WIDGET (self), &allocation);
  gtk_widget_get_allocation (GTK_WIDGET (self->priv->popup_window), &popup_allocation);

  /* calculate the new position out of the button's
   * and the smiley's sizes */
  final_x = ref_x + allocation.x;
  final_y = ref_y + allocation.y - popup_allocation.height;

  /* move its ass */
  gtk_window_move (GTK_WINDOW (self->priv->popup_window),
                   final_x, final_y);
}

static void
gm_smiley_chooser_button_set_smiley_set (GmSmileyChooserButton* self,
					 const gchar** smiley_set)
{
  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self) && smiley_set != NULL);

  if (self->priv->smiley_set)
    g_strfreev (self->priv->smiley_set);

  self->priv->smiley_set = g_strdupv ((gchar**) smiley_set);
}


static void
gm_smiley_chooser_button_reload_smiley_set (GmSmileyChooserButton* self)
{
  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self));

  /* nothing but kill and rebuild our view */
  gm_smiley_chooser_button_destroy_view (self);
  gm_smiley_chooser_build_view (self);
}

static void
gm_smiley_chooser_button_popup (GmSmileyChooserButton* self)
{
  gint my_x = 0;
  gint my_y = 0;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self));

  /* get my position data and position the popup */
  (void) gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (self)),
                                &my_x, &my_y);

  gtk_window_move (GTK_WINDOW (self->priv->popup_window),
		   my_x, my_y);

  gtk_widget_show_all (self->priv->popup_window);
  gtk_window_present (GTK_WINDOW (self->priv->popup_window));

  self->priv->popped_up = TRUE;

  gm_smiley_chooser_button_reposition_popup (self, my_x, my_y);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), TRUE);
}

static void
gm_smiley_chooser_button_popdown (GmSmileyChooserButton* self)
{
  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self));

  gtk_widget_hide (GTK_WIDGET (self->priv->popup_window));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self), FALSE);

  self->priv->popped_up = FALSE;
}

static void
gm_smiley_chooser_button_destroy_view (GmSmileyChooserButton* self)
{
  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self));

  if (self->priv->popped_up)
    gm_smiley_chooser_button_popdown (self);

  if (self->priv->grid)
    {
      g_object_unref (G_OBJECT (self->priv->grid));
      gtk_widget_destroy (self->priv->grid);
      self->priv->grid = NULL;
    }

  if (self->priv->frame)
    {
      g_object_unref (G_OBJECT (self->priv->frame));
      gtk_widget_destroy (self->priv->frame);
      self->priv->frame = NULL;
    }

  if (self->priv->popup_window)
    {
      g_object_unref (G_OBJECT (self->priv->popup_window));
      gtk_widget_destroy (self->priv->popup_window);
      self->priv->popup_window = NULL;
    }
}


static void
gm_smiley_chooser_build_view (GmSmileyChooserButton* self)
{
  GtkWidget* button = NULL;
  GtkWidget* image = NULL;
  GdkPixbuf* pixbuf = NULL;

  const gfloat golden_ratio = 1.6180339887;
  guint smiley = 0;
  guint num_smileys = 0;

  guint grid_width = 0;
  guint grid_height = 0;
  guint iter_x = 0;
  guint iter_y = 0;

  g_return_if_fail (GM_IS_SMILEY_CHOOSER_BUTTON (self)
		    && self->priv->smiley_set != NULL
		    && self->priv->smiley_set[0] != NULL);

  /* the popup window */
  self->priv->popup_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_ref_sink (G_OBJECT (self->priv->popup_window));
  g_signal_connect (self->priv->popup_window, "button-press-event",
		    G_CALLBACK (on_popup_button_press_event), self);
//  gtk_window_set_title (GTK_WINDOW (self->priv->popup_window), _("Smile!"));
  gtk_window_set_type_hint (GTK_WINDOW (self->priv->popup_window),
			    GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (self->priv->popup_window), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (self->priv->popup_window), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (self->priv->popup_window), FALSE);

  /* the frame */
//  self->priv->frame = gtk_frame_new (_("Smile!"));
  self->priv->frame = gtk_frame_new (NULL);
  g_object_ref_sink (G_OBJECT (self->priv->frame));

  /* compute the number of available smileys */
  for (smiley = 0;
       self->priv->smiley_set[smiley] != NULL;
       smiley = smiley + 2) {}
  num_smileys = smiley / 2;

  /* calculate the dimensions out of the number of smileys */
  /* FIXME calc the height/width of the grid */
  if (num_smileys == 1)
    /* 1 smiley - special case, or fix the calculation below
     * if possible */
    grid_width = grid_height = 1;
  else {
    grid_height = round (sqrt (num_smileys / golden_ratio));
    grid_width = ceil (sqrt (num_smileys / golden_ratio) * golden_ratio);
    /* the following is needed to catch bordercases where the
     * rounding/ceiling fails to quantize to the wanted value.
     * No matter how you do the columns (round/ceil), you sometimes
     * have either one row too much or too less - that's why */
    if ( (grid_height * grid_width) < num_smileys)
      {
        if ( ((grid_height + 1) * grid_width) > (grid_height * (grid_width + 1)))
          grid_width++;
        else
          grid_height++;
      }
  }

  /* the grid */
  self->priv->grid = gtk_grid_new ();
  g_object_ref (G_OBJECT (self->priv->grid));

  /* populate the table with the smiley buttons */
  smiley = 0;
  for (iter_y = 0;
       iter_y < grid_height && (smiley / 2) < num_smileys;
       iter_y++) {
    for (iter_x = 0;
         iter_x < grid_width && (smiley / 2) < num_smileys;
         iter_x++) {
      button = gtk_button_new ();
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                         self->priv->smiley_set[smiley + 1], 16,
                                         (GtkIconLookupFlags)0, NULL);
      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_container_add (GTK_CONTAINER (button), image);
      g_object_set_data_full (G_OBJECT (button),
                              "smiley_characters",
                              (gpointer) g_strdup (self->priv->smiley_set[smiley]),
                              g_free);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (on_smiley_image_clicked), self);

      gtk_grid_attach (GTK_GRID (self->priv->grid),
		       button,
		       iter_x, iter_y,
		       1, 1);
      smiley += 2;
    } /* for (iter_x) */
  } /* for (iter_y) */

  /* glue it all together */
  gtk_container_add (GTK_CONTAINER (self->priv->popup_window), self->priv->frame);
  gtk_container_add (GTK_CONTAINER (self->priv->frame), self->priv->grid);
  gtk_widget_show (self->priv->frame);
  gtk_widget_show_all (self->priv->grid);
}


/* Implementation of the public API */

GtkWidget*
gm_smiley_chooser_button_new (void)
{
  GmSmileyChooserButton* self = NULL;
  GtkWidget* widget = NULL;

  self =
    (GmSmileyChooserButton*) g_object_new (GM_TYPE_SMILEY_CHOOSER_BUTTON, NULL);
  gtk_button_set_use_underline (GTK_BUTTON (self), TRUE);

  /* if already possible (unlikely), initially set the toplevel reference */
  widget = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (widget &&
      gtk_widget_is_toplevel (widget) &&
      GTK_IS_WINDOW (widget))
    {
      g_object_ref_sink (G_OBJECT (widget));
      self->priv->toplevel_window_handler[HANDLER_CONFIGURE] =
	g_signal_connect (widget, "configure-event",
			  G_CALLBACK (on_toplevel_configure_event), self);
      self->priv->toplevel_window_handler[HANDLER_SCREEN_CHANGED] =
        g_signal_connect (widget, "screen-changed",
                          G_CALLBACK (on_toplevel_screen_changed), self);
      self->priv->toplevel_window_handler[HANDLER_HIDE] =
        g_signal_connect (widget, "hide",
                          G_CALLBACK (on_toplevel_hide), self);
      self->priv->toplevel_window_handler[HANDLER_DELETE_EVENT] =
        g_signal_connect (widget, "delete-event",
                          G_CALLBACK (on_toplevel_delete_event), self);
    }

  g_signal_connect (self, "hierarchy-changed",
    		    G_CALLBACK (on_button_hierarchy_changed), self);

  gm_smiley_chooser_button_set_smiley_set (self, gm_get_smileys());
  gm_smiley_chooser_button_reload_smiley_set (self);

  return GTK_WIDGET (self);
}

