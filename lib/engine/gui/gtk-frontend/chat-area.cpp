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
 *                        chat-area.cpp  -  description
 *                         --------------------------------
 *   begin                : written in july 2008 by Julien Puydt
 *   copyright            : (C) 2008-2014 by Julien Puydt
 *                          (C) 2008 by Jan Schampera
 *   description          : Implementation of a Chat area (view and control)
 *
 */


#include "chat-area.h"

#include "scoped-connections.h"

#include "gm-text-buffer-enhancer.h"
#include "gm-text-anchored-tag.h"
#include "gm-text-smiley.h"
#include "gm-text-extlink.h"
#include "gm-smiley-chooser-button.h"

#include "platform.h"

#include <string.h>
#include <stdarg.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

struct _ChatAreaPrivate
{
  Ekiga::ConversationPtr conversation;
  Ekiga::scoped_connections connections;
  GmTextBufferEnhancer* enhancer;

  /* we contain those, so no need to unref them */
  GtkWidget* scrolled_text_window;
  GtkWidget* text_view;
  GtkWidget* message;
};

G_DEFINE_TYPE (ChatArea, chat_area, GTK_TYPE_PANED);

/* declaration of internal api */

static void chat_area_add_notice (ChatArea* self,
				  const gchar* txt);

static void chat_area_add_message (ChatArea* self,
				   const gchar* from,
				   const gchar* txt);

/* a helper to shorten tag definitions
 * FIXME when C99 finally is supported everywhere, this
 * can be a variadic macro
 */

static void gm_chat_area_define_simple_text_tag (GtkTextBuffer*,
						 GmTextBufferEnhancer*,
						 const gchar*,
						 const gchar*,
						 const gchar*,
						 const gchar*,
						 ...);

/* declaration of callbacks */

static gboolean on_motion_notify_event (GtkWidget* widget,
					GdkEventMotion* event,
					gpointer data);

static void on_open_link_activate (GtkMenuItem* item,
				   gpointer data);

static void on_copy_link_activate (GtkMenuItem* item,
				   gpointer data);

static gboolean on_extlink_tag_event (GtkTextTag* tag,
				      GObject* textview,
				      GdkEvent* event,
				      GtkTextIter* iter,
				      gpointer data);

static void on_smiley_selected (GmSmileyChooserButton*,
				gpointer,
				gpointer);

static void on_font_changed (GtkButton* button,
                             gpointer data);

static gboolean message_activated_cb (GtkWidget *w,
                                      GdkEventKey *key,
                                      gpointer data);

static bool visit_messages (ChatArea* self,
			    const Ekiga::Message& message);

static void on_message_received (ChatArea* self,
				 const Ekiga::Message& message);

static void on_conversation_removed (ChatArea* self);

static void on_chat_area_grab_focus (GtkWidget*,
				     gpointer);

static gboolean on_chat_area_focus (GtkWidget*,
				    GtkDirectionType,
				    gpointer);

/* implementation of internal api */

static void
gm_chat_area_define_simple_text_tag (GtkTextBuffer* buffer,
				     GmTextBufferEnhancer* enhancer,
				     const gchar* tag_name,
				     const gchar* opening_tag,
				     const gchar* closing_tag,
				     const gchar* first_property_name,
				     ...)
{
  va_list args;
  GtkTextTag* tag = NULL;
  GmTextBufferEnhancerHelper* helper = NULL;
  gchar* tmp_tagstring = NULL;

  g_return_if_fail (buffer != NULL);
  g_return_if_fail (enhancer != NULL);
  g_return_if_fail (opening_tag != NULL);
  g_return_if_fail (closing_tag != NULL);

  va_start (args, first_property_name);
  tag = gtk_text_buffer_create_tag (buffer, tag_name,
				    NULL);

  if (first_property_name)
    g_object_set_valist (G_OBJECT (tag), first_property_name,
			 args);
  va_end (args);

  /* the OPENING tag */
  tmp_tagstring = g_strdup (opening_tag);
  helper = gm_text_anchored_tag_new (tmp_tagstring, tag, TRUE);
  gm_text_buffer_enhancer_add_helper (enhancer, helper);
  g_object_unref (helper);
  g_free (tmp_tagstring);
  /* the CLOSING tag */
  tmp_tagstring = g_strdup (closing_tag);
  helper = gm_text_anchored_tag_new (tmp_tagstring, tag, FALSE);
  gm_text_buffer_enhancer_add_helper (enhancer, helper);
  g_object_unref (helper);
  g_free (tmp_tagstring);

  /* 'tag' must not be unref'd because its refcount is equal to one, and
   * owned by the buffer's tag table */
}


