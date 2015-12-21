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
 *                         dialpad.c  -  description
 *                         ---------------------------
 *   begin                : Thu Jan 3 2008
 *   authors              : (C) 2008 by Steve Frécinaux
 *   description          : Dial pad widget.
 */

#include "dialpad.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <string.h>

struct const_key_info
{
  const char *number;
  const unsigned codes[3];
};

/* Translators: the following strings are letters that stand on mobile phone
 * keys.
 */
static const struct const_key_info keys_info[] = {
  { "1", { GDK_KEY_1, GDK_KEY_KP_1, GDK_KEY_VoidSymbol } },
  { "2", { GDK_KEY_2, GDK_KEY_KP_2, GDK_KEY_VoidSymbol } },
  { "3", { GDK_KEY_3, GDK_KEY_KP_3, GDK_KEY_VoidSymbol } },
  { "4", { GDK_KEY_4, GDK_KEY_KP_4, GDK_KEY_VoidSymbol } },
  { "5", { GDK_KEY_5, GDK_KEY_KP_5, GDK_KEY_VoidSymbol } },
  { "6", { GDK_KEY_6, GDK_KEY_KP_6, GDK_KEY_VoidSymbol } },
  { "7", { GDK_KEY_7, GDK_KEY_KP_7, GDK_KEY_VoidSymbol } },
  { "8", { GDK_KEY_8, GDK_KEY_KP_8, GDK_KEY_VoidSymbol } },
  { "9", { GDK_KEY_9, GDK_KEY_KP_9, GDK_KEY_VoidSymbol } },
  { "*", { GDK_KEY_asterisk, GDK_KEY_KP_Multiply, GDK_KEY_VoidSymbol } },
  { "0", { GDK_KEY_0, GDK_KEY_KP_0, GDK_KEY_VoidSymbol } },
  { "#", { GDK_KEY_numbersign, GDK_KEY_VoidSymbol } }
};

struct _EkigaDialpadPrivate
{
  GtkAccelGroup *accel_group;
  GtkWidget *buttons[G_N_ELEMENTS (keys_info)];
};

enum
{
  BUTTON_CLICKED,
  LAST_SIGNAL
};

static guint ekiga_dialpad_signals[LAST_SIGNAL];

enum
{
  PROP_0,
  PROP_ACCEL_GROUP
};

G_DEFINE_TYPE (EkigaDialpad, ekiga_dialpad, GTK_TYPE_GRID);

static void
ekiga_dialpad_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  EkigaDialpad *dialpad = EKIGA_DIALPAD (object);

  switch (prop_id) {
    case PROP_ACCEL_GROUP:
      g_value_set_object (value, dialpad->priv->accel_group);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
ekiga_dialpad_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  EkigaDialpad *dialpad = EKIGA_DIALPAD (object);

  switch (prop_id) {
    case PROP_ACCEL_GROUP:
      dialpad->priv->accel_group = GTK_ACCEL_GROUP (g_value_get_object (value));
      g_object_ref (dialpad->priv->accel_group);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
on_dialpad_button_clicked (GtkButton    *button,
                           EkigaDialpad *dialpad)
{
  unsigned i;

  for (i = 0; i < G_N_ELEMENTS (keys_info); i++) {
    if (GTK_WIDGET (button) == dialpad->priv->buttons[i]) {
      g_signal_emit (dialpad, ekiga_dialpad_signals[BUTTON_CLICKED], 0,
                     keys_info[i].number);
      return;
    }
  }

  g_return_if_reached ();
}

static void
ekiga_dialpad_init (EkigaDialpad *dialpad)
{
  GtkWidget *label     = NULL;
  GtkWidget *button    = NULL;
  GtkWidget *box       = NULL;

  gchar *text          = NULL;

  unsigned i;

  dialpad->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialpad,
                                               EKIGA_TYPE_DIALPAD,
                                               EkigaDialpadPrivate);

  gtk_container_set_border_width (GTK_CONTAINER (dialpad), 0);
  gtk_grid_set_column_homogeneous (GTK_GRID (dialpad), TRUE);
  gtk_grid_set_row_homogeneous (GTK_GRID (dialpad), TRUE);
  gtk_grid_set_row_spacing (GTK_GRID (dialpad), 6);
  gtk_grid_set_column_spacing (GTK_GRID (dialpad), 6);

  // the dialpad is LTR even for RTL languages
  gtk_widget_set_direction (GTK_WIDGET (dialpad), GTK_TEXT_DIR_LTR);

  /* Create the buttons */
  for (i = 0; i < G_N_ELEMENTS (keys_info); i++) {

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_set_border_width (GTK_CONTAINER (box), 6);

    label = gtk_label_new (NULL);
    text = g_strdup_printf ("<span size=\"large\"><b>%s</b></span>",
                            _(keys_info [i].number));
    gtk_label_set_markup (GTK_LABEL (label), text);
    gtk_misc_set_padding (GTK_MISC (label), 10, 5);
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, FALSE, 0);
    gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);

    button = gtk_button_new ();
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    gtk_container_add (GTK_CONTAINER (button), box);

    dialpad->priv->buttons[i] = button;

    gtk_grid_attach (GTK_GRID (dialpad),  button,
                     i % 3, i / 3,
                     1, 1);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (on_dialpad_button_clicked), dialpad);
  }
}

