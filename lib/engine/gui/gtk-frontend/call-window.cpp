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

#include "gm-info-bar.h"
#include "gactor-menu.h"
#include "scoped-connections.h"
#include "form-dialog-gtk.h"
#include "ext-window.h"

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

#define STAGE_WIDTH 640
#define STAGE_HEIGHT 480

enum CallingState {Standby, Calling, Ringing, Connected, Called};

G_DEFINE_TYPE (EkigaCallWindow, ekiga_call_window, GM_TYPE_WINDOW);

enum {

  COLOR,
  CONTRAST,
  BRIGHTNESS,
  WHITENESS,
  SPEAKER_VOLUME,
  MIC_VOLUME,
  MAX_SETTINGS,
};

struct _EkigaCallWindowPrivate
{
  boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core;
  boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core;

  GtkAccelGroup *accel;

  boost::shared_ptr<Ekiga::Call> current_call;
  CallingState calling_state;

  GtkWidget *event_box;
  GtkWidget *spinner;
  GtkBuilder *builder;

  bool dead;
  bool bad_connection;

  GtkWidget *call_panel_toolbar;

  GtkWidget *input_signal;
  GtkWidget *output_signal;
#ifndef WIN32
  GC gc;
#endif
  GtkWidget *settings_button;

  unsigned int destroy_timeout_id;
  unsigned int timeout_id;

  GtkWidget *info_bar;

  /* Audio settings */
  int settings[MAX_SETTINGS];
  GtkWidget *settings_range[MAX_SETTINGS];

  std::string transmitted_audio_codec;
  std::string received_audio_codec;

  Ekiga::GActorMenuPtr call_menu;

  Ekiga::scoped_connections connections;
};

/* channel types */
enum {
  CHANNEL_FIRST,
  CHANNEL_AUDIO,
  CHANNEL_LAST
};


static void fullscreen_changed_cb (G_GNUC_UNUSED GSimpleAction *action,
                                   G_GNUC_UNUSED GVariant *parameter,
                                   gpointer data);

static void show_call_devices_settings_cb (G_GNUC_UNUSED GSimpleAction *action,
                                           G_GNUC_UNUSED GVariant *parameter,
                                           gpointer data);

static void call_devices_settings_changed_cb (GtkRange *range,
                                              gpointer data);

static void on_audioinput_device_opened_cb (Ekiga::AudioInputManager & /* manager */,
                                            Ekiga::AudioInputDevice & /* device */,
                                            Ekiga::AudioInputSettings & settings,
                                            gpointer self);

static void on_audioinput_device_closed_cb (Ekiga::AudioInputManager & /* manager */,
                                            Ekiga::AudioInputDevice & /*device*/,
                                            gpointer self);

static void on_audioinput_device_error_cb (Ekiga::AudioInputManager & /* manager */,
                                           Ekiga::AudioInputDevice & device,
                                           Ekiga::AudioInputErrorCodes error_code,
                                           gpointer self);

static void on_audiooutput_device_opened_cb (Ekiga::AudioOutputManager & /*manager*/,
                                             Ekiga::AudioOutputPS ps,
                                             Ekiga::AudioOutputDevice & /*device*/,
                                             Ekiga::AudioOutputSettings & settings,
                                             gpointer self);

