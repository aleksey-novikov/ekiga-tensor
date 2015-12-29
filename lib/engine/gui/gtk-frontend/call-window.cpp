/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2012 Damien Sandras <dsandras@seconix.com>
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
 *                         call_window.cpp  -  description
 *                         -------------------------------
 *   begin                : Wed Dec 28 2012
 *   copyright            : (C) 2000-2012 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          build the call window.
 */

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include <boost/smart_ptr.hpp>

#include "config.h"

#include "ekiga-settings.h"

#include "call-window.h"

#include "dialpad.h"

#include "opal-bank.h"

#include "gm-info-bar.h"
#include "gactor-menu.h"
#include "scoped-connections.h"

#ifndef WIN32
#include <signal.h>
#include <gdk/gdkx.h>
#else
#include "platform/winpaths.h"
#include <gdk/gdkwin32.h>
#include <cstdio>
#endif

#if defined(P_FREEBSD) || defined (P_MACOSX)
#include <libintl.h>
#endif

#include "engine.h"
#include "call-core.h"
#include "audioinput-core.h"
#include "audiooutput-core.h"
#include "rtcp-statistics.h"

#include "uri.h"

#define CALL_IMAGE_SIZE "CALL_WINDOW_CALL_IMAGE_SIZE"

enum CallingState {Standby, Calling, Ringing, Connected, Called};

G_DEFINE_TYPE (EkigaCallWindow, ekiga_call_window, GM_TYPE_WINDOW);

struct _EkigaCallWindowPrivate
{
  boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core;
  boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core;
  boost::weak_ptr<Opal::Bank> bank;

  GtkAccelGroup *accel;

  boost::shared_ptr<Ekiga::Call> current_call;
  CallingState calling_state;

  GtkBuilder *builder;

  bool dead;

  GtkWidget *call_status_text;
  GtkWidget *call_duration_text;
  GtkWidget *call_image_frame;
  GtkWidget *call_image;

  GtkWidget *local_user_text;
  GtkWidget *remote_user_text;
  GtkWidget *remote_uri_text;

  GtkWidget *transfer_button;
  GtkWidget *hold_button;
  GtkWidget *pickup_button;
  GtkWidget *hangup_button;

  GtkWidget *transfer_box;
  GtkWidget *call_entry;
  GtkWidget *call_button;

#ifndef WIN32
  GC gc;
#endif

  unsigned int destroy_timeout_id;
  unsigned int timeout_id;

  Ekiga::scoped_connections connections;

  std::string transfer_string;

  /* GSettings */
  boost::shared_ptr<Ekiga::Settings> call_options_settings;
};


static void on_audioinput_device_error_cb (Ekiga::AudioInputManager & /* manager */,
                                           Ekiga::AudioInputDevice & device,
                                           Ekiga::AudioInputErrorCodes error_code,
                                           gpointer self);

static void on_audiooutput_device_error_cb (Ekiga::AudioOutputManager & /*manager */,
                                            Ekiga::AudioOutputPS ps,
                                            Ekiga::AudioOutputDevice & device,
                                            Ekiga::AudioOutputErrorCodes error_code,
                                            gpointer self);

static void on_ringing_call_cb (boost::shared_ptr<Ekiga::Call> call,
                                gpointer self);

static void on_established_call_cb (boost::shared_ptr<Ekiga::Call> call,
                                    gpointer self);

static void on_cleared_call_cb (boost::shared_ptr<Ekiga::Call> call,
                                std::string reason,
                                gpointer self);

static void on_missed_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                               gpointer self);

static void on_held_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                             gpointer self);

static void on_retrieved_call_cb (boost::shared_ptr<Ekiga::Call> /*call*/,
                                  gpointer self);

static gboolean on_stats_refresh_cb (gpointer self);

static gboolean on_delayed_destroy_cb (gpointer self);

static void ekiga_call_window_delete_event_cb (GtkWidget *widget,
                                                   G_GNUC_UNUSED GdkEventAny *event);

