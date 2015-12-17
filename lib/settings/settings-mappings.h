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

#ifndef SETTINGS_MAPPINGS_H_
#define SETTINGS_MAPPINGS_H_

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/* DESCRIPTION  :  Callback to be used with g_settings_bind_with_mapping
 * BEHAVIOR     :  Converts a "int" property object into a string based
 *                 GSettings value. Particularly useful with enums.
 * PRE          :  data is an array of string values whose index corresponds
 *                 to the "int" property.
 */
GVariant * string_gsettings_set_from_int (const GValue *gvalue,
                                          const GVariantType *expected_type,
                                          gpointer data);


/* DESCRIPTION  :  Callback to be used with g_settings_bind_with_mapping
 * BEHAVIOR     :  Converts a string based GSettings value into a "int"
 *                 property object. Particularly useful with enums.
 * PRE          :  data is an array of string values whose index corresponds
 *                 to the "int" property.
 */
gboolean string_gsettings_get_from_int (GValue *gvalue,
                                        GVariant *variant,
                                        gpointer data);


/* DESCRIPTION  :  Callback to be used with g_settings_bind_with_mapping
 * BEHAVIOR     :  Converts a "toggle" property object into a string based
 *                 GSettings value. Particularly useful with enums.
 * PRE          :  data is the string value to set when the object is
 *                 toggled.
 */
GVariant * string_gsettings_set_from_active (const GValue *gvalue,
                                             const GVariantType *expected_type,
                                             gpointer data);

/* DESCRIPTION  :  Callback to be used with g_settings_bind_with_mapping
 * BEHAVIOR     :  Converts a string based GSettings value into a "toggle"
 *                 property object. Particularly useful with enums.
 * PRE          :  data is the string value to set when the object is
 *                 toggled.
 */
gboolean string_gsettings_get_from_active (GValue *gvalue,
                                           GVariant *variant,
                                           gpointer data);

G_END_DECLS

#endif