static void on_audiooutput_device_closed_cb (Ekiga::AudioOutputManager & /*manager*/,
                                             Ekiga::AudioOutputPS ps,
                                             Ekiga::AudioOutputDevice & /*device*/,
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

static void on_stream_opened_cb (boost::shared_ptr<Ekiga::Call> /* call */,
                                 std::string name,
                                 Ekiga::Call::StreamType type,
                                 bool is_transmitting,
                                 gpointer self);

static void on_stream_closed_cb (boost::shared_ptr<Ekiga::Call> /* call */,
                                 G_GNUC_UNUSED std::string name,
                                 Ekiga::Call::StreamType type,
                                 bool is_transmitting,
                                 gpointer self);

static bool on_handle_questions (Ekiga::FormRequestPtr request,
                                 gpointer data);

static gboolean on_stats_refresh_cb (gpointer self);

static gboolean on_delayed_destroy_cb (gpointer self);

static gboolean ekiga_call_window_delete_event_cb (GtkWidget *widget,
                                                   G_GNUC_UNUSED GdkEventAny *event);

static gboolean ekiga_call_window_fullscreen_event_cb (GtkWidget *widget,
                                                       G_GNUC_UNUSED GdkEventAny *event);

/**/
static void ekiga_call_window_remove_action_entries (GActionMap *map,
                                                      const GActionEntry *entries);

static void ekiga_call_window_update_calling_state (EkigaCallWindow *self,
                                                    CallingState calling_state);

static void ekiga_call_window_update_header_bar_actions (EkigaCallWindow *self,
                                                         CallingState calling_state);

static void ekiga_call_window_clear_signal_levels (EkigaCallWindow *self);

static void ekiga_call_window_clear_stats (EkigaCallWindow *self);

static void ekiga_call_window_update_title (EkigaCallWindow *self,
                                            unsigned calling_state,
                                            const std::string & remote_party = std::string ());

static void ekiga_call_window_update_stats (EkigaCallWindow *self,
                                            const RTCPStatistics & statistics);

static void ekiga_call_window_init_menu (EkigaCallWindow *self);

static void ekiga_call_window_init_clutter (EkigaCallWindow *self);

static GtkWidget *gm_call_window_build_settings_popover (EkigaCallWindow *call_window,
                                                         GtkWidget *relative);

static void ekiga_call_window_toggle_fullscreen (EkigaCallWindow *self);

static void ekiga_call_window_init_gui (EkigaCallWindow *self);

static void
show_call_devices_settings_cb (G_GNUC_UNUSED GSimpleAction *action,
                               G_GNUC_UNUSED GVariant *parameter,
                               gpointer data)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (data));
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  gtk_widget_show_all (gm_call_window_build_settings_popover (self,
                                                              self->priv->settings_button));
}

static void
call_devices_settings_changed_cb (G_GNUC_UNUSED GtkRange *range,
                                  gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  for (int i = 0 ; i < MAX_SETTINGS ; i++) {
    if (self->priv->settings_range[i]) {
      self->priv->settings[i] = gtk_range_get_value (GTK_RANGE (self->priv->settings_range[i]));
    }
  }

  if (self->priv->settings[SPEAKER_VOLUME] != -1)
    self->priv->audiooutput_core->set_volume (Ekiga::primary, self->priv->settings[SPEAKER_VOLUME]);
  if (self->priv->settings[MIC_VOLUME] != -1)
    self->priv->audioinput_core->set_volume (self->priv->settings[MIC_VOLUME]);
}


static void
on_audioinput_device_opened_cb (Ekiga::AudioInputManager & /* manager */,
                                Ekiga::AudioInputDevice & /* device */,
                                Ekiga::AudioInputSettings & settings,
                                gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);
  if (settings.modifyable)
    self->priv->settings[MIC_VOLUME] = settings.volume;
}


static void
on_audioinput_device_closed_cb (Ekiga::AudioInputManager & /* manager */,
                                Ekiga::AudioInputDevice & /*device*/,
                                gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  self->priv->settings[MIC_VOLUME] = -1;
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

  gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                            GTK_MESSAGE_ERROR, message);
  g_free (message);
}


static void
on_audiooutput_device_opened_cb (Ekiga::AudioOutputManager & /*manager*/,
                                 Ekiga::AudioOutputPS ps,
                                 Ekiga::AudioOutputDevice & /*device*/,
                                 Ekiga::AudioOutputSettings & settings,
                                 gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (ps == Ekiga::secondary)
    return;

  if (settings.modifyable)
    self->priv->settings[SPEAKER_VOLUME] = settings.volume;
}


static void
on_audiooutput_device_closed_cb (Ekiga::AudioOutputManager & /*manager*/,
                                 Ekiga::AudioOutputPS ps,
                                 Ekiga::AudioOutputDevice & /*device*/,
                                 gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (ps == Ekiga::secondary)
    return;

  self->priv->settings[SPEAKER_VOLUME] = -1;
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

  gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                            GTK_MESSAGE_ERROR, message);

  g_free (message);
}


static void
on_ringing_call_cb (G_GNUC_UNUSED boost::shared_ptr<Ekiga::Call>  call,
                    gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  g_return_if_fail (self);

  ekiga_call_window_update_calling_state (self, Ringing);
  ekiga_call_window_update_header_bar_actions (self, Ringing);
}