/**/
static void ekiga_call_window_set_local_user (EkigaCallWindow *self,
                                              const char *local_user);

static void ekiga_call_window_set_remote_uri (EkigaCallWindow *self,
                                              const char *remote_uri);

static void ekiga_call_window_set_remote_user (EkigaCallWindow *self,
                                               const char *remote_user);

static void ekiga_call_window_set_call_duration (EkigaCallWindow *self,
                                                 const char *duration);

static void ekiga_call_window_update_calling_state (EkigaCallWindow *self,
                                                    CallingState calling_state,
                                                    const char *call_status = NULL);

static void ekiga_call_window_set_call_hold (EkigaCallWindow *self,
                                             bool is_on_hold);

static void ekiga_call_window_init_gui (EkigaCallWindow *self);


// Actions
static void pickup_activated (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer win);

static void hangup_activated (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer win);

static void hold_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer win);

static void transfer_activated (GSimpleAction *action,
                                GVariant *parameter,
                                gpointer win);

static void call_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer win);

static GActionEntry win_entries[] =
{
    { "pickup", pickup_activated, NULL, NULL, NULL, 0 },
    { "hangup", hangup_activated, NULL, NULL, NULL, 0 },
    { "hold", hold_activated, NULL, NULL, NULL, 0 },
    { "transfer", transfer_activated, NULL, NULL, NULL, 0 },
    { "call", call_activated, NULL, NULL, NULL, 0 }
};


static gboolean
transfer_current_call_loop_cb (gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  g_return_val_if_fail (EKIGA_IS_CALL_WINDOW (self), false);

  if (!self->priv->current_call || self->priv->transfer_string.empty()) {
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->transfer_box), true);
    return false;
  }

  self->priv->current_call->send_dtmf (self->priv->transfer_string[0]);
  self->priv->transfer_string = self->priv->transfer_string.substr (1);

  return true;
}


static void
transfer_current_call_cb (G_GNUC_UNUSED GtkWidget *widget,
                          gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  const gchar *url = gtk_entry_get_text (GTK_ENTRY (self->priv->call_entry));

  if (!url || !strlen(url))
    return;

  std::string number = gm_transform_number (url);
  if (number.empty())
    return;

  if (self->priv->current_call) {
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->transfer_box), false);
    self->priv->transfer_string = std::string("#") + number;
    g_timeout_add (self->priv->call_options_settings->get_int ("dtmf-timeout"), transfer_current_call_loop_cb, self);
  }
}


static bool
local_user_helper_cb (Ekiga::AccountPtr acc, EkigaCallWindow *self)
{
  Opal::AccountPtr account = boost::dynamic_pointer_cast<Opal::Account>(acc);
  if (account && account->is_active () && account->get_status() == _("Registered")) {
    ekiga_call_window_set_local_user (self, account->get_username().c_str());
    return false;
  }
  return true;
}


static void
on_audioinput_device_error_cb (Ekiga::AudioInputManager & /* manager */,
                               Ekiga::AudioInputDevice & device,
                               Ekiga::AudioInputErrorCodes error_code,
                               gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);
  gchar *message = NULL;

  switch (error_code) {

  case Ekiga::AI_ERROR_DEVICE:
    message = g_strdup_printf (_("Unable to open %s for recording.\n\nIn case it is a pluggable device, it may be sufficient to reconnect it. If not, or if it still does not work, please check your audio setup, the permissions and that the device is not busy."), (const char *) device.name.c_str ());
    break;

  case Ekiga::AI_ERROR_READ:
    message = g_strdup_printf (_("%s was successfully opened but it is impossible to read data from this device.\n\nIn case it is a pluggable device it may be sufficient to reconnect it. If not, or if it still does not work, please check your audio setup."), (const char *) device.name.c_str ());
    break;

  case Ekiga::AI_ERROR_NONE:
  default:
    message = g_strdup_printf (_("Error while opening audio input device %s"), (const char *) device.name.c_str ());
    break;
  }

  GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (self),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_ERROR,
                                                          GTK_BUTTONS_OK,
                                                          "%s",
                                                          message);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Error"));
  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);

  gtk_widget_show_all (dialog);

  g_free (message);
}


