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
 *                         gm-entry.h  -  description
 *                         --------------------------
 *   begin                : Sat Oct 25 2014
 *   copyright            : (C) 2014 by Damien Sandras
 *   description          : Contains a GmEntry implementation handling
 *                          quick delete and basic validation depending
 *                          on the entry type.
 *
 */


#ifndef __GM_ENTRY_H__
#define __GM_ENTRY_H__

#include <gtk/gtk.h>


G_BEGIN_DECLS

// ? because sip is used by default
#define SIP_URI_SCHEME "(sip:|sips:)?"
#define BASIC_URI_PART "[A-Za-z0-9_\\-\\.]+"
#define BASIC_SIP_URI_REGEX "^" SIP_URI_SCHEME BASIC_URI_PART "@" BASIC_URI_PART "$"
#define BASIC_URI_REGEX BASIC_SIP_URI_REGEX
#define PHONE_NUMBER_REGEX "^\\+?[0-9]+$"
#define NUMBER_REGEX "^[0-9]+$"

typedef struct _GmEntry GmEntry;
typedef struct _GmEntryPrivate GmEntryPrivate;
typedef struct _GmEntryClass GmEntryClass;

/*
 * Public API
 *
 */

/** Create a new GmEntry.
 * @param The regex that will determine if the content is valid.
 *        Can be NULL.
 * @return A GmEntry.
 */
GtkWidget *gm_entry_new (const gchar *regex);


/** Check if the entry content is valid.
 * @return TRUE if the GmEntry content matches the Regex, FALSE otherwise.
 */
gboolean gm_entry_text_is_valid (GmEntry *entry);


/** Set if the entry content can be empty.
 * @param The GmEntry
 * @param The allow-empty property.
 */
void gm_entry_set_allow_empty (GmEntry *self,
                               gboolean allow_empty);


/** Check if the GmEntry content may be emty.
 * @param The GmEntry
 * @return TRUE if the GmEntry content may be empty, FALSE otherwise.
 */
gboolean gm_entry_get_allow_empty (GmEntry *self);


/** Set the activate icon.
 * The activate icon is displayed when the GmEntry content is considered as
 * valid.
 *
 * @param The GmEntry.
 * @param The activate icon name.
 */
void gm_entry_set_activate_icon (GmEntry *self,
                                 const gchar *activate_icon);


/** Return the GmEntry activate icon name.
 * @param The GmEntry
 * @return The current activate icon.
 */
const gchar *gm_entry_get_activate_icon (GmEntry *self);


/* Signals emitted by that widget :
 *
 * - "validity-changed": Emitted when the GmEntry validity changes.
 * - "activated"       : Emitted when the entry is activated and:
 *                        - its content matches the regex (is valid).
 *                        - its content is not empty (independantly of the
 *                          "allow-empty" property value. There must be
 *                          something to activate.
 *                       This signal is similar to the native "activate" signal.
 *                       However, the native signal will be emitted even if the
 *                       GmEntry content is invalid or empty.
 *
 */

/* Properties of that widget :
 *
 * - "allow-empty"  :  Defaults to TRUE.
 * - "activate-icon":  Icon that appears when the entry content is not empty
 *                     and valid. Clicking on it emits the activated signal.
 * - "regex"        :  Set the regex string to use for validity checking.
 *
 */


/* GObject thingies */
struct _GmEntry
{
  GtkEntry parent;

  GmEntryPrivate *priv;
};

struct _GmEntryClass
{
  GtkEntryClass parent;

  void (*valid) (GmEntry* self);
};

#define GM_TYPE_ENTRY (gm_entry_get_type ())

#define GM_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GM_TYPE_ENTRY, GmEntry))

#define GM_IS_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GM_TYPE_ENTRY))

#define GM_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GM_TYPE_ENTRY, GmEntryClass))

#define GM_IS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_TYPE_ENTRY))

#define GM_ENTRY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_TYPE_ENTRY, GmEntryClass))

GType gm_entry_get_type ();

G_END_DECLS

#endif