static void
on_established_call_cb (boost::shared_ptr<Ekiga::Call> call,
                        gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  ekiga_call_window_update_calling_state (self, Connected);
  ekiga_call_window_update_title (self, Connected, call->get_remote_party_name ());
  ekiga_call_window_update_header_bar_actions (self, Connected);

  self->priv->current_call = call;

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

  ekiga_call_window_update_calling_state (self, Standby);
  ekiga_call_window_update_title (self, Standby);
  ekiga_call_window_update_header_bar_actions (self, Standby);
  ekiga_call_window_clear_stats (self);

  if (self->priv->current_call) {
    self->priv->current_call = boost::shared_ptr<Ekiga::Call>();

    if (self->priv->destroy_timeout_id > 0)
      g_source_remove (self->priv->destroy_timeout_id);
    if (self->priv->timeout_id > 0)
      g_source_remove (self->priv->timeout_id);

    self->priv->destroy_timeout_id = 0;
    self->priv->timeout_id = 0;
    self->priv->bad_connection = false;
    self->priv->call_menu.reset ();
  }

  ekiga_call_window_clear_signal_levels (self);

  gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                            GTK_MESSAGE_INFO, reason.c_str ());
}

static void on_missed_call_cb (boost::shared_ptr<Ekiga::Call> call,
                               gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (self->priv->current_call && call && self->priv->current_call->get_id () != call->get_id ()) {
    return; // Trying to clear another call than the current active one
  }
  self->priv->bad_connection = false;
  self->priv->call_menu.reset ();

  ekiga_call_window_update_calling_state (self, Standby);
  ekiga_call_window_update_title (self, Standby);
  ekiga_call_window_update_header_bar_actions (self, Standby);
}

static void
on_held_call_cb (boost::shared_ptr<Ekiga::Call>  /*call*/,
                 gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                            GTK_MESSAGE_INFO, _("Call on hold"));
}


static void
on_retrieved_call_cb (boost::shared_ptr<Ekiga::Call>  /*call*/,
                      gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                            GTK_MESSAGE_INFO, _("Call retrieved"));
}


static void
set_codec (EkigaCallWindowPrivate *priv,
           std::string name,
           bool is_transmitting)
{
  if (is_transmitting)
    priv->transmitted_audio_codec = name;
  else
    priv->received_audio_codec = name;
}

static void
on_stream_opened_cb (boost::shared_ptr<Ekiga::Call>  /* call */,
                     std::string name,
                     Ekiga::Call::StreamType type,
                     bool is_transmitting,
                     gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  set_codec (self->priv, name, is_transmitting);
}


static void
on_stream_closed_cb (boost::shared_ptr<Ekiga::Call>  /* call */,
                     G_GNUC_UNUSED std::string name,
                     Ekiga::Call::StreamType type,
                     bool is_transmitting,
                     gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  set_codec (self->priv, "", is_transmitting);
}


static bool
on_handle_questions (Ekiga::FormRequestPtr request,
                     gpointer data)
{
  FormDialog dialog (request, GTK_WIDGET (data));

  dialog.run ();

  return true;
}

static gboolean
on_stats_refresh_cb (gpointer data)
{
  EkigaCallWindow *self = EKIGA_CALL_WINDOW (data);

  if (self->priv->calling_state == Connected && self->priv->current_call) {
    gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self->priv->call_panel_toolbar),
                                  self->priv->current_call->get_duration ().c_str ());

    ekiga_call_window_update_stats (self, self->priv->current_call->get_statistics ());
  }

  return true;
}

static gboolean
on_delayed_destroy_cb (gpointer self)
{
  gtk_widget_destroy (GTK_WIDGET (self));

  return FALSE;
}

static gboolean
ekiga_call_window_delete_event_cb (GtkWidget *widget,
                                   G_GNUC_UNUSED GdkEventAny *event)
{
  EkigaCallWindow *self = NULL;
  GSettings *settings = NULL;

  self = EKIGA_CALL_WINDOW (widget);
  g_return_val_if_fail (EKIGA_IS_CALL_WINDOW (self), false);
  if (self->priv->dead)
    return true;

  self->priv->dead = true;

  /* Hang up or disable preview */
  if (self->priv->fullscreen) {
    ekiga_call_window_toggle_fullscreen (self);
  }

  if (self->priv->calling_state != Standby && self->priv->current_call) {
    self->priv->current_call->hang_up ();
  }

  /* Destroying the call window directly is not nice
   * from the user perspective.
   */
  self->priv->destroy_timeout_id = g_timeout_add_seconds (2, on_delayed_destroy_cb, self);

  return true;
}