static void
on_audiooutput_device_error_cb (Ekiga::AudioOutputManager & /*manager */,
                                Ekiga::AudioOutputPS ps,
                                Ekiga::AudioOutputDevice & device,
                                Ekiga::AudioOutputErrorCodes error_code,
                                gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);
  gchar *message = NULL;

  if (ps == Ekiga::secondary)
    return;

  switch (error_code) {

  case Ekiga::AO_ERROR_DEVICE:
    message = g_strdup_printf (_("Unable to open %s for playing.\n\nIn case it is a pluggable device it may be sufficient to reconnect it. If not, or if it still does not work, please check your audio setup, the permissions and that the device is not busy."), (const char *) device.name.c_str ());
    break;

  case Ekiga::AO_ERROR_WRITE:
    message = g_strdup_printf (_("%s was successfully opened but it is impossible to write data to this device.\n\nIn case it is a pluggable device it may be sufficient to reconnect it. If not, or if it still does not work, please check your audio setup."), (const char *) device.name.c_str ());
    break;

  case Ekiga::AO_ERROR_NONE:
  default:
    message = g_strdup_printf (_("Error while opening audio output device %s"),
                               (const char *) device.name.c_str ());
    break;
  }

  GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (self),
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_ERROR,
                                                          GTK_BUTTONS_OK,
                                                          "%s",
                                                          message);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Error"));
  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (gtk_widget_destroy),
                            dialog);

  gtk_widget_show_all (dialog);

  g_free (message);
}


static void
on_ringing_call_cb (G_GNUC_UNUSED boost::shared_ptr<Ekiga::Call>  call,
                    gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  g_return_if_fail (self);

  ekiga_call_window_update_calling_state (self, Ringing);
}


static void
on_established_call_cb (boost::shared_ptr<Ekiga::Call> call,
                        gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  ekiga_call_window_update_calling_state (self, Connected);
  ekiga_call_window_set_remote_user (self, call->get_remote_party_name ().c_str ());

  self->priv->current_call = call;

  on_stats_refresh_cb (self);
  self->priv->timeout_id = g_timeout_add_seconds (1, on_stats_refresh_cb, self);
}


static void
on_cleared_call_cb (boost::shared_ptr<Ekiga::Call> call,
                    G_GNUC_UNUSED std::string reason,
                    gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (self->priv->current_call && self->priv->current_call->get_id () != call->get_id ()) {
    return; // Trying to clear another call than the current active one
  }

  if (self->priv->current_call) {
    self->priv->current_call = boost::shared_ptr<Ekiga::Call>();

    if (self->priv->timeout_id > 0)
      g_source_remove (self->priv->timeout_id);
    self->priv->timeout_id = 0;
  }

  ekiga_call_window_update_calling_state (self, Standby, reason.c_str());
}

static void on_missed_call_cb (boost::shared_ptr<Ekiga::Call> call,
                               gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (self->priv->current_call && call && self->priv->current_call->get_id () != call->get_id ()) {
    return; // Trying to clear another call than the current active one
  }
  ekiga_call_window_update_calling_state (self, Standby, _("Missed call"));

  self->priv->current_call = boost::shared_ptr<Ekiga::Call>();
}


static void
on_held_call_cb (boost::shared_ptr<Ekiga::Call>  /*call*/,
                 gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  ekiga_call_window_set_call_hold (self, true);
}


static void
on_retrieved_call_cb (boost::shared_ptr<Ekiga::Call>  /*call*/,
                      gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  ekiga_call_window_set_call_hold (self, false);
}