static void
chat_area_add_notice (ChatArea* self,
		      const gchar* txt)
{
  gchar* str = NULL;
  GtkTextMark *mark = NULL;
  GtkTextBuffer* buffer = NULL;
  GtkTextIter iter;

  str = g_strdup_printf ("NOTICE: %s\n", txt);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gm_text_buffer_enhancer_insert_text (self->priv->enhancer, &iter,
				       str, -1);
  g_free (str);

  mark = gtk_text_buffer_get_mark (buffer, "current-position");
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (self->priv->text_view), mark,
                                0.0, FALSE, 0,0);
}

static void
chat_area_add_message (ChatArea* self,
		       const gchar* from,
		       const gchar* txt)
{
  gchar* str = NULL;
  GtkTextMark *mark = NULL;
  GtkTextBuffer* buffer = NULL;
  GtkTextIter iter;

  str = g_strdup_printf ("<b><i>%s %s</i></b>\n%s\n", from, _("says:"), txt);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gm_text_buffer_enhancer_insert_text (self->priv->enhancer, &iter,
				       str, -1);
  g_free (str);

  mark = gtk_text_buffer_get_mark (buffer, "current-position");
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (self->priv->text_view), mark,
                                0.0, FALSE, 0,0);
}

/* implementation of callbacks */

static gboolean
on_motion_notify_event (GtkWidget* widget,
			GdkEventMotion* event,
			G_GNUC_UNUSED gpointer data)
{
  gboolean result = FALSE;
  GdkModifierType state;
  gint xwin = 0;
  gint ywin = 0;
  gint xbuf = 0;
  gint ybuf = 0;
  GtkTextIter iter;
  GSList* tags = NULL;
  GSList* tmp_tags = NULL;
  GtkTextTag* tag = NULL;
  GdkCursor* cursor = NULL;
  GdkDeviceManager* device_manager= NULL;
  GdkDevice* pointer= NULL;

  device_manager = gdk_display_get_device_manager (gdk_window_get_display (event->window));
  pointer = gdk_device_manager_get_client_pointer (device_manager);
  gdk_window_get_device_position (event->window, pointer, &xwin, &ywin, &state);
  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (widget),
					 GTK_TEXT_WINDOW_WIDGET,
					 xwin, ywin,
					 &xbuf, &ybuf);
  gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (widget),
				      &iter, xbuf, ybuf);
  tags = gtk_text_iter_get_tags (&iter);
  for (tmp_tags = tags;
       tmp_tags != NULL;
       tmp_tags = g_slist_next (tmp_tags)) {

    tag = GTK_TEXT_TAG (tmp_tags->data);
    cursor = (GdkCursor*)g_object_get_data (G_OBJECT (tag), "cursor");
    gdk_window_set_cursor (event->window, cursor);
    if (cursor != NULL) {

      result = TRUE;
      break;
    }
  }

  g_slist_free (tags);

  return result;
}

static void
on_open_link_activate (G_GNUC_UNUSED GtkMenuItem* item,
		       gpointer data)
{
  gchar* link = NULL;

  link = (gchar*)g_object_get_data (G_OBJECT (data), "link");

  gm_platform_open_uri (link);
}

