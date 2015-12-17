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
 *                        conversation-page.h  -  description
 *                         --------------------------------
 *   begin                : written in 2014 by Julien Puydt
 *   copyright            : (C) 2014 by Julien Puydt
 *   description          : Declaration of a page displaying a Conversation
 *
 */

#ifndef __CONVERSATION_PAGE_H__
#define __CONVERSATION_PAGE_H__

#include <gtk/gtk.h>
#include "conversation.h"

G_BEGIN_DECLS

/* public api */
typedef struct _ConversationPage ConversationPage;

GtkWidget* conversation_page_new (Ekiga::ConversationPtr conversation);

const gchar* conversation_page_get_title (ConversationPage* page);

guint conversation_page_get_unread_count (ConversationPage* page);

Ekiga::ConversationPtr conversation_page_get_conversation (ConversationPage* page);

/* GObject boilerplate */

typedef struct _ConversationPagePrivate ConversationPagePrivate;
typedef struct _ConversationPageClass ConversationPageClass;

struct _ConversationPage {
  GtkBox parent;

  ConversationPagePrivate* priv;
};

struct _ConversationPageClass {
  GtkBoxClass parent_class;

  /* signals */
  void (*updated) (ConversationPage* self); // allows tracking unread counts
};

#define TYPE_CONVERSATION_PAGE             (conversation_page_get_type())
#define CONVERSATION_PAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),TYPE_CONVERSATION_PAGE,ConversationPage))
#define CONVERSATION_PAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),TYPE_CONVERSATION_PAGE,ConversationPageClass))
#define IS_CONVERSATION_PAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),TYPE_CONVERSATION_PAGE))
#define IS_CONVERSATION_PAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),TYPE_CONVERSATION_PAGE))
#define CONVERSATION_PAGE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),TYPE_CONVERSATION_PAGE,ConversationPageClass))

GType conversation_page_get_type () G_GNUC_CONST;

G_END_DECLS

#endif /* __CONVERSATION_PAGE_H__ */