static gboolean
on_stats_refresh_cb (gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (self->priv->calling_state == Connected && self->priv->current_call) {
    ekiga_call_window_set_remote_user (self, self->priv->current_call->get_remote_party_name ().c_str ());
    ekiga_call_window_set_call_duration (self, self->priv->current_call->get_duration ().c_str ());
  }

  return true;
}


static gboolean
on_delayed_destroy_cb (gpointer win)
{
  g_return_val_if_fail (EKIGA_IS_CALL_WINDOW (win), FALSE);

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (win);

  if (self->priv->destroy_timeout_id > 0)
    g_source_remove (self->priv->destroy_timeout_id);

  gtk_widget_destroy (GTK_WIDGET (self));

  return FALSE;
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
url_changed_cb (GtkEditable *e,
                gpointer win)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (win));

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (win);

  const gchar *text = gtk_entry_get_text (GTK_ENTRY (e));
  gtk_widget_set_sensitive (GTK_WIDGET (self->priv->call_button), (strlen(text) > 0));
}


static void
pickup_activated (G_GNUC_UNUSED GSimpleAction *action,
                  G_GNUC_UNUSED GVariant *parameter,
                  gpointer win)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (win));

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (win);

  if (self->priv->current_call && self->priv->calling_state == Called)
    self->priv->current_call->answer ();
}


static void
hangup_activated (G_GNUC_UNUSED GSimpleAction *action,
                  G_GNUC_UNUSED GVariant *parameter,
                  gpointer win)
{
  ekiga_call_window_delete_event_cb (GTK_WIDGET (win), NULL);
}


static void
hold_activated (G_GNUC_UNUSED GSimpleAction *action,
                G_GNUC_UNUSED GVariant *parameter,
                gpointer win)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (win));

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (win);

  if (self->priv->current_call)
    self->priv->current_call->toggle_hold ();
}


static void
transfer_activated (G_GNUC_UNUSED GSimpleAction *action,
                    G_GNUC_UNUSED GVariant *parameter,
                    gpointer win)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (win));

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (win);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (self->priv->transfer_button))) {
    gtk_entry_set_text (GTK_ENTRY (self->priv->call_entry), "");
    gtk_widget_set_sensitive (GTK_WIDGET (self->priv->call_button), FALSE);
    gtk_widget_show (self->priv->transfer_box);
    gtk_widget_grab_focus (self->priv->call_entry);
  } else
    gtk_widget_hide (self->priv->transfer_box);
}


static void
call_activated (G_GNUC_UNUSED GSimpleAction *action,
                G_GNUC_UNUSED GVariant *parameter,
                gpointer win)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (win));

  transfer_current_call_cb (NULL, win);
}


static void
ekiga_call_window_delete_event_cb (GtkWidget *widget,
                                   G_GNUC_UNUSED GdkEventAny *event)
{
  EkigaCallWindow *self = NULL;

  self = EKIGA_CALL_WINDOW (widget);
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));
  if (self->priv->dead)
    return;

  self->priv->dead = true;

  if (self->priv->calling_state != Standby && self->priv->current_call) {
    self->priv->current_call->hang_up ();
  }

  /* Destroying the call window directly is not nice
   * from the user perspective.
   */
  if (self->priv->destroy_timeout_id > 0)
    g_source_remove (self->priv->destroy_timeout_id);
  self->priv->destroy_timeout_id = g_timeout_add (100, on_delayed_destroy_cb, self);

  return;
}


static void
ekiga_call_window_set_local_user (EkigaCallWindow *self,
                                  const char *local_user)
{
  gchar *str = g_strdup_printf("%s: %s", _("My number"), local_user);
  gtk_label_set_text (GTK_LABEL (self->priv->local_user_text), str);
  g_free (str);
}

