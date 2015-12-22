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
 *                         ekiga_window.cpp  -  description
 *                         --------------------------------
 *   begin                : Mon Mar 26 2001
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          build the main window.
 */

#include "config.h"

#include "settings-mappings.h"

#include "ekiga-window.h"

#include "dialpad.h"
#include "gm-entry.h"

#include "gm-info-bar.h"
#include "scoped-connections.h"

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>

#include "engine.h"

#include "account-core.h"
#include "audioinput-core.h"
#include "audiooutput-core.h"

#include "ekiga-app.h"
#include "roster-view-gtk.h"
#include "call-history-view-gtk.h"
#include "history-source.h"

#include "opal-bank.h"

#include "uri.h"

enum CallingState {Standby, Calling, Connected, Called};

G_DEFINE_TYPE (EkigaWindow, ekiga_window, GM_TYPE_WINDOW);


struct _EkigaWindowPrivate
{
  GmApplication *app;

  boost::shared_ptr<Ekiga::AccountCore> account_core;
  boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core;
  boost::shared_ptr<Ekiga::CallCore> call_core;
  boost::shared_ptr<Ekiga::ContactCore> contact_core;
  boost::shared_ptr<Ekiga::PresenceCore> presence_core;
  boost::weak_ptr<Opal::Bank> bank;
  boost::weak_ptr<History::Source> history_source;

  GtkWidget *call_window;

  GtkAccelGroup *accel;
  GtkWidget *main_stack;
  GtkBuilder *builder;

  /* Dialpad uri toolbar */
  GtkWidget *entry;
  GtkWidget *call_button;

  /* Actions toolbar */
  GtkWidget *actions_toolbar;
  GtkWidget *menu_button;

  GtkWidget* call_history_view;

  GtkWidget *info_bar;

  /* Calls */
  boost::shared_ptr<Ekiga::Call> current_call;
  CallingState calling_state;

  Ekiga::scoped_connections connections;

  std::string call_uri;

  /* GSettings */
  boost::shared_ptr<Ekiga::Settings> user_interface_settings;
  boost::shared_ptr<Ekiga::Settings> sound_events_settings;
  boost::shared_ptr<Ekiga::Settings> contacts_settings;
};

/* channel types */
enum {
  CHANNEL_FIRST,
  CHANNEL_AUDIO,
  CHANNEL_LAST
};

static const char* win_menu =
"<?xml version='1.0'?>"
"<interface>"
"  <menu id='menubar'>"
"  </menu>"
"</interface>";


/* GUI Functions */
static void place_call_cb (GtkWidget * /*widget*/,
                           gpointer data);

static void url_changed_cb (GtkEditable *e,
                            gpointer data);

static void ekiga_window_append_call_url (EkigaWindow *mw,
                                                const char *url);

static void ekiga_window_trim_call_url (EkigaWindow *mw);



/* DESCRIPTION  :  This callback is called when the user
 *                 presses a key.
 * BEHAVIOR     :  Sends a DTMF if we are in a call.
 * PRE          :  A valid pointer to the main window GMObject.
 */
static gboolean key_press_event_cb (EkigaWindow *mw,
                                    GdkEventKey *key);


/* DESCRIPTION  :  This callback is called when the user
 *                 clicks on the dialpad button.
 * BEHAVIOR     :  Generates a dialpad event.
 * PRE          :  A valid pointer to the main window GMObject.
 */
static void dialpad_button_clicked_cb (EkigaDialpad  *dialpad,
                                       const gchar *button_text,
                                       EkigaWindow *ekiga_window);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates the uri toolbar in the dialpad panel.
 * PRE          :  The main window GMObject.
 */
static GtkWidget *ekiga_window_uri_entry_new (EkigaWindow *mw);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates the actions toolbar in the main window.
 * PRE          :  The main window GMObject.
 */
static void ekiga_window_init_actions_toolbar (EkigaWindow *mw);

static void call_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer app);

static GActionEntry win_entries[] =
{
    { "call", call_activated, NULL, NULL, NULL, 0 }
};


/*
 * Callbacks
 */
static bool
call_uri_helper_cb (Ekiga::AccountPtr acc,
                              const gchar* text,
                              EkigaWindow* mw)
{
  Opal::AccountPtr account = boost::dynamic_pointer_cast<Opal::Account>(acc);
  if (account && account->is_active ()) {
    mw->priv->call_uri = gm_make_uri (text, account->get_outbound_proxy());
    return false;
  }
  return true;
}