static gboolean
ekiga_call_window_fullscreen_event_cb (GtkWidget *widget,
                                       G_GNUC_UNUSED GdkEventAny *event)
{
  EkigaCallWindow *self = NULL;

  self = EKIGA_CALL_WINDOW (widget);
  g_return_val_if_fail (EKIGA_IS_CALL_WINDOW (self), false);
  ekiga_call_window_toggle_fullscreen (self);

  return true; // Do not relay the event anymore
}

static void
ekiga_call_window_remove_action_entries (GActionMap *map,
                                         const GActionEntry *entries)
{
  for (unsigned int i = 0 ; i < G_N_ELEMENTS (entries) ; i++)
    g_action_map_remove_action (map, entries[i].name);
}

static void
ekiga_call_window_update_calling_state (EkigaCallWindow *self,
                                        CallingState calling_state)
{
  g_return_if_fail (self != NULL);

  switch (calling_state)
    {
    case Standby:
      /* Spinner updates */
      gtk_widget_hide (self->priv->spinner);
      gtk_spinner_stop (GTK_SPINNER (self->priv->spinner));

      /* Auto destroy */
      g_timeout_add_seconds (2, on_delayed_destroy_cb, self);
      break;

    case Calling:
      /* Spinner updates */
      gtk_widget_show (self->priv->spinner);
      gtk_spinner_start (GTK_SPINNER (self->priv->spinner));
      break;

    case Ringing:

      /* Spinner updates */
      gtk_widget_show (self->priv->spinner);
      gtk_spinner_start (GTK_SPINNER (self->priv->spinner));
      break;

    case Connected:

      /* Spinner updates */
      gtk_widget_hide (self->priv->spinner);
      gtk_spinner_start (GTK_SPINNER (self->priv->spinner));
      break;

    case Called:
    default:
      break;
    }

  self->priv->calling_state = calling_state;
}

static void
ekiga_call_window_update_header_bar_actions (EkigaCallWindow *self,
                                             CallingState calling_state)
{
  GList *it = NULL;
  g_return_if_fail (self != NULL);

  it = gtk_container_get_children (GTK_CONTAINER (self->priv->call_panel_toolbar));


  switch (calling_state) {

  case Called:
    while (it != NULL) {
      if (it->data && GTK_IS_ACTIONABLE (it->data)) {
        const char* action_name = gtk_actionable_get_action_name (GTK_ACTIONABLE (it->data));
        if (!g_strcmp0 (action_name, "win.reject") || !g_strcmp0 (action_name, "win.answer"))
          gtk_widget_show (GTK_WIDGET (it->data));
        else
          gtk_widget_hide (GTK_WIDGET (it->data));
      }
      it = g_list_next (it);
    }
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->priv->call_panel_toolbar), FALSE);
    break;

  case Standby:
  case Calling:
  case Ringing:
  case Connected:
  default:
    while (it != NULL) {
      if (it->data && GTK_IS_ACTIONABLE (it->data)) {
        const char* action_name = gtk_actionable_get_action_name (GTK_ACTIONABLE (it->data));
        if (!g_strcmp0 (action_name, "win.reject") || !g_strcmp0 (action_name, "win.answer"))
          gtk_widget_hide (GTK_WIDGET (it->data));
        else
          gtk_widget_show (GTK_WIDGET (it->data));
      }
      it = g_list_next (it);
    }
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->priv->call_panel_toolbar), TRUE);
    break;
  }

  g_list_free (it);
}

static void
ekiga_call_window_clear_signal_levels (EkigaCallWindow *self)
{
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  //gm_level_meter_clear (GM_LEVEL_METER (self->priv->output_signal));
  //gm_level_meter_clear (GM_LEVEL_METER (self->priv->input_signal));
}

static void
ekiga_call_window_clear_stats (EkigaCallWindow *self)
{
  RTCPStatistics stats;
  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  ekiga_call_window_update_stats (self, stats);
}