static void
ekiga_call_window_set_remote_user (EkigaCallWindow *self,
                                   const char *remote_user)
{
  char *user = g_strdup(remote_user);
  char *buffer = g_strdup(remote_user);

  char *p1 = strchr (user, ':');
  char *p2 = strrchr (user, '@');

  if (!p1)
    p1 = user;
  else
    p1++;

  char *q = buffer;
  while (*p1 != 0 && (p2 == NULL || p1 < p2))
    *q++ = *p1++;

  *q = 0;

  gchar *markup = g_markup_printf_escaped ("<span size=\"large\">%s</span>", buffer);
  gtk_label_set_markup(GTK_LABEL (self->priv->remote_user_text), markup);
  g_free (markup);
  g_free (buffer);
  g_free (user);
}


static void
ekiga_call_window_set_remote_uri (EkigaCallWindow *self,
                                  const char *remote_uri)
{
  gchar *markup = g_markup_printf_escaped ("<span size=\"large\">%s</span>", remote_uri);
  gtk_label_set_markup(GTK_LABEL (self->priv->remote_uri_text), markup);
  g_free (markup);
}


static void
ekiga_call_window_set_call_duration (EkigaCallWindow *self,
                                     const char *duration)
{
  gchar *markup = g_markup_printf_escaped ("<span>%s</span>", duration);
  gtk_label_set_markup(GTK_LABEL (self->priv->call_duration_text), markup);
  g_free (markup);
}


static void
ekiga_call_window_update_calling_state (EkigaCallWindow *self,
                                        CallingState calling_state,
                                        const char *call_status)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  self->priv->calling_state = calling_state;

  if (self->priv->call_image) {
    gtk_container_remove (GTK_CONTAINER (self->priv->call_image_frame), self->priv->call_image);
    self->priv->call_image = NULL;
  }

  switch (calling_state)
    {
    case Standby:
      gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), call_status);
      if (call_status && !strcmp(call_status, _("Remote user is busy")))
        self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing-busy", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
      else if (call_status && (!strcmp(call_status, _("Service unavailable")) || !strcmp(call_status, _("User is not available"))))
        self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing-unreachable", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
      else if (call_status && (!strcmp (call_status, _("User not found"))))
        self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing-wrong", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
      else
        self->priv->call_image = gtk_image_new_from_icon_name ("call-cleared", gtk_icon_size_from_name (CALL_IMAGE_SIZE));

      /* Update the hold state */
      ekiga_call_window_set_call_hold (self, false);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->transfer_button), false);

      gtk_widget_hide (self->priv->transfer_button);
      gtk_widget_hide (self->priv->hold_button);
      gtk_widget_hide (self->priv->pickup_button);
      gtk_widget_show (self->priv->hangup_button);

      /* Auto destroy */
      if (!self->priv->dead) {
        if (self->priv->destroy_timeout_id > 0)
          g_source_remove (self->priv->destroy_timeout_id);
        self->priv->destroy_timeout_id = g_timeout_add_seconds (10, on_delayed_destroy_cb, self);
      }
      break;

    case Calling:
      self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
      gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Outgoing call"));
      gtk_label_set_text (GTK_LABEL (self->priv->call_duration_text), "");

      gtk_widget_hide (self->priv->transfer_button);
      gtk_widget_hide (self->priv->hold_button);
      gtk_widget_hide (self->priv->pickup_button);
      gtk_widget_show (self->priv->hangup_button);
      break;

    case Ringing:
      if (self->priv->current_call->is_outgoing()) {
        self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Outgoing call"));
      } else {
        self->priv->call_image = gtk_image_new_from_icon_name ("call-incoming", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Incoming call"));
      }
      gtk_label_set_text (GTK_LABEL (self->priv->call_duration_text), "");
      break;

    case Connected:
      if (self->priv->current_call->is_outgoing()) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->hold_button)))
          self->priv->call_image = gtk_image_new_from_icon_name ("call-on-hold", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        else
          self->priv->call_image = gtk_image_new_from_icon_name ("call-outgoing-answered", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Outgoing call"));
      } else {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->priv->hold_button)))
          self->priv->call_image = gtk_image_new_from_icon_name ("call-on-hold", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        else
          self->priv->call_image = gtk_image_new_from_icon_name ("call-incoming-answered", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
        gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Incoming call"));
      }

      gtk_widget_show (self->priv->transfer_button);
      gtk_widget_show (self->priv->hold_button);
      gtk_widget_hide (self->priv->pickup_button);
      gtk_widget_show (self->priv->hangup_button);
      break;

    case Called:
      self->priv->call_image = gtk_image_new_from_icon_name ("call-incoming", gtk_icon_size_from_name (CALL_IMAGE_SIZE));
      gtk_label_set_text (GTK_LABEL (self->priv->call_status_text), _("Incoming call"));
      gtk_label_set_text (GTK_LABEL (self->priv->call_duration_text), "");

      gtk_widget_hide (self->priv->transfer_button);
      gtk_widget_hide (self->priv->hold_button);
      gtk_widget_show (self->priv->pickup_button);
      gtk_widget_show (self->priv->hangup_button);
      break;

    default:
      break;
    }

  if (self->priv->call_image) {
    gtk_container_add (GTK_CONTAINER (self->priv->call_image_frame), self->priv->call_image);
    gtk_widget_show (self->priv->call_image);
  }
}