static void
place_call_cb (GtkWidget * /*widget*/,
               gpointer data)
{
  EkigaWindow *mw = NULL;

  g_return_if_fail (EKIGA_IS_WINDOW (data));

  mw = EKIGA_WINDOW (data);

  if (mw->priv->calling_state == Standby) {
    const gchar* uri = gtk_entry_get_text (GTK_ENTRY (mw->priv->entry));

    if (!uri || !strlen(uri))
      return;

    mw->priv->call_uri = "";

    boost::shared_ptr<Opal::Bank> b = mw->priv->bank.lock ();
    b->visit_accounts (boost::bind (&call_uri_helper_cb, _1, uri, mw));

    gtk_entry_set_text (GTK_ENTRY (mw->priv->entry), "");

    if (mw->priv->call_uri.empty())
      return;

    // Dial
    if (!mw->priv->call_core->dial (mw->priv->call_uri))
      gm_info_bar_push_message (GM_INFO_BAR (mw->priv->info_bar),
                                GTK_MESSAGE_ERROR,
                                _("Could not connect to remote host"));
  }
}

static void
url_changed_cb (GtkEditable *e,
                gpointer data)
{
  EkigaWindow *mw = EKIGA_WINDOW (data);

  const gchar *text = gtk_entry_get_text (GTK_ENTRY (e));
  gtk_widget_set_sensitive (GTK_WIDGET (mw->priv->call_button), (strlen(text) > 0));
}


static void
on_account_updated (Ekiga::AccountPtr account,
                    gpointer data)
{
  g_return_if_fail (EKIGA_IS_WINDOW (data));

  EkigaWindow *self = EKIGA_WINDOW (data);
  gchar *msg = NULL;

  gchar *title = g_strdup (_("Ekiga"));

  switch (account->get_state ()) {
  case Ekiga::Account::RegistrationFailed:
  case Ekiga::Account::UnregistrationFailed:
    msg = g_strdup_printf ("%s: %s",
                           account->get_name ().c_str (),
                           account->get_status ().c_str ());

    gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                              GTK_MESSAGE_ERROR, msg);

    g_free (msg);
    break;

  case Ekiga::Account::Registered:
    g_free (title);
    title = g_strdup_printf("%s (%s)", _("Ekiga"), account->get_username().c_str());
    break;

  case Ekiga::Account::Unregistered:
  case Ekiga::Account::Processing:
  default:
    break;
  }

  gtk_window_set_title (GTK_WINDOW (self), title);
  g_free (title);
}


static void on_setup_call_cb (boost::shared_ptr<Ekiga::Call>  call,
                              gpointer self)
{
  EkigaWindow *mw = EKIGA_WINDOW (self);

  if (!call->is_outgoing ()) {
    if (mw->priv->current_call)
      return; // No call setup needed if already in a call

    mw->priv->audiooutput_core->start_play_event ("incoming-call-sound", 4000, 256);

    mw->priv->current_call = call;
    mw->priv->calling_state = Called;
  }
  else {

    mw->priv->current_call = call;
    mw->priv->calling_state = Calling;
  }

  /* Unsensitive a few things */
  gtk_widget_set_sensitive (GTK_WIDGET (mw->priv->entry), false);
}


static void on_ringing_call_cb (boost::shared_ptr<Ekiga::Call>  call,
                                gpointer self)
{
  EkigaWindow *mw = EKIGA_WINDOW (self);

  if (call->is_outgoing ()) {
    mw->priv->audiooutput_core->start_play_event("ring-tone-sound", 3000, 256);
  }
}


static void on_established_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                                    gpointer self)
{
  EkigaWindow *mw = EKIGA_WINDOW (self);

  /* Update calling state */
  mw->priv->calling_state = Connected;

  /* Manage sound events */
  mw->priv->audiooutput_core->stop_play_event("incoming-call-sound");
  mw->priv->audiooutput_core->stop_play_event("ring-tone-sound");
}