static void
ekiga_call_window_update_title (EkigaCallWindow *self,
                                unsigned calling_state,
                                const std::string & remote_party)
{
  g_return_if_fail (self != NULL);
  gchar *title = NULL;

  switch (calling_state)
    {
    case Calling:
      if (!remote_party.empty ())
        title = g_strdup_printf (_("Calling %s"), remote_party.c_str ());
      break;

    case Connected:
      if (!remote_party.empty ())
        title = g_strdup_printf (_("Connected with %s"), remote_party.c_str ());
      break;

    case Ringing:
      break;
    case Called:
      if (!remote_party.empty ())
        title = g_strdup_printf (_("Call from %s"), remote_party.c_str ());
      break;

    case Standby:
    default:
      title = g_strdup (_("Call Window"));
      break;
    }

  if (!title)
      title = g_strdup (_("Call Window"));

  gtk_header_bar_set_title (GTK_HEADER_BAR (self->priv->call_panel_toolbar), title);
  g_free (title);
}

static void
ekiga_call_window_update_stats (EkigaCallWindow *self,
                                const RTCPStatistics & stats)
{
  gchar *stats_msg = NULL;

  g_return_if_fail (EKIGA_IS_CALL_WINDOW (self));

  gchar *jitter, *remote_jitter;
  if (stats.jitter == -1)
    jitter = g_strdup (_("N/A"));
  else
    jitter = g_strdup_printf (_("%d ms"), stats.jitter);
  if (stats.remote_jitter == -1)
    remote_jitter = g_strdup (_("N/A"));
  else
    remote_jitter = g_strdup_printf (_("%d ms"), stats.remote_jitter);

  stats_msg =
    g_strdup_printf (_("<b><u>Reception:</u></b> %s\nLost Packets: %d %%\nJitter: %s\nBandwidth: %d kbits/s\n\n"
                       "<b><u>Transmission:</u></b> %s\nRemote Lost Packets: %d %%\nRemote Jitter: %s\nBandwidth: %d kbits/s\n\n"),
                     stats.received_audio_codec.c_str (), stats.lost_packets, jitter, stats.received_audio_bandwidth,
                     stats.transmitted_audio_codec.c_str (), stats.remote_lost_packets, remote_jitter, stats.transmitted_audio_bandwidth);
  gtk_widget_set_tooltip_markup (GTK_WIDGET (self->priv->event_box), stats_msg);

  if (!self->priv->bad_connection && (stats.jitter > 250 || stats.lost_packets > 2)) {

    gm_info_bar_push_message (GM_INFO_BAR (self->priv->info_bar),
                              GTK_MESSAGE_WARNING,
                              _("The call quality is rather bad. Please check your Internet connection or your audio driver."));
    self->priv->bad_connection = true;
  }

  g_free (jitter);
  g_free (remote_jitter);
  g_free (stats_msg);
}


static GtkWidget *
gm_call_window_build_settings_popover (EkigaCallWindow *self,
                                       GtkWidget *relative)
{
  GtkWidget *hbox = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *image = NULL;

  GtkWidget *popover = NULL;

  GIcon *icon = NULL;
  gboolean audio = FALSE;

  popover = gtk_popover_new (NULL);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (popover), 18);
  gtk_container_add (GTK_CONTAINER (popover), vbox);
  gtk_popover_set_relative_to (GTK_POPOVER (popover), relative);

  const char *icons[MAX_SETTINGS] = {
    "preferences-color-symbolic",
    "display-brightness-symbolic",
    "display-brightness-symbolic",
    "display-brightness-symbolic",
    "audio-speakers-symbolic",
    "audio-input-microphone-symbolic",
  };

  for (int i = 0 ; i < MAX_SETTINGS ; i++) {

    if (self->priv->settings[i] == -1)
      continue;

    audio = (i == SPEAKER_VOLUME || i == MIC_VOLUME);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    icon = g_themed_icon_new (icons[i]);
    image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_MENU);
    g_object_unref (icon);
    gtk_box_pack_start (GTK_BOX (hbox), image, false, false, 6);

    self->priv->settings_range[i] = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, audio ? 100.0 : 255.0, 5.0);
    gtk_range_set_value (GTK_RANGE (self->priv->settings_range[i]), self->priv->settings[i]);
    gtk_scale_set_draw_value (GTK_SCALE (self->priv->settings_range[i]), false);
    gtk_scale_set_value_pos (GTK_SCALE (self->priv->settings_range[i]), GTK_POS_RIGHT);
    gtk_box_pack_start (GTK_BOX (hbox), self->priv->settings_range[i], true, true, 6);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);
    gtk_widget_set_size_request (GTK_WIDGET (self->priv->settings_range[i]), 150, -1);

    g_signal_connect (self->priv->settings_range[i], "value-changed",
                      G_CALLBACK (call_devices_settings_changed_cb), self);
  }

  g_signal_connect_swapped (popover, "hide",
                            G_CALLBACK (gtk_widget_destroy), popover);

  return popover;
}