static void
ekiga_call_window_set_call_hold (EkigaCallWindow *self,
                                 bool is_on_hold)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  if (is_on_hold)
    gtk_button_set_label (GTK_BUTTON (self->priv->hold_button), _("Retrieve Call"));
  else
    gtk_button_set_label (GTK_BUTTON (self->priv->hold_button), _("Hold Call"));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->hold_button),
                                is_on_hold);

  if (self->priv->calling_state == Connected)
    ekiga_call_window_update_calling_state (self, self->priv->calling_state);
}


static void
ekiga_call_window_init_gui (EkigaCallWindow *self)
{
  GtkWidget *alignment = NULL;
  GtkWidget *box = NULL;

  GtkWidget *image = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *hbox = NULL;

  // Init actions
  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   self);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), box);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), hbox, true, true, 10);

  // Left VBox
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, false, false, 10);

  self->priv->call_image_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (self->priv->call_image_frame), GTK_SHADOW_IN);
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->call_image_frame);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (alignment), true, false, 5);

  self->priv->call_image = NULL;

  self->priv->local_user_text = gtk_label_new (NULL);
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->local_user_text);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (alignment), false, false, 0);

  // Right VBox
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, true, true, 10);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  gtk_box_pack_start (GTK_BOX (vbox), alignment, true, true, 10);

  self->priv->call_status_text = gtk_label_new ("Call Type");
  gtk_box_pack_start (GTK_BOX (hbox), self->priv->call_status_text, false, false, 0);

  self->priv->call_duration_text = gtk_label_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), self->priv->call_duration_text, false, false, 0);


  GtkWidget *vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, false, false, 0);

  self->priv->remote_user_text = gtk_label_new ("Remote user");
  alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->remote_user_text);
  gtk_box_pack_start (GTK_BOX (vbox2), alignment, false, false, 0);

  self->priv->remote_uri_text = gtk_label_new ("Remote URI");
  alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->remote_uri_text);
  gtk_box_pack_start (GTK_BOX (vbox2), alignment, false, false, 0);


  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (alignment), false, false, 0);

  self->priv->hangup_button = gtk_button_new_with_label (_("Hangup"));
  image = gtk_image_new_from_icon_name ("call-stop-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (self->priv->hangup_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->hangup_button), "win.hangup");
  gtk_box_pack_end (GTK_BOX (hbox), self->priv->hangup_button, false, false, 0);

  self->priv->pickup_button = gtk_button_new_with_label (_("Pickup"));
  image = gtk_image_new_from_icon_name ("call-start-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (self->priv->pickup_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->pickup_button), "win.pickup");
  gtk_box_pack_end (GTK_BOX (hbox), self->priv->pickup_button, false, false, 0);

  self->priv->hold_button = gtk_toggle_button_new_with_label (_("Hold"));
  image = gtk_image_new_from_icon_name ("media-playback-pause-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (self->priv->hold_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->hold_button), "win.hold");
  gtk_box_pack_end (GTK_BOX (hbox), self->priv->hold_button, false, false, 0);

  self->priv->transfer_button = gtk_toggle_button_new_with_label (_("Transfer"));
  image = gtk_image_new_from_icon_name ("send-to-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (self->priv->transfer_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->transfer_button), "win.transfer");
  gtk_box_pack_end (GTK_BOX (hbox), self->priv->transfer_button, false, false, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->transfer_button), false);

  self->priv->transfer_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  alignment = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->transfer_box);
  gtk_box_pack_end (GTK_BOX (vbox), alignment, false, false, 0);

  self->priv->call_entry = gtk_entry_new ();
  alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->call_entry);
  gtk_container_add (GTK_CONTAINER (self->priv->transfer_box), alignment);
  g_signal_connect (self->priv->call_entry, "changed",
                    G_CALLBACK (url_changed_cb), self);
  g_signal_connect (self->priv->call_entry, "activate",
                    G_CALLBACK (transfer_current_call_cb), self);
  g_signal_connect (self->priv->call_entry, "insert-text",
                    G_CALLBACK (insert_url_cb), self);

  self->priv->call_button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("call-start-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (self->priv->call_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->call_button), "win.call");
  alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (alignment), self->priv->call_button);
  gtk_container_add (GTK_CONTAINER (self->priv->transfer_box), alignment);

  gtk_widget_show_all (box);
  gtk_widget_hide (self->priv->transfer_box);

  ekiga_call_window_set_call_hold (self, false);

  gtk_window_set_type_hint (GTK_WINDOW (self), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);

  self->priv->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (self), self->priv->accel);
  gtk_widget_add_accelerator (self->priv->pickup_button, "clicked", self->priv->accel,
                              GDK_KEY_A, (GdkModifierType) GDK_SUPER_MASK, (GtkAccelFlags) 0);
  gtk_widget_add_accelerator (self->priv->pickup_button, "clicked", self->priv->accel,
                              GDK_KEY_a, (GdkModifierType) GDK_SUPER_MASK, (GtkAccelFlags) 0);
  gtk_widget_add_accelerator (self->priv->pickup_button, "clicked", self->priv->accel,
                              GDK_KEY_Cyrillic_EF, (GdkModifierType) GDK_SUPER_MASK, (GtkAccelFlags) 0);
  gtk_widget_add_accelerator (self->priv->pickup_button, "clicked", self->priv->accel,
                              GDK_KEY_Cyrillic_ef, (GdkModifierType) GDK_SUPER_MASK, (GtkAccelFlags) 0);
  gtk_widget_add_accelerator (self->priv->pickup_button, "clicked", self->priv->accel,
                              GDK_KEY_Phone, (GdkModifierType) 0, (GtkAccelFlags) 0);
  g_object_unref (self->priv->accel);
}