static void on_cleared_call_cb (boost::shared_ptr<Ekiga::Call> call,
                                std::string /*reason*/,
                                gpointer self)
{
  EkigaWindow *mw = EKIGA_WINDOW (self);

  if (mw->priv->current_call && mw->priv->current_call->get_id () != call->get_id ()) {
    return; // Trying to clear another call than the current active one
  }

  /* Update calling state */
  if (mw->priv->current_call)
    mw->priv->current_call = boost::shared_ptr<Ekiga::Call>();
  mw->priv->calling_state = Standby;


  /* Sound events */
  mw->priv->audiooutput_core->stop_play_event("incoming-call-sound");
  mw->priv->audiooutput_core->stop_play_event("ring-tone-sound");

  /* Sensitive a few things back */
  gtk_widget_set_sensitive (GTK_WIDGET (mw->priv->entry), true);
}


static void on_missed_call_cb (boost::shared_ptr<Ekiga::Call>  call,
                               gpointer self)
{
  EkigaWindow *mw = EKIGA_WINDOW (self);

  /* Display info first */
  gchar* info = NULL;
  info = g_strdup_printf (_("Missed call from %s"),
                          call->get_remote_party_name ().c_str ());
  gm_info_bar_push_message (GM_INFO_BAR (mw->priv->info_bar),
                            GTK_MESSAGE_INFO, info);
  g_free (info);

  // FIXME: the engine should take care of this
  /* If the cleared call is the current one, switch back to standby, otherwise return
   * as long as the information has been displayed */
  if (mw->priv->current_call && mw->priv->current_call->get_id () == call->get_id ()) {
    mw->priv->current_call = boost::shared_ptr<Ekiga::Call>();
    mw->priv->calling_state = Standby;

    /* Sensitive a few things back */
    gtk_widget_set_sensitive (GTK_WIDGET (mw->priv->entry), true);

    /* Clear sounds */
    mw->priv->audiooutput_core->stop_play_event ("incoming-call-sound");
    mw->priv->audiooutput_core->stop_play_event ("ring-tone-sound");
  }
}


static bool on_handle_errors (std::string error,
                              gpointer data)
{
  g_return_val_if_fail (data != NULL, false);

  GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (data),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_ERROR,
                                                          GTK_BUTTONS_OK,
                                                          "%s",
                                                          error.c_str ());

  gtk_window_set_title (GTK_WINDOW (dialog), _("Error"));
  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);

  gtk_widget_show_all (dialog);

  return true;
}


/* GTK callbacks */
static void
dialpad_button_clicked_cb (EkigaDialpad  * /* dialpad */,
                           const gchar *button_text,
                           EkigaWindow *mw)
{
  if (mw->priv->current_call && mw->priv->calling_state == Connected)
    mw->priv->current_call->send_dtmf (button_text[0]);
  else
    ekiga_window_append_call_url (mw, button_text);
}


static gboolean
key_press_event_cb (EkigaWindow *mw,
                    GdkEventKey *key)
{
  const char valid_dtmfs[] = "1234567890#*";
  unsigned i = 0;

  if (mw->priv->current_call) {
    while (i < strlen (valid_dtmfs)) {
      if (key->string[0] && key->string[0] == valid_dtmfs[i]) {
        mw->priv->current_call->send_dtmf (key->string[0]);
        return true;
      }
      i++;
    }
  } else if (key->keyval == GDK_KEY_BackSpace) {
    ekiga_window_trim_call_url (mw);
    return true;

  } else if (key->keyval == GDK_KEY_Return || key->keyval == GDK_KEY_KP_Enter) {
    place_call_cb(NULL, mw);
    return true;
  }

  return false;
}


static void
insert_url_cb (G_GNUC_UNUSED GtkEditable *entry,
	       gchar *new_text,
	       G_GNUC_UNUSED gint new_text_length,
	       G_GNUC_UNUSED gpointer position,
	       G_GNUC_UNUSED gpointer data)
{
  const char valid_dtmfs[] = "1234567890#*";

  gchar *text = g_strdup(new_text);

  unsigned l = 0;
  for (unsigned k = 0; k < strlen(text); k++)
    for (unsigned i = 0; i < strlen (valid_dtmfs); i++)
      if (text[k] == valid_dtmfs[i]) {
        new_text[l++] = text[k];
        break;
      }
  new_text[l] = '\0';

  g_free(text);
}


