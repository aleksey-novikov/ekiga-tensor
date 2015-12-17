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
 *                         settings-mappings.c  -  description
 *                         -----------------------------------
 *   begin                : Sat 14 Dec 2013
 *   copyright            : (C) 2000-2014 by Damien Sandras
 *   description          : GSettings methods and helpers.
 */

#include "settings-mappings.h"

GVariant *
string_gsettings_set_from_int (const GValue *gvalue,
                               G_GNUC_UNUSED const GVariantType *expected_type,
                               gpointer data)
{
  const gchar **values = (const gchar **) data;

  return g_variant_new_string (values [g_value_get_int (gvalue)]);
}


gboolean
string_gsettings_get_from_int (GValue *gvalue,
                               GVariant *variant,
                               gpointer data)
{
  const gchar *value = NULL;
  const gchar **values = (const gchar **) data;
  int i = 0;

  // GSettings value
  value = g_variant_get_string (variant, NULL);

  while (values [i] != NULL) {
    if (g_strcmp0 (values [i], value) == 0) {
      g_value_set_int (gvalue, i);
      return TRUE;
    }
    i++;
  }

  return FALSE;
}


GVariant *
string_gsettings_set_from_active (const GValue *gvalue,
                                  G_GNUC_UNUSED const GVariantType *expected_type,
                                  gpointer data)
{
  const gchar *value = (const gchar *) data;
  GVariant *retval = NULL;

  if (g_value_get_boolean (gvalue))
    retval = g_variant_new_string (value);

  return retval;
}


gboolean
string_gsettings_get_from_active (GValue *gvalue,
                                  GVariant *variant,
                                  gpointer data)
{
  const gchar *id = (const gchar *) (data);
  const gchar *value = NULL;

  // GSettings value
  value = g_variant_get_string (variant, NULL);

  if (g_strcmp0 (value, id) == 0)
    g_value_set_boolean (gvalue, TRUE);
  else
    g_value_set_boolean (gvalue, FALSE);

  return TRUE;
}