static void
on_copy_link_activate (G_GNUC_UNUSED GtkMenuItem* item,
		       gpointer data)
{
  gchar* link = NULL;
  GtkClipboard* clipboard = NULL;

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  link = (gchar*)g_object_get_data (G_OBJECT (data), "link");

  gtk_clipboard_set_text (clipboard, link, -1);
}

static gboolean
on_extlink_tag_event (GtkTextTag* tag,
		      G_GNUC_UNUSED GObject* textview,
		      GdkEvent* event,
		      GtkTextIter* iter,
		      G_GNUC_UNUSED gpointer data)
{
  gboolean result = FALSE;

  switch (event->type) {

  case GDK_BUTTON_PRESS: {

    gchar* link = NULL;
    GtkTextIter* start = gtk_text_iter_copy (iter);
    GtkTextIter* end = gtk_text_iter_copy (iter);

    gtk_text_iter_backward_to_tag_toggle (start, tag);
    gtk_text_iter_forward_to_tag_toggle (end, tag);

    link = gtk_text_buffer_get_slice (gtk_text_iter_get_buffer (iter),
				      start, end, FALSE);

    switch (event->button.button) {

    case 1:

      gm_platform_open_uri (link);
      break;

    case 3: {

      GtkWidget* menu = NULL;
      GtkWidget* menu_item = NULL;

      menu = gtk_menu_new ();
      g_object_set_data_full (G_OBJECT (menu), "link",
			      g_strdup (link), g_free);

      menu_item = gtk_menu_item_new_with_label (_("Open link in browser"));
      g_signal_connect_after (menu_item, "activate",
			      G_CALLBACK (on_open_link_activate), menu);
      gtk_widget_show (menu_item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

      menu_item = gtk_menu_item_new_with_label (_("Copy link"));
      g_signal_connect_after (menu_item, "activate",
			      G_CALLBACK (on_copy_link_activate), menu);
      gtk_widget_show (menu_item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

      gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
		      event->button.button, event->button.time);
      g_object_ref_sink (menu);
      g_object_unref (menu);
      break;
    }

    default:
      break; // nothing
    }

    g_free (link);
    gtk_text_iter_free (end);
    gtk_text_iter_free (start);
    result = TRUE;
    break;
  }

  case GDK_ENTER_NOTIFY:
  case GDK_LEAVE_NOTIFY:
  case GDK_BUTTON_RELEASE:
  case GDK_NOTHING:
  case GDK_DELETE:
  case GDK_DESTROY:
  case GDK_EXPOSE:
  case GDK_MOTION_NOTIFY:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
  case GDK_FOCUS_CHANGE:
  case GDK_CONFIGURE:
  case GDK_MAP:
  case GDK_UNMAP:
  case GDK_PROPERTY_NOTIFY:
  case GDK_SELECTION_NOTIFY:
  case GDK_SELECTION_REQUEST:
  case GDK_SELECTION_CLEAR:
  case GDK_PROXIMITY_IN:
  case GDK_PROXIMITY_OUT:
  case GDK_VISIBILITY_NOTIFY:
  case GDK_CLIENT_EVENT:
  case GDK_DROP_FINISHED:
  case GDK_DROP_START:
  case GDK_DRAG_STATUS:
  case GDK_DRAG_ENTER:
  case GDK_DRAG_MOTION:
  case GDK_DRAG_LEAVE:
  case GDK_SCROLL:
  case GDK_WINDOW_STATE:
  case GDK_SETTING:
  case GDK_OWNER_CHANGE:
  case GDK_GRAB_BROKEN:
  case GDK_DAMAGE:
  case GDK_EVENT_LAST:
  case GDK_TOUCH_BEGIN:
  case GDK_TOUCH_UPDATE:
  case GDK_TOUCH_END:
  case GDK_TOUCH_CANCEL:
  default:
    result = FALSE; // nothing
    break; // nothing
  }

  return result;
}

static void
on_smiley_selected (G_GNUC_UNUSED GmSmileyChooserButton* smiley_chooser_button,
		    gpointer characters,
		    gpointer data)
{
  const gchar* text = NULL;
  ChatArea* self = NULL;
  GtkTextBuffer *buffer = NULL;
  GtkTextIter iter;

  self = (ChatArea*)data;

  text = g_strdup ((gchar*) characters);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->message));
  gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_insert (buffer, &iter, text, -1);
  gtk_widget_grab_focus (self->priv->message);
}