static void
ekiga_call_window_init_menu (EkigaCallWindow *self)
{
  g_return_if_fail (self != NULL);
  self->priv->builder = gtk_builder_new ();
  gtk_builder_add_from_string (self->priv->builder, win_menu, -1, NULL);

  g_action_map_add_action_entries (G_ACTION_MAP (g_application_get_default ()),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   self);

  gtk_widget_insert_action_group (GTK_WIDGET (self), "win",
                                  G_ACTION_GROUP (g_application_get_default ()));
}


static void
ekiga_call_window_init_gui (EkigaCallWindow *self)
{
  GtkWidget *event_box = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *frame = NULL;
  GtkWidget *button = NULL;

  GtkWidget *image = NULL;

  GIcon *icon = NULL;

  /* The main table */
  event_box = gtk_event_box_new ();
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
  gtk_container_add (GTK_CONTAINER (event_box), vbox);
  gtk_container_add (GTK_CONTAINER (frame), event_box);
  gtk_container_add (GTK_CONTAINER (self), frame);
  gtk_widget_show_all (frame);

  /* Menu */
  ekiga_call_window_init_menu (self);

  /* The widgets header bar */
  self->priv->call_panel_toolbar = gtk_header_bar_new ();
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->priv->call_panel_toolbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW (self), self->priv->call_panel_toolbar);
  gtk_window_set_icon_name (GTK_WINDOW (self), PACKAGE_NAME);
  gtk_widget_show (self->priv->call_panel_toolbar);

  /* The info bar */
  self->priv->info_bar = gm_info_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (self->priv->info_bar), FALSE, FALSE, 0);

  /* FIXME:
   * All those actions should be call specific.
   * We should generate the header bar actions like we generate a menu.
   * Probably introducing a GActorHeaderBar would be nice.
   * However, it is unneeded right now as we only support one call at a time.
   */

  /* Reject */
  button = gtk_button_new_with_mnemonic (_("_Reject"));
  icon = g_themed_icon_new ("phone-hang-up");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.reject");
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Reject the incoming call"));
  gtk_widget_show (button);

  /* Hang up */
  button = gtk_button_new ();
  icon = g_themed_icon_new ("call-end-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.hangup");
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Hang up the current call"));
  gtk_widget_show (button);

  /* Call Hold */
  button = gtk_toggle_button_new ();
  icon = g_themed_icon_new ("call-hold-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.hold");
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Hold or retrieve the current call"));
  gtk_widget_show (button);

  /* Call Transfer */
  button = gtk_button_new ();
  icon = g_themed_icon_new ("call-transfer-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.transfer");
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Transfer the current call"));
  gtk_widget_show (button);

  /* Devices settings */
  self->priv->settings_button = gtk_button_new ();
  icon = g_themed_icon_new ("emblem-system-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (self->priv->settings_button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->priv->settings_button),
                                           "win.call-devices-settings");
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self->priv->call_panel_toolbar),
                             self->priv->settings_button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->priv->settings_button),
                               _("Change audio settings"));
  gtk_widget_show (self->priv->settings_button);

  /* Call Accept */
  button = gtk_button_new_with_mnemonic (_("_Answer"));
  icon = g_themed_icon_new ("phone-pick-up");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.answer");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Answer the incoming call"));
  gtk_widget_show (button);

  /* Spinner */
  self->priv->spinner = gtk_spinner_new ();
  gtk_widget_set_size_request (GTK_WIDGET (self->priv->spinner), 24, 24);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self->priv->call_panel_toolbar), self->priv->spinner);

  /* Menu button */
  button = gtk_menu_button_new ();
  icon = g_themed_icon_new ("open-menu-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  g_object_set (G_OBJECT (button), "use-popover", true, NULL);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button),
                                  G_MENU_MODEL (gtk_builder_get_object (self->priv->builder, "menubar")));
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_show (button);

  /* Full Screen */
  button = gtk_button_new ();
  icon = g_themed_icon_new ("view-fullscreen-symbolic");
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  g_object_unref (icon);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (button), "win.enable-fullscreen");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self->priv->call_panel_toolbar), button);
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Switch to fullscreen"));
  gtk_widget_show (button);

  gtk_window_set_resizable (GTK_WINDOW (self), true);
  ekiga_call_window_update_header_bar_actions (self, Standby);
}