static void
call_activated (G_GNUC_UNUSED GSimpleAction *action,
                G_GNUC_UNUSED GVariant *parameter,
                gpointer win)
{
  g_return_if_fail (EKIGA_IS_WINDOW (win));

  EkigaWindow *mw = EKIGA_WINDOW (win);
  place_call_cb (NULL, mw);
}


static void
ekiga_window_append_call_url (EkigaWindow *mw,
                                    const char *url)
{
  int pos = -1;
  GtkEditable *entry;

  g_return_if_fail (EKIGA_IS_WINDOW (mw));
  g_return_if_fail (url != NULL);

  entry = GTK_EDITABLE (mw->priv->entry);

  if (gtk_editable_get_selection_bounds (entry, NULL, NULL))
    gtk_editable_delete_selection (entry);

  pos = gtk_editable_get_position (entry);
  gtk_editable_insert_text (entry, url, strlen (url), &pos);
  gtk_editable_select_region (entry, -1, -1);
  gtk_editable_set_position (entry, pos);
}


static void
ekiga_window_trim_call_url (EkigaWindow *mw)
{
  int pos = -1;
  GtkEditable *entry;

  g_return_if_fail (EKIGA_IS_WINDOW (mw));

  entry = GTK_EDITABLE (mw->priv->entry);

  if (gtk_editable_get_selection_bounds (entry, NULL, NULL)) {
    gtk_editable_delete_selection (entry);
    return;
  }

  if (!gtk_widget_is_focus(GTK_WIDGET (entry)))
    gtk_editable_set_position (entry, -1);

  pos = gtk_editable_get_position (entry);
  if (pos > 0) {
    gtk_editable_select_region (entry, pos - 1, pos);
    gtk_editable_delete_selection (entry);

    gtk_editable_select_region (entry, -1, -1);
    gtk_editable_set_position (entry, pos - 1);
  }
}


static GtkWidget *
ekiga_window_uri_entry_new (EkigaWindow *mw)
{
  GtkWidget *entry = NULL;

  g_return_val_if_fail (EKIGA_IS_WINDOW (mw), NULL);

  /* URI Entry */
  entry = gm_entry_new (NUMBER_REGEX);

  gtk_widget_add_accelerator (entry, "grab-focus",
                              mw->priv->accel, GDK_KEY_L,
                              (GdkModifierType) GDK_CONTROL_MASK,
                              (GtkAccelFlags) 0);
  gtk_editable_set_position (GTK_EDITABLE (entry), -1);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (url_changed_cb), mw);
  g_signal_connect (entry, "activated",
                    G_CALLBACK (place_call_cb), mw);
  g_signal_connect (entry, "insert-text",
                    G_CALLBACK (insert_url_cb), mw);
  return entry;
}


static void
ekiga_window_init_actions_toolbar (EkigaWindow *mw)
{
  GtkWidget *switcher = NULL;

  g_return_if_fail (EKIGA_IS_WINDOW (mw));

  mw->priv->actions_toolbar = gtk_header_bar_new ();
  gtk_window_set_titlebar (GTK_WINDOW (mw), mw->priv->actions_toolbar);

  /* Start packing buttons */
  mw->priv->menu_button = GTK_WIDGET (gtk_menu_button_new ());
  gtk_menu_button_set_use_popover (GTK_MENU_BUTTON(mw->priv->menu_button), TRUE);

  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (mw->priv->menu_button),
                                  G_MENU_MODEL (gtk_builder_get_object (mw->priv->builder, "winmenu")));
  gtk_header_bar_pack_start (GTK_HEADER_BAR (mw->priv->actions_toolbar), GTK_WIDGET (mw->priv->menu_button));

  switcher = gtk_stack_switcher_new ();
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher), GTK_STACK (mw->priv->main_stack));
//  gtk_header_bar_set_custom_title (GTK_HEADER_BAR (mw->priv->actions_toolbar), switcher);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (mw->priv->actions_toolbar), switcher);
//  gtk_widget_set_margin_end (GTK_WIDGET (switcher), 6);

  gtk_widget_show_all (mw->priv->actions_toolbar);

  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (mw->priv->actions_toolbar), FALSE);
}