static void
on_font_changed (GtkButton* button,
                 gpointer data)
{
  ChatArea* self = NULL;
  GtkTextBuffer *buffer = NULL;
  GtkTextMark* mark = NULL;
  GtkTextMark* mark_at_insert = NULL;
  GtkTextMark* mark_at_bound = NULL;
  GtkTextIter start;
  GtkTextIter end;
  GtkTextIter iter_sav_insert;
  GtkTextIter iter_sav_bound;

  const gchar* opening_tag = NULL;
  const gchar* closing_tag = NULL;
  gchar* tags = NULL;

  self = (ChatArea*)data;

  /* FIXME it's somehow dangerous to not check if we have these associations
   * set for the button object?
   * -Jan */
  opening_tag = (const gchar*) g_object_get_data (G_OBJECT (button),
						  "gm_open_tag");
  closing_tag = (const gchar*) g_object_get_data (G_OBJECT (button),
						  "gm_close_tag");
  tags = g_strdup_printf ("%s%s", opening_tag, closing_tag);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->message));
  if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end)) {

    /* "save" the current selection by inserting marks with different gravity
     * depending on the order of insert/bound in the buffer */
    gtk_text_buffer_get_iter_at_mark (buffer, &iter_sav_insert,
				      gtk_text_buffer_get_insert (buffer));
    gtk_text_buffer_get_iter_at_mark (buffer, &iter_sav_bound,
				      gtk_text_buffer_get_selection_bound (buffer));
    if (gtk_text_iter_compare (&iter_sav_bound, &iter_sav_insert) < 0) {
      /* selection is left-right */
      mark_at_bound = gtk_text_buffer_create_mark (buffer, NULL,
						   &iter_sav_bound, FALSE);
      mark_at_insert = gtk_text_buffer_create_mark (buffer, NULL,
						    &iter_sav_insert, TRUE);
      mark = mark_at_insert;
    } else {
      /* selection is right-left */
      mark_at_bound = gtk_text_buffer_create_mark (buffer, NULL,
						   &iter_sav_bound, TRUE);
      mark_at_insert = gtk_text_buffer_create_mark (buffer, NULL,
						    &iter_sav_insert, FALSE);
      mark = mark_at_bound;
    }

    /* actually insert the tags */
    gtk_text_buffer_insert (buffer, &start, opening_tag, -1);
    gtk_text_buffer_get_iter_at_mark (buffer, &end, mark);
    gtk_text_buffer_insert (buffer, &end, closing_tag, -1);

    /* restore the original selection */
    gtk_text_buffer_get_iter_at_mark (buffer, &iter_sav_bound, mark_at_bound);
    gtk_text_buffer_get_iter_at_mark (buffer, &iter_sav_insert, mark_at_insert);
    gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &iter_sav_bound);
    gtk_text_buffer_move_mark_by_name (buffer, "insert", &iter_sav_insert);
  }
  else { /* no text selected - just insert */

    gtk_text_buffer_get_iter_at_mark (buffer, &end,
				      gtk_text_buffer_get_insert (buffer));
    gtk_text_buffer_insert (buffer, &end, tags, -1);
    gtk_text_iter_backward_chars (&end, strlen (closing_tag));
    gtk_text_buffer_place_cursor (buffer, &end);
  }

  g_free (tags);

  gtk_widget_grab_focus (self->priv->message);
}