static void
ekiga_call_window_init (EkigaCallWindow *self)
{
  self->priv = new EkigaCallWindowPrivate ();

  self->priv->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (self), self->priv->accel);
  gtk_accel_group_connect (self->priv->accel, GDK_KEY_Escape, (GdkModifierType) 0, GTK_ACCEL_LOCKED,
                           g_cclosure_new_swap (G_CALLBACK (ekiga_call_window_delete_event_cb),
                                                (gpointer) self, NULL));
  gtk_accel_group_connect (self->priv->accel, GDK_KEY_F11, (GdkModifierType) 0, GTK_ACCEL_LOCKED,
                           g_cclosure_new_swap (G_CALLBACK (ekiga_call_window_fullscreen_event_cb),
                                                (gpointer) self, NULL));
  g_object_unref (self->priv->accel);

  self->priv->current_call = boost::shared_ptr<Ekiga::Call>();
  self->priv->destroy_timeout_id = 0;
  self->priv->timeout_id = 0;
  self->priv->calling_state = Standby;
  self->priv->dead = false;
  self->priv->bad_connection = false;

  for (int i = 0 ; i < MAX_SETTINGS ; i++)
    self->priv->settings[i] = -1;

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
                                          "hide_on_esc", false, NULL));
  Ekiga::ServiceCore& core = gm_application_get_core (app);

  self->priv->audioinput_core = core.get<Ekiga::AudioInputCore> ("audioinput-core");
  self->priv->audiooutput_core = core.get<Ekiga::AudioOutputCore> ("audiooutput-core");

  ekiga_call_window_init_gui (self);

  /* AudioInput Engine signals */
  boost::signals2::connection conn;
  conn = self->priv->audioinput_core->device_opened.connect (boost::bind (&on_audioinput_device_opened_cb, _1, _2, _3, (gpointer) self));
  self->priv->connections.add (conn);

  conn = self->priv->audioinput_core->device_closed.connect (boost::bind (&on_audioinput_device_closed_cb, _1, _2, (gpointer) self));
  self->priv->connections.add (conn);

  conn = self->priv->audioinput_core->device_error.connect (boost::bind (&on_audioinput_device_error_cb, _1, _2, _3, (gpointer) self));
  self->priv->connections.add (conn);

  /* AudioOutput Engine signals */
  conn = self->priv->audiooutput_core->device_opened.connect (boost::bind (&on_audiooutput_device_opened_cb, _1, _2, _3, _4, (gpointer) self));
  self->priv->connections.add (conn);

  conn = self->priv->audiooutput_core->device_closed.connect (boost::bind (&on_audiooutput_device_closed_cb, _1, _2, _3, (gpointer) self));
  self->priv->connections.add (conn);

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

  /* Update menu */
  self->priv->call_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*call));

  /* Update UI elements */
  CallingState s = call->is_outgoing () ? Calling : Called;
  ekiga_call_window_update_calling_state (self, s);
  ekiga_call_window_update_title (self, s, call->is_outgoing () ? call->get_remote_uri () : call->get_remote_party_name ());
  ekiga_call_window_update_header_bar_actions (self, s);

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

  conn = call->stream_opened.connect (boost::bind (&on_stream_opened_cb, _1, _2, _3, _4, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->stream_closed.connect (boost::bind (&on_stream_closed_cb, _1, _2, _3, _4, (gpointer) self));
  self->priv->connections.add (conn);

  conn = call->questions.connect (boost::bind (&on_handle_questions, _1, (gpointer) self));
  self->priv->connections.add (conn);
}