static void
ekiga_call_window_init (EkigaCallWindow *self)
{
  self->priv = new EkigaCallWindowPrivate ();

  self->priv->current_call = boost::shared_ptr<Ekiga::Call>();
  self->priv->destroy_timeout_id = 0;
  self->priv->timeout_id = 0;
  self->priv->calling_state = Standby;
  self->priv->dead = false;

  self->priv->call_options_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CALL_OPTIONS_SCHEMA));

  g_signal_connect (self, "delete_event",
                    G_CALLBACK (ekiga_call_window_delete_event_cb), NULL);
}


static void
ekiga_call_window_finalize (GObject *gobject)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (gobject);

  if (self->priv->timeout_id > 0)
    g_source_remove (self->priv->timeout_id);

  delete self->priv;

  G_OBJECT_CLASS (ekiga_call_window_parent_class)->finalize (gobject);
}


static void
ekiga_call_window_show (GtkWidget *widget)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (widget);

  ekiga_call_window_set_local_user (self, "----");

  boost::shared_ptr<Opal::Bank> b = self->priv->bank.lock ();
  b->visit_accounts (boost::bind (&local_user_helper_cb, _1, self));

  GTK_WIDGET_CLASS (ekiga_call_window_parent_class)->show (widget);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}


static gboolean
ekiga_call_window_focus_in_event (GtkWidget     *widget,
                                  GdkEventFocus *event)
{
  if (gtk_window_get_urgency_hint (GTK_WINDOW (widget)))
    gtk_window_set_urgency_hint (GTK_WINDOW (widget), false);

  return GTK_WIDGET_CLASS (ekiga_call_window_parent_class)->focus_in_event (widget, event);
}