static gboolean
message_activated_cb (G_GNUC_UNUSED GtkWidget *w,
                      GdkEventKey *key,
                      gpointer data)
{
  ChatArea *self = CHAT_AREA (data);
  GtkTextIter start_iter, end_iter;
  GtkTextBuffer *buffer = NULL;
  gchar *body = NULL;

  g_return_val_if_fail (data != NULL, false);

  // if ...-shift-enter, insert newline
  // if enter, send message
  // note there are two enter: from main kbd and from keypad
  if ((key->keyval == GDK_KEY_Return || key->keyval == GDK_KEY_KP_Enter)
      && (key->state & GDK_SHIFT_MASK) == 0) {

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->message));
    gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start_iter);
    gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (buffer), &end_iter);

    /* if nothing to send - send nothing ;-) */
    if (gtk_text_iter_get_offset (&end_iter) == 0)
      return TRUE;

    body = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &start_iter, &end_iter, TRUE);

    Ekiga::Message::payload_type message;
    // FIXME: perhaps we have more than plain!
    message.insert(std::make_pair("text/plain", body));
    if (self->priv->conversation->send_message (message))
      gtk_text_buffer_delete (GTK_TEXT_BUFFER (buffer), &start_iter, &end_iter);

    return TRUE;
  }

  return FALSE;
}

static bool
visit_messages (ChatArea* self,
		const Ekiga::Message& message)
{
  on_message_received (self, message);
  return true;
}

static void
on_message_received (ChatArea* self,
		     const Ekiga::Message& message)
{
  if (message.name == "") {


    Ekiga::Message::payload_type::const_iterator iter
      = message.payload.find ("text/plain");
    if (iter != message.payload.end ())
      chat_area_add_notice (self, iter->second.c_str ());
  } else {

    Ekiga::Message::payload_type::const_iterator iter
      = message.payload.find ("text/plain");
    if (iter != message.payload.end ())
      chat_area_add_message (self, message.name.c_str (), iter->second.c_str ());
  }
}

static void
on_conversation_removed (ChatArea* self)
{
  gtk_widget_hide (self->priv->message);
  self->priv->connections.clear ();
  self->priv->conversation.reset ();
}

static void
on_chat_area_grab_focus (GtkWidget* widget,
			 G_GNUC_UNUSED gpointer data)
{
  ChatArea* self = NULL;

  self = (ChatArea*)widget;

  gtk_widget_grab_focus (self->priv->message);
  self->priv->conversation->reset_unread_messages_count ();
}

static gboolean
on_chat_area_focus (GtkWidget* widget,
		    G_GNUC_UNUSED GtkDirectionType direction,
		    G_GNUC_UNUSED gpointer data)
{
  ChatArea* self = NULL;

  self = (ChatArea*)widget;

  gtk_widget_grab_focus (self->priv->message);

  return TRUE;
}

/* GObject code */

static void
chat_area_dispose (GObject* obj)
{
  ChatArea* self = NULL;

  self = (ChatArea*)obj;

  self->priv->connections.clear ();

  if (self->priv->enhancer != NULL) {

    g_object_unref (self->priv->enhancer);
    self->priv->enhancer = NULL;
  }

  G_OBJECT_CLASS (chat_area_parent_class)->dispose (obj);
}

static void
chat_area_finalize (GObject* obj)
{
  ChatArea* self = NULL;

  self = (ChatArea*)obj;

  delete self->priv;

  G_OBJECT_CLASS (chat_area_parent_class)->finalize (obj);
}

static void
chat_area_class_init (ChatAreaClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = chat_area_dispose;
  gobject_class->finalize = chat_area_finalize;
}