static GObject *
ekiga_dialpad_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
  GObjectClass *parent_class;
  EkigaDialpad *dialpad;

  /* Invoke parent constructor. */
  parent_class = G_OBJECT_CLASS (ekiga_dialpad_parent_class);
  dialpad = EKIGA_DIALPAD (parent_class->constructor (type,
                                                      n_construct_properties,
                                                      construct_properties));

  /* The construct properties have been set -> we can use them (similar to
   * object_class->construcor which was not available in gtk 2.10) */
  if (dialpad->priv->accel_group != NULL) {
    unsigned i;
    for (i = 0; i < G_N_ELEMENTS (keys_info); i++) {
      unsigned j;
      for (j = 0; keys_info[i].codes[j] != GDK_KEY_VoidSymbol; j++) {
        gtk_widget_add_accelerator (dialpad->priv->buttons[i],
                                    "clicked",
                                    dialpad->priv->accel_group,
                                    keys_info[i].codes[j],
                                    (GdkModifierType) 0, (GtkAccelFlags) 0);
      }
    }
  }

  return G_OBJECT (dialpad);
}

static void
ekiga_dialpad_finalize (GObject *object)
{
  EkigaDialpad *dialpad = EKIGA_DIALPAD (object);

  if (dialpad->priv->accel_group != NULL)
    g_object_unref (dialpad->priv->accel_group);

  G_OBJECT_CLASS (ekiga_dialpad_parent_class)->finalize (object);
}

static void
ekiga_dialpad_class_init (EkigaDialpadClass *klass)
{
  GType the_type = G_TYPE_FROM_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = ekiga_dialpad_constructor;
  object_class->finalize = ekiga_dialpad_finalize;
  object_class->get_property = ekiga_dialpad_get_property;
  object_class->set_property = ekiga_dialpad_set_property;

  g_object_class_install_property (object_class,
                                   PROP_ACCEL_GROUP,
                                   g_param_spec_object ("accel-group",
                                                        "Accel group",
                                                        "Accel group",
                                                        GTK_TYPE_ACCEL_GROUP,
                                                        (GParamFlags) (G_PARAM_READWRITE |
                                                                       G_PARAM_CONSTRUCT_ONLY |
                                                                       G_PARAM_STATIC_STRINGS)));

  ekiga_dialpad_signals[BUTTON_CLICKED] =
            g_signal_new ("button-clicked", the_type,
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (EkigaDialpadClass, button_clicked),
                          NULL, NULL,
                          g_cclosure_marshal_VOID__STRING,
                          G_TYPE_NONE,
                          1,
                          G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);

  g_type_class_add_private (klass, sizeof (EkigaDialpadPrivate));
}

GtkWidget *ekiga_dialpad_new (GtkAccelGroup *accel_group)
{
  gpointer obj;

  if (accel_group == NULL)
    obj = g_object_new (EKIGA_TYPE_DIALPAD, NULL);
  else
    obj = g_object_new (EKIGA_TYPE_DIALPAD, "accel-group", accel_group, NULL);

  return GTK_WIDGET (obj);
}