static void
ekiga_window_init_dialpad (EkigaWindow *mw)
{
  GtkWidget *dialpad = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *image = NULL;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_halign (hbox, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_valign (hbox, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);

  mw->priv->entry = ekiga_window_uri_entry_new (mw);
  gtk_container_add(GTK_CONTAINER(hbox), mw->priv->entry);

  mw->priv->call_button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("call-start-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (mw->priv->call_button), image);
  gtk_widget_set_tooltip_text (GTK_WIDGET (mw->priv->call_button), _("Call"));
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (mw->priv->call_button), "win.call");
  gtk_container_add (GTK_CONTAINER (hbox), mw->priv->call_button);

  dialpad = ekiga_dialpad_new (mw->priv->accel);
  gtk_widget_set_hexpand (dialpad, TRUE);
  gtk_widget_set_vexpand (dialpad, TRUE);
  gtk_widget_set_halign (dialpad, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (dialpad, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (vbox), dialpad);
  g_signal_connect (dialpad, "button-clicked",
                    G_CALLBACK (dialpad_button_clicked_cb), mw);

  gtk_stack_add_named (GTK_STACK (mw->priv->main_stack), vbox, "dialpad");
  gtk_container_child_set (GTK_CONTAINER (mw->priv->main_stack),
                           vbox,
                           "icon-name", "input-dialpad-symbolic", NULL);

  g_signal_connect (mw, "key-press-event",
                    G_CALLBACK (key_press_event_cb), mw);
}


static void
ekiga_window_init_history (EkigaWindow *mw)
{
  boost::shared_ptr<History::Source> history_source = mw->priv->history_source.lock ();
  if (history_source) {
    boost::shared_ptr<History::Book> history_book = history_source->get_book ();

    mw->priv->call_history_view = call_history_view_gtk_new (history_book,
                                                             mw->priv->call_core,
                                                             mw->priv->contact_core);
    gtk_stack_add_named (GTK_STACK (mw->priv->main_stack), mw->priv->call_history_view, "call-history");
    gtk_container_child_set (GTK_CONTAINER (mw->priv->main_stack),
                             mw->priv->call_history_view,
                             "icon-name", "document-open-recent-symbolic", NULL);
  }
}


static void
ekiga_window_init_gui (EkigaWindow *mw)
{
  GtkWidget *window_vbox;
  // FIXME ??? ekiga-settings.h
  gtk_window_set_title (GTK_WINDOW (mw), _("Ekiga Softphone"));
  gtk_window_set_icon_name (GTK_WINDOW (mw), PACKAGE_NAME);

  window_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (mw), window_vbox);
  gtk_widget_show_all (window_vbox);

  /* The info bar */
  mw->priv->info_bar = gm_info_bar_new ();
  gtk_box_pack_start (GTK_BOX (window_vbox),
                      GTK_WIDGET (mw->priv->info_bar), TRUE, TRUE, 0);

  /* The main stack */
  mw->priv->main_stack = gtk_stack_new ();
  gtk_stack_set_transition_type (GTK_STACK (mw->priv->main_stack),
                                 GTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT);

  /* The actions toolbar */
  ekiga_window_init_actions_toolbar (mw);

  /* The stack pages */
  ekiga_window_init_dialpad (mw);
  ekiga_window_init_history (mw);
  gtk_widget_show_all (mw->priv->main_stack);
  gtk_box_pack_start (GTK_BOX (window_vbox), mw->priv->main_stack,
                      true, true, 0);

  /* Set initial call button state */
  gtk_widget_set_sensitive (GTK_WIDGET (mw->priv->call_button), false);

  gtk_window_set_type_hint (GTK_WINDOW (mw), GDK_WINDOW_TYPE_HINT_MENU);

  /* Realize */
  gtk_widget_realize (GTK_WIDGET (mw));
}


static void
ekiga_window_init (EkigaWindow *mw)
{
  mw->priv = new EkigaWindowPrivate;

  /* Accelerators */
  mw->priv->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (mw), mw->priv->accel);
  g_object_unref (mw->priv->accel);

  mw->priv->current_call = boost::shared_ptr<Ekiga::Call>();
  mw->priv->calling_state = Standby;
  mw->priv->call_window = NULL;

  mw->priv->user_interface_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (USER_INTERFACE ".main-window"));
  mw->priv->sound_events_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (SOUND_EVENTS_SCHEMA));
  mw->priv->contacts_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CONTACTS_SCHEMA));
}


static GObject *
ekiga_window_constructor (GType the_type,
                                guint n_construct_properties,
                                GObjectConstructParam *construct_params)
{
  GObject *object;

  object = G_OBJECT_CLASS (ekiga_window_parent_class)->constructor
    (the_type, n_construct_properties, construct_params);

  return object;
}