static void
chat_area_init (ChatArea* self)
{
  GtkTextBuffer* buffer = NULL;
  GmTextBufferEnhancerHelper* helper = NULL;
  GtkTextTag* tag = NULL;
  GtkTextIter iter;
  GtkWidget *frame = NULL;
  GtkWidget *sep = NULL;
  GtkWidget *image = NULL;

  g_object_set (G_OBJECT (self),
		"orientation", GTK_ORIENTATION_VERTICAL,
		NULL);

  self->priv = new ChatAreaPrivate;

  /* first the area has a text view to display
     the GtkScrolledWindow is there to make
     the GtkTextView scrollable */

  self->priv->scrolled_text_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (self->priv->scrolled_text_window),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  self->priv->text_view = gtk_text_view_new ();
  g_signal_connect (self->priv->text_view, "motion-notify-event",
		    G_CALLBACK (on_motion_notify_event), NULL);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (self->priv->text_view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (self->priv->text_view),
				    FALSE);
  gtk_text_view_set_justification (GTK_TEXT_VIEW (self->priv->text_view),
				   GTK_JUSTIFY_LEFT);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->priv->text_view),
			       GTK_WRAP_WORD);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (self->priv->text_view),
				 2);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (self->priv->text_view),
				  2);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->priv->text_view));
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (self->priv->text_view), FALSE);
  gtk_text_buffer_create_mark (buffer, "current-position", &iter, FALSE);

  gtk_container_add (GTK_CONTAINER (self->priv->scrolled_text_window),
		     self->priv->text_view);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_container_add (GTK_CONTAINER (frame), self->priv->scrolled_text_window);
  gtk_paned_pack1 (GTK_PANED (self), frame, TRUE, TRUE);
  gtk_widget_show_all (frame);

  /* then we want to enhance this display */

  self->priv->enhancer = gm_text_buffer_enhancer_new (buffer);

  tag = gtk_text_buffer_create_tag (buffer, "external-link",
				    "foreground", "blue",
				    "underline", PANGO_UNDERLINE_SINGLE,
				    NULL);
  g_signal_connect (tag, "event",
		    G_CALLBACK (on_extlink_tag_event), NULL);
  {
    GdkCursor* cursor = gdk_cursor_new (GDK_HAND2);
    g_object_set_data_full (G_OBJECT (tag), "cursor", cursor,
			    (GDestroyNotify) g_object_unref);
  }
  helper = gm_text_extlink_new ("\\<(http[s]?|[s]?ftp)://[^[:blank:]]+\\>", tag);
  gm_text_buffer_enhancer_add_helper (self->priv->enhancer, helper);
  g_object_unref (helper);

  helper = gm_text_smiley_new ();
  gm_text_buffer_enhancer_add_helper (self->priv->enhancer, helper);
  g_object_unref (helper);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
				       "bold", "<b>", "</b>",
				       "weight", PANGO_WEIGHT_BOLD,
				       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
				       "italic", "<i>", "</i>",
				       "style", PANGO_STYLE_ITALIC,
				       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
				       "underline", "<u>", "</u>",
				       "underline", PANGO_UNDERLINE_SINGLE,
				       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
				       "col_black", "<color=black>", "</color>",
				       "foreground", "#000000",
				       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_white", "<color=white>", "</color>",
                                       "foreground", "#FFFFFF",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_red", "<color=red>", "</color>",
                                       "foreground", "#FF0000",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_cyan", "<color=cyan>", "</color>",
                                       "foreground", "#00FFFF",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_green", "<color=green>", "</color>",
                                       "foreground", "#00FF00",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_magenta", "<color=magenta>", "</color>",
                                       "foreground", "#FF00FF",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_blue", "<color=blue>", "</color>",
                                       "foreground", "#0000FF",
                                       NULL);

  gm_chat_area_define_simple_text_tag (buffer, self->priv->enhancer,
                                       "col_yellow", "<color=yellow>", "</color>",
                                       "foreground", "#FFFF00",
                                       NULL);


  /* and finally the chat area has a nice entry system */
  GtkWidget* vbox = NULL;
  GtkWidget* bbox = NULL;
  GtkWidget* button = NULL;
  GtkWidget* smiley_button;
  GtkWidget* smiley_chooser_button = NULL;

  frame = gtk_frame_new (NULL);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_paned_pack2 (GTK_PANED (self), frame, TRUE, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show_all (frame);

  bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  /* FIXME gtk_box_set_spacing() seems to be neccesary, though we
     define a padding with the pack() methods */
  /* FIXME the box doesn't do the 2px space at the left and right edges! */
  gtk_box_set_spacing (GTK_BOX (bbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), bbox,
		      FALSE, FALSE, 2);
  gtk_widget_show (bbox);

  smiley_button = gtk_image_new_from_icon_name ("face-smile", GTK_ICON_SIZE_BUTTON);

  smiley_chooser_button = gm_smiley_chooser_button_new ();
  gtk_button_set_label (GTK_BUTTON(smiley_chooser_button), _("_Smile..."));
  gtk_button_set_image (GTK_BUTTON(smiley_chooser_button), smiley_button);
  gtk_button_set_relief (GTK_BUTTON(smiley_chooser_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (smiley_chooser_button), FALSE);
  g_signal_connect (smiley_chooser_button, "smiley_selected",
		    G_CALLBACK (on_smiley_selected), self);
  gtk_box_pack_start (GTK_BOX (bbox), smiley_chooser_button,
		      FALSE, TRUE, 2);

  /* the BOLD button */
  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("format-text-bold-symbolic",
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_object_set_data_full (G_OBJECT (button), "gm_open_tag",
			  (gpointer) "<b>", NULL);
  g_object_set_data_full (G_OBJECT (button), "gm_close_tag",
			  (gpointer) "</b>", NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (on_font_changed), self);
  gtk_box_pack_start (GTK_BOX (bbox), button,
		      FALSE, TRUE, 2);
  gtk_widget_show (button);

  /* the ITALIC button */
  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("format-text-italic-symbolic",
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_object_set_data_full (G_OBJECT (button), "gm_open_tag",
			  (gpointer) "<i>", NULL);
  g_object_set_data_full (G_OBJECT (button), "gm_close_tag",
			  (gpointer) "</i>", NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (on_font_changed), self);
  gtk_box_pack_start (GTK_BOX (bbox), button,
		      FALSE, TRUE, 2);
  gtk_widget_show (button);

  /* the UNDERLINE button */
  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("format-text-underline-symbolic",
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_object_set_data_full (G_OBJECT (button), "gm_open_tag",
			  (gpointer) "<u>", NULL);
  g_object_set_data_full (G_OBJECT (button), "gm_close_tag",
			  (gpointer) "</u>", NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (on_font_changed), self);
  gtk_box_pack_start (GTK_BOX (bbox), button,
		      FALSE, TRUE, 2);
  gtk_widget_show (button);

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);

  self->priv->message = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->priv->message),
                               GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_cursor_visible  (GTK_TEXT_VIEW (self->priv->message), true);
  g_signal_connect (self->priv->message, "key-press-event",
                    G_CALLBACK (message_activated_cb), self);
  gtk_box_pack_start (GTK_BOX (vbox), self->priv->message,
		      TRUE, TRUE, 2);

  gtk_widget_set_size_request (GTK_WIDGET (self->priv->message), 155, -1);
  gtk_widget_show_all (vbox);

  g_signal_connect (self, "focus",
		    G_CALLBACK (on_chat_area_focus), NULL);
  g_signal_connect (self, "grab-focus",
		    G_CALLBACK (on_chat_area_grab_focus), self->priv->message);
  gtk_widget_grab_focus (self->priv->message);
}

/* public api */

GtkWidget*
chat_area_new (Ekiga::ConversationPtr conversation)
{
  ChatArea* self = NULL;

  self = (ChatArea*)g_object_new (TYPE_CHAT_AREA, NULL);
  self->priv->conversation = conversation;
  self->priv->connections.add (conversation->removed.connect (boost::bind (&on_conversation_removed, self)));
  self->priv->connections.add (conversation->message_received.connect (boost::bind (&on_message_received, self, _1)));
  conversation->visit_messages (boost::bind (&visit_messages, self, _1));

  return (GtkWidget*)self;
}

const std::string
chat_area_get_title (ChatArea* area)
{
  return area->priv->conversation->get_title ();
}