static void
ekiga_call_window_class_init (EkigaCallWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ekiga_call_window_finalize;

  widget_class->show = ekiga_call_window_show;
  widget_class->focus_in_event = ekiga_call_window_focus_in_event;

  gtk_icon_size_register (CALL_IMAGE_SIZE, 128, 128);
}


GtkWidget *
call_window_new (GmApplication *app)
{
  EkigaCallWindow *self;

  g_return_val_if_fail (GM_IS_APPLICATION (app), NULL);

  self = EKIGA_CALL_WINDOW (g_object_new (EKIGA_TYPE_CALL_WINDOW,
                                          "application", GTK_APPLICATION (app),
                                          "key", USER_INTERFACE ".call-window",
                                          "hide_on_delete", false,
                                          NULL));

  Ekiga::ServiceCore& core = gm_application_get_core (app);

  self->priv->audioinput_core = core.get<Ekiga::AudioInputCore> ("audioinput-core");
  self->priv->audiooutput_core = core.get<Ekiga::AudioOutputCore> ("audiooutput-core");
  self->priv->bank = boost::weak_ptr<Opal::Bank> (core.get<Opal::Bank> ("opal-account-store"));

  ekiga_call_window_init_gui (self);

  /* AudioInput Engine signals */
  boost::signals2::connection conn;
  conn = self->priv->audioinput_core->device_error.connect (boost::bind (&on_audioinput_device_error_cb, _1, _2, _3, (gpointer) self));
  self->priv->connections.add (conn);

  /* AudioOutput Engine signals */
  conn = self->priv->audiooutput_core->device_error.connect (boost::bind (&on_audiooutput_device_error_cb, _1, _2, _3, _4, (gpointer) self));
  self->priv->connections.add (conn);

  return GTK_WIDGET (self);
}


void
call_window_present (EkigaCallWindow *self)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  if (self->priv->destroy_timeout_id > 0) {
    g_source_remove (self->priv->destroy_timeout_id);
    self->priv->destroy_timeout_id = 0;
  }
  self->priv->dead = false;

  gtk_window_present (GTK_WINDOW (self));
}


void
call_window_add_call (GtkWidget *call_window,
                      boost::shared_ptr<Ekiga::Call> call)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (call_window));

  EkigaCallWindow *self = EKIGA_CALL_WINDOW (call_window);
  if (self->priv->current_call)
    return;

  self->priv->current_call = call;

  /* Update UI elements */
  CallingState s = call->is_outgoing () ? Calling : Called;
  ekiga_call_window_update_calling_state (self, s);
  ekiga_call_window_set_remote_user (self, call->get_remote_uri ().c_str ());
  ekiga_call_window_set_remote_uri (self, call->get_remote_uri ().c_str ());

  /* Connect new signals */
  boost::signals2::connection conn;
  conn = call->ringing.connect (boost::bind (&on_ringing_call_cb, _1, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->established.connect (boost::bind (&on_established_call_cb, _1, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->cleared.connect (boost::bind (&on_cleared_call_cb, _1, _2, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->missed.connect (boost::bind (&on_missed_call_cb, _1, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->held.connect (boost::bind (&on_held_call_cb, _1, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->retrieved.connect (boost::bind (&on_retrieved_call_cb, _1, (gpointer) self));
  self->priv->connections.add (conn);
}