static void
ekiga_window_dispose (GObject* gobject)
{
  EkigaWindow *mw = EKIGA_WINDOW (gobject);

  // Workaround bug #724506
  // Unbind exactly once, then reset the variable. GTK+ will
  // destroy the widget when things get finalized.
  if (mw->priv->main_stack) {
    g_settings_unbind (mw->priv->main_stack, "visible-child-name");
    mw->priv->main_stack = NULL;
  }

  if (mw->priv->builder) {
    g_object_unref (mw->priv->builder);
    mw->priv->builder = NULL;
  }

  G_OBJECT_CLASS (ekiga_window_parent_class)->dispose (gobject);
}

static void
ekiga_window_finalize (GObject *gobject)
{
  EkigaWindow *mw = EKIGA_WINDOW (gobject);

  delete mw->priv;

  G_OBJECT_CLASS (ekiga_window_parent_class)->finalize (gobject);
}

static gboolean
ekiga_window_focus_in_event (GtkWidget     *widget,
                                   GdkEventFocus *event)
{
  if (gtk_window_get_urgency_hint (GTK_WINDOW (widget)))
    gtk_window_set_urgency_hint (GTK_WINDOW (widget), FALSE);

  return GTK_WIDGET_CLASS (ekiga_window_parent_class)->focus_in_event (widget, event);
}


static void
ekiga_window_class_init (EkigaWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = ekiga_window_constructor;
  object_class->dispose = ekiga_window_dispose;
  object_class->finalize = ekiga_window_finalize;

  widget_class->focus_in_event = ekiga_window_focus_in_event;
}


static void
ekiga_window_connect_engine_signals (EkigaWindow *mw)
{
  boost::signals2::connection conn;

  g_return_if_fail (EKIGA_IS_WINDOW (mw));

  /* Engine Signals callbacks */
  // FIXME: We should NOT use the Opal::Bank directly here. It makes no sense.
  // We should visit Banks and Accounts.
  boost::shared_ptr<Opal::Bank> b = mw->priv->bank.lock ();
  conn = b->account_updated.connect (boost::bind (&on_account_updated, _1, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->setup_call.connect (boost::bind (&on_setup_call_cb, _1, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->ringing_call.connect (boost::bind (&on_ringing_call_cb, _1, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->established_call.connect (boost::bind (&on_established_call_cb, _1, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->cleared_call.connect (boost::bind (&on_cleared_call_cb, _1, _2, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->missed_call.connect (boost::bind (&on_missed_call_cb, _1, (gpointer) mw));
  mw->priv->connections.add (conn);

  conn = mw->priv->call_core->errors.connect (boost::bind (&on_handle_errors, _1, (gpointer) mw));
  mw->priv->connections.add (conn);
}

GtkWidget *
gm_ekiga_window_new (GmApplication *app)
{
  EkigaWindow *mw;

  g_return_val_if_fail (GM_IS_APPLICATION (app), NULL);

  /* basic gtk+ setup  */
  mw = EKIGA_WINDOW (g_object_new (EKIGA_TYPE_WINDOW,
                                   "application", GTK_APPLICATION (app),
                                   "key", USER_INTERFACE ".main-window",
                                   NULL));
  Ekiga::ServiceCore& core = gm_application_get_core (app);

  /* fetching needed engine objects */
  mw->priv->app = app;

  mw->priv->account_core
    = core.get<Ekiga::AccountCore> ("account-core");
  mw->priv->audiooutput_core
    = core.get<Ekiga::AudioOutputCore>("audiooutput-core");
  mw->priv->call_core
    = core.get<Ekiga::CallCore> ("call-core");
  mw->priv->contact_core
    = core.get<Ekiga::ContactCore> ("contact-core");
  mw->priv->presence_core
    = core.get<Ekiga::PresenceCore> ("presence-core");
  mw->priv->bank = boost::weak_ptr<Opal::Bank> (core.get<Opal::Bank> ("opal-account-store"));
  mw->priv->history_source
    = core.get<History::Source> ("call-history-store");

  ekiga_window_connect_engine_signals (mw);

  ekiga_window_init_gui (mw);

  return GTK_WIDGET(mw);
}
