 
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
 *                         pref_window.cpp  -  description
 *                         -------------------------------
 *   begin                : Tue Dec 26 2000
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains all the functions needed to
 *                          create the preferences window and all its callbacks
 *   Additional code      : Miguel Rodríguez Pérez  <miguelrp@gmail.com>
 */


#include "config.h"

#include <glib/gi18n.h>
#include <boost/tuple/tuple.hpp>

#include "ekiga-settings.h"

#include "preferences-window.h"

#include "scoped-connections.h"
#include "form-request-simple.h"
#include "form-dialog-gtk.h"

#include "codecsbox.h"
#include "gm-entry.h"

#ifndef HAVE_SIDEBAR
#include "gm-sidebar.h"
#endif

#ifdef WIN32
#include "platform/winpaths.h"
#endif

#define GTK_GRID_LAST_ROW(g, p) while (gtk_grid_get_child_at (GTK_GRID (g), 0, p++));


struct _PreferencesWindowPrivate
{
  _PreferencesWindowPrivate ();

  GtkWidget *audio_codecs_list;
  GtkWidget *sound_events_list;
  GtkWidget *audio_player;
  GtkWidget *sound_events_output;
  GtkWidget *audio_recorder;
  GtkWidget *iface;
  GtkWidget *fsbutton;
  GtkWidget *stack;

  GmApplication *app;

  boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core;
  boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core;
  boost::shared_ptr<Ekiga::Settings> protocols_settings;
  boost::shared_ptr<Ekiga::Settings> sip_settings;
  boost::shared_ptr<Ekiga::Settings> nat_settings;
  boost::shared_ptr<Ekiga::Settings> call_forwarding_settings;
  boost::shared_ptr<Ekiga::Settings> call_options_settings;
  boost::shared_ptr<Ekiga::Settings> personal_data_settings;
  boost::shared_ptr<Ekiga::Settings> sound_events_settings;
  boost::shared_ptr<Ekiga::Settings> audio_devices_settings;
  boost::shared_ptr<Ekiga::Settings> audio_codecs_settings;
  boost::shared_ptr<Ekiga::Settings> contacts_settings;
  boost::shared_ptr<Ekiga::Settings> queue_settings;
  Ekiga::scoped_connections connections;
};


_PreferencesWindowPrivate::_PreferencesWindowPrivate ()
{
  sound_events_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (SOUND_EVENTS_SCHEMA));
  audio_devices_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (AUDIO_DEVICES_SCHEMA));
  audio_codecs_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (AUDIO_CODECS_SCHEMA));
  personal_data_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (PERSONAL_DATA_SCHEMA));
  protocols_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (PROTOCOLS_SCHEMA));
  sip_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (SIP_SCHEMA));
  nat_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (NAT_SCHEMA));
  call_forwarding_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CALL_FORWARDING_SCHEMA));
  call_options_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CALL_OPTIONS_SCHEMA));
  contacts_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CONTACTS_SCHEMA));
  queue_settings =
    boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (QUEUE_SCHEMA));
}

G_DEFINE_TYPE (PreferencesWindow, preferences_window, GM_TYPE_WINDOW);

enum {
  COLUMN_STRING_RAW = 0,
  COLUMN_STRING_TRANSLATED,
  COLUMN_SENSITIVE,
  COLUMN_GSETTINGS,
};

typedef boost::tuple<std::string, std::string> Choice;
typedef std::list<Choice> Choices;
typedef std::list<Choice>::iterator Choices_iterator;
typedef std::list<Choice>::const_iterator Choices_const_iterator;


/* Declarations */

/* GUI Functions */

/* DESCRIPTION  :  /
 * BEHAVIOR     :  Builds the sound events list of the preferences window.
 * PRE          :  /
 */
static void gm_prefs_window_sound_events_list_build (PreferencesWindow *prefs_window);


/* DESCRIPTION  : /
 * BEHAVIOR     : Adds an update button connected to the given callback to
 * 		  the given GtkBox.
 * PRE          : A valid pointer to the container widget where to attach
 *                the button, followed by a label, the callback, a
 *                tooltip.
 */
static void gm_pw_add_update_button (GtkWidget *box,
                                     const char *label,
                                     GCallback func,
                                     const gchar *tooltip,
                                     gpointer data);


/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the general settings page.
 * PRE          : A valid pointer to the preferences window GMObject, and to the
 * 		  container widget where to attach the generated page.
 */
static void gm_pw_init_general_page (PreferencesWindow *self,
                                     GtkWidget *container);



/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the call options page.
 * PRE          : A valid pointer to the preferences window GMObject, and to the
 * 		  container widget where to attach the generated page.
 */
static void gm_pw_init_call_options_page (PreferencesWindow *self,
                                          GtkWidget *container);


/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the sound events settings page.
 * PRE          : A valid pointer to the preferences window GMObject, and to the
 * 		  container widget where to attach the generated page.
 */
static void gm_pw_init_sound_events_page (PreferencesWindow *self,
                                          GtkWidget *container);


/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the SIP settings page.
 * PRE          : A valid pointer to the preferences window GMObject, and to the
 * 		  container widget where to attach the generated page.
 */
static void gm_pw_init_sip_page (PreferencesWindow *self,
                                 GtkWidget *container);


/* DESCRIPTION  : /
 * BEHAVIOR     : Builds the audio settings page.
 * PRE          : A valid pointer to the preferences window GMObject, and to the
 * 		  container widget where to attach the generated page.
 */
static void gm_pw_init_audio_page (PreferencesWindow *self,
                                   GtkWidget *container);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a GtkEntry associated with a config key and returns
 *                 the result.
 *                 The first parameter is the section in which
 *                 the GtkEntry should be attached. The other parameters are
 *                 the text label, the config key, the tooltip.
 * PRE          :  /
 */
GtkWidget * gm_pw_entry_new (GtkWidget* grid,
                             const gchar *label_txt,
                             boost::shared_ptr<Ekiga::Settings> settings,
                             const std::string & key,
                             const gchar *tooltip,
                             bool indent = true);

/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a GtkHScale associated with a config key and
 *                 returns the result.
 *                 The first parameter is the section in which
 *                 the GtkHScale should be attached. The other parameters
 *                 are the text labels, the config key, the tooltip, the
 *                 minimal and maximal values and the incrementation step.
 * PRE          :  /
 */
GtkWidget *gm_pw_scale_new (GtkWidget* grid,
                            const gchar *down_label_txt,
                            const gchar *up_label_txt,
                            boost::shared_ptr <Ekiga::Settings> settings,
                            const std::string & key,
                            const gchar *tooltip,
                            double min,
                            double max,
                            double step,
                            bool indent = true);

/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a GtkSpinButton associated with a config key
 *                 and add it to the given GtkGrid container.
 *                 The first parameter is the section in which
 *                 the GtkSpinButton should be attached. The other parameters
 *                 are the text label(s), the config key, the tooltip, the
 *                 minimal and maximal values and the incrementation step.
 * PRE          :  /
 */
GtkWidget *gm_pw_spin_new (GtkWidget* grid,
                           const gchar *label_txt,
                           const gchar *label_txt2,
                           boost::shared_ptr <Ekiga::Settings> settings,
                           const std::string & key,
                           const gchar *tooltip,
                           double min,
                           double max,
                           double step,
                           bool indent = true);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a GtkToggleButton associated with a config key
 *                 and add it to the given GtkGrid container.
 *                 The first parameter is the section in which
 *                 the GtkWidget should be attached. The other parameters are
 *                 the text label, the config key, the tooltip.
 * PRE          :  /
 */
GtkWidget * gm_pw_toggle_new (GtkWidget* grid,
                              const gchar *label_txt,
                              boost::shared_ptr <Ekiga::Settings> settings,
                              const std::string & key,
                              const gchar *tooltip,
                              bool indent = true);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a subsection inside a section of a prefs window.
 *                 The parameters are the prefs window and the section name.
 * PRE          :  Not NULL name.
 */
void gm_pw_subsection_new (GtkWidget *container,
                           const gchar *name);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a new subsection in the given prefs window and
 *                 returns it. The parameter are the prefs window and the
 *                 prefs window subsection name. General subsections can
 *                 be created in the returned gnome prefs window subsection
 *                 and widgets can be attached to them.
 * PRE          :  /
 */
GtkWidget *gm_pw_window_subsection_new (GtkWidget *,
                                        const gchar *);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Creates a GtkOptionMenu associated with a string config
 *                 key and returns the result.
 *                 The first parameter is the section in which the GtkEntry
 *                 should be attached. The other parameters are the text label,
 *                 the possible values for the menu, the config key, the
 *                 tooltip.
 * PRE          :  The array ends with NULL.
 */
static GtkWidget *gm_pw_string_option_menu_new (GtkWidget *,
                                                const gchar *,
                                                const Choices &,
                                                boost::shared_ptr <Ekiga::Settings>,
                                                const std::string &,
                                                const gchar *,
                                                bool indent = true);


/* DESCRIPTION  :  /
 * BEHAVIOR     :  Updates the content of a GtkOptionMenu associated with
 *                 a string config key. The first parameter is the menu,
 *                 the second is the array of possible values, and the
 *                 last parameters are config related.
 * PRE          :  The array ends with NULL.
 */
static void gm_pw_string_option_menu_update (GtkWidget *option_menu,
                                             const Choices & options,
                                             boost::shared_ptr <Ekiga::Settings>,
                                             const std::string & key);

static void gm_pw_string_option_menu_add (GtkWidget *option_menu,
                                          const Choice & option,
                                          boost::shared_ptr <Ekiga::Settings>,
                                          const std::string & key);

static void gm_pw_string_option_menu_remove (GtkWidget *option_menu,
                                             const std::string & option,
                                             boost::shared_ptr <Ekiga::Settings>,
                                             const std::string & key);

static Choices gm_pw_get_device_choices (const std::vector<std::string> & v);

static void  gm_prefs_window_update_devices_list (PreferencesWindow *self);


/* Callbacks */

/* DESCRIPTION  :  This callback is called when the user clicks
 *                 on the refresh devices list button in the prefs.
 * BEHAVIOR     :  Redetects the devices and refreshes the menu.
 * PRE          :  /
 */
static void refresh_devices_list_cb (GtkWidget *widget,
                                     gpointer data);


/* DESCRIPTION  :  This callback is called when the user clicks
 *                 on a sound event in the list.
 * BEHAVIOR     :  It udpates the GtkFileChooser's filename to the config
 *                 value for the key corresponding to the currently
 *                 selected sound event.
 * PRE          :  /
 */
static void sound_event_selected_cb (GtkTreeSelection *selection,
                                     gpointer data);


/* DESCRIPTION  :  This callback is called when the user clicks
 *                 on the play button in the sound events list.
 * BEHAVIOR     :  Plays the currently selected sound event using the
 * 		   selected audio player and plugin through a GMSoundEvent.
 * PRE          :  The entry.
 */
static void sound_event_play_cb (GtkWidget *widget,
                                 gpointer data);


/* DESCRIPTION  :  This callback is called when the user clicks
 *                 on a sound event in the list and change the toggle.
 * BEHAVIOR     :  It udpates the config key associated with the currently
 *                 selected sound event so that it reflects the state of the
 *                 sound event (enabled or disabled) and also updates the list.
 * PRE          :  /
 */
static void sound_event_toggled_cb (GtkCellRendererToggle *cell,
                                    gchar *path_str,
                                    gpointer data);


/* DESCRIPTION  :  This callback is called when the user selected a file
 *                 with the file selector button for the image
 * BEHAVIOR     :  Update of the config database.
 * PRE          :  /
 */
/*
   static void image_filename_browse_cb (GtkWidget *widget,
   gpointer data);
 */

/* DESCRIPTION  :  This callback is called when the user selected a file
 *                 for a sound event
 * BEHAVIOR     :  Update of the config database.
 * PRE          :  /
 */
static void sound_event_changed_cb (GtkWidget *widget,
                                    gpointer data);


/* DESCRIPTION  :  This callback is called when something changes in the sound
 *                 event related settings.
 * BEHAVIOR     :  It updates the events list widget.
 * PRE          :  A pointer to the prefs window GMObject.
 */
static void sound_event_setting_changed (GSettings *,
                                         gchar *,
                                         gpointer data);


/* Implementation */
static void
gm_prefs_window_sound_events_list_build (PreferencesWindow *self)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;

  bool enabled = FALSE;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->priv->sound_events_list));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  /* Sound on incoming calls */
  enabled = self->priv->sound_events_settings->get_bool ("enable-incoming-call-sound");
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, enabled,
                      1, _("Play sound on incoming calls"),
                      2, "incoming-call-sound",
                      3, "enable-incoming-call-sound",
                      4, "incoming-call-sound",
                      -1);

  enabled = self->priv->sound_events_settings->get_bool ("enable-ring-tone-sound");
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, enabled,
                      1, _("Play ring tone"),
                      2, "ring-tone-sound",
                      3, "enable-ring-tone-sound",
                      4, "ring-tone-sound",
                      -1);

  enabled = self->priv->sound_events_settings->get_bool ("enable-busy-tone-sound");
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, enabled,
                      1, _("Play busy tone"),
                      2, "busy-tone-sound",
                      3, "enable-busy-tone-sound",
                      4, "busy-tone-sound",
                      -1);

  enabled = self->priv->sound_events_settings->get_bool ("enable-new-voicemail-sound");
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, enabled,
                      1, _("Play sound for new voice mails"),
                      2, "new-voicemail-sound",
                      3, "enable-new-voicemail-sound",
                      4, "new-voicemail-sound",
                      -1);

  enabled = self->priv->sound_events_settings->get_bool ("enable-new-message-sound");
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, enabled,
                      1, _("Play sound for new instant messages"),
                      2, "new-message-sound",
                      3, "enable-new-message-sound",
                      4, "new-message-sound",
                      -1);
}


static void
gm_pw_add_update_button (GtkWidget *container,
                         const char *label,
                         GCallback func,
                         const gchar *tooltip,
                         gpointer data)
{
  GtkWidget* button = NULL;

  int pos = 0;

  /* Update Button */
  button = gtk_button_new_with_mnemonic (label);
  gtk_widget_set_tooltip_text (button, tooltip);
  gtk_widget_set_halign (button, GTK_ALIGN_END);
  gtk_widget_set_valign (button, GTK_ALIGN_END);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);

  GTK_GRID_LAST_ROW (container, pos);
  gtk_grid_attach (GTK_GRID (container), button, 0, pos-1, 3, 1);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (func),
                    (gpointer) data);
}


static void
gm_pw_init_general_page (PreferencesWindow *self,
                         GtkWidget *container)
{
  GtkWidget *entry = NULL;

  /* Display */
  gm_pw_toggle_new (container, _("Show o_ffline contacts"),
                    self->priv->contacts_settings, "show-offline-contacts",
                    _("Show offline contacts in the roster"), false);

  /* History Settings */
  gm_pw_spin_new (container, _("History limit"), _("days"),
                  self->priv->contacts_settings, "history-limit",
                  _("Older entries are cleared on startup"),
                  1.0, 100.0, 1.0);

  /* Network Settings */
  gm_pw_subsection_new (container, _("Network Settings"));
  gm_pw_spin_new (container, _("Type of Service (TOS)"), NULL,
                  self->priv->protocols_settings, "rtp-tos-field",
                  _("The Type of Service (TOS) byte on outgoing RTP IP packets. This byte is used by the network to provide some level of Quality of Service (QoS). Default value 184 (0xB8) corresponds to Expedited Forwarding (EF) PHB as defined in RFC 3246."),
                  0.0, 255.0, 1.0);

  /* Queue Settings */
  gm_pw_subsection_new (container, _("Queue"));

  gm_pw_toggle_new (container, _("Enable queue"),
                    self->priv->queue_settings, "enable",
                    _("Enable queue support"), false);

  gm_pw_toggle_new (container, _("Enable entering/leaving queue"),
                    self->priv->queue_settings, "enable-enter-leave",
                    _("Enable entering and leaving queue support"), false);

  entry =
    gm_pw_entry_new (container, _("Enter queue number"),
                     self->priv->queue_settings, "enter",
                     _("Number to dial to enter the queue"),
                     false);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 100, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 15);

  entry =
    gm_pw_entry_new (container, _("Leave queue number"),
                     self->priv->queue_settings, "leave",
                     _("Number to dial to leave the queue"),
                     false);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 100, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 15);

  entry =
    gm_pw_entry_new (container, _("Pause queue number"),
                     self->priv->queue_settings, "pause",
                     _("Number to dial to pause the queue"),
                     false);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 100, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 15);

  entry =
    gm_pw_entry_new (container, _("Resume queue number"),
                     self->priv->queue_settings, "resume",
                     _("Number to dial to resume the queue"),
                     false);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 100, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 15);
}

static void
gm_pw_init_call_options_page (PreferencesWindow *self,
                              GtkWidget *container)
{
  GtkWidget *entry = NULL;

  gm_pw_toggle_new (container, _("_Always forward calls to the given host"),
                    self->priv->call_forwarding_settings, "always-forward",
                    _("If enabled, all incoming calls will be forwarded to the host that is specified in the protocol settings"), false);

  gm_pw_toggle_new (container, _("Forward calls to the given host if _no answer"),
                    self->priv->call_forwarding_settings, "forward-on-no-answer",
                    _("If enabled, all incoming calls will be forwarded to the host that is specified in the protocol settings if you do not answer the call"), false);

  gm_pw_toggle_new (container, _("Forward calls to the given host if _busy"),
                    self->priv->call_forwarding_settings, "forward-on-busy",
                    _("If enabled, all incoming calls will be forwarded to the host that is specified in the protocol settings if you already are in a call or if you are in busy mode"), false);


  gm_pw_subsection_new (container, _("Call Options"));

  /* Translators: the full sentence is Forward calls after x seconds. */
  gm_pw_spin_new (container, _("Forward calls after"), _("seconds"),
                  self->priv->call_options_settings, "no-answer-timeout",
                  _("Automatically reject or forward incoming calls if no answer is given after the specified amount of time (in seconds)"), 0.0, 299.0, 1.0);
  gm_pw_toggle_new (container, _("_Automatically answer incoming calls"),
                    self->priv->call_options_settings, "auto-answer",
                    _("If enabled, automatically answer incoming calls"));
  gm_pw_spin_new (container, _("Timeout between DTMFs"), _("msec"),
                  self->priv->call_options_settings, "dtmf-timeout",
                  _("Timeout between DTMFs on transfer call (in mseconds)"), 100.0, 1000.0, 100.0);

  /* Lengths and prefixes */
  gm_pw_subsection_new (container, _("Dial Prefixes"));

  gm_pw_spin_new (container, _("Local number length"), NULL,
                  self->priv->call_options_settings, "local-number-length",
                  _("Dial number length for local calls"), 3.0, 5.0, 1.0);

  entry =
    gm_pw_entry_new (container, _("Non-local number prefix"),
                     self->priv->call_options_settings, "nonlocal-number-prefix",
                     _("Prefix automatically added when dialing"),
                     false);
  g_object_set (entry, "regex", NUMBER_REGEX, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 40, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 5);

  gm_pw_spin_new (container, _("Outer number length"), NULL,
                  self->priv->call_options_settings, "outer-number-length",
                  _("Dial number length for outer calls"), 8.0, 12.0, 1.0);

  entry =
    gm_pw_entry_new (container, _("Outer number prefix"),
                     self->priv->call_options_settings, "outer-number-prefix",
                     _("Prefix automatically added when dialing"),
                     false);
  g_object_set (entry, "regex", NUMBER_REGEX, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (entry), 40, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 5);
}


static void
gm_pw_init_sound_events_page (PreferencesWindow *self,
                              GtkWidget *container)
{
  GtkWidget *button = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *frame = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *selector_hbox = NULL;
  GtkWidget *selector_playbutton = NULL;

  GtkListStore *list_store = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeViewColumn *column = NULL;

  GtkCellRenderer *renderer = NULL;

  GtkFileFilter *filefilter = NULL;

  PStringArray devs;
  int pos = 0;

  /* Packing widgets */
  GTK_GRID_LAST_ROW (container, pos);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  g_object_set (G_OBJECT (vbox), "expand", TRUE, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_grid_attach (GTK_GRID (container), vbox, 0, pos-1, 2, 1);

  /* The 3rd column will be invisible and contain the config key containing
     the file to play. The 4th one contains the key determining if the
     sound event is enabled or not. */
  list_store =
    gtk_list_store_new (5,
                        G_TYPE_BOOLEAN,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        G_TYPE_STRING,
                        G_TYPE_STRING);

  self->priv->sound_events_list =
    gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->priv->sound_events_list), FALSE);

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sound_events_list));

  frame = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), self->priv->sound_events_list);
  gtk_container_set_border_width (GTK_CONTAINER (self->priv->sound_events_list), 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);


  /* Set all Colums */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("A"),
                                                     renderer,
                                                     "active",
                                                     0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->sound_events_list), column);
  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (sound_event_toggled_cb), (gpointer) self);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Event"),
                                                     renderer,
                                                     "text",
                                                     1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (self->priv->sound_events_list), column);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);

  self->priv->fsbutton =
    gtk_file_chooser_button_new (_("Choose a sound"),
                                 GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_box_pack_start (GTK_BOX (hbox), self->priv->fsbutton, TRUE, TRUE, 2);

  filefilter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filefilter, _("Wavefiles"));
#ifdef WIN32
  gtk_file_filter_add_pattern (filefilter, "*.wav");
#else
  gtk_file_filter_add_mime_type (filefilter, "audio/x-wav");
#endif
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (self->priv->fsbutton), filefilter);

  selector_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  selector_playbutton = gtk_button_new_with_label (_("Play"));
  gtk_box_pack_end (GTK_BOX (selector_hbox),
                    selector_playbutton, FALSE, FALSE, 0);
  gtk_widget_show (selector_playbutton);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (self->priv->fsbutton),
                                     selector_hbox);

  g_signal_connect (self->priv->fsbutton, "selection-changed",
                    G_CALLBACK (sound_event_changed_cb),
                    (gpointer) self);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (sound_event_selected_cb),
                    (gpointer) self);

  button = gtk_button_new_with_label (_("Play"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (sound_event_play_cb),
                    (gpointer) self);

  /* Place it after the signals so that we can make sure they are run if
     required */
  gm_prefs_window_sound_events_list_build (self);
}


static void
gm_pw_init_sip_page (PreferencesWindow *self,
                     GtkWidget *container)
{
  GtkWidget *entry = NULL;

  Choices capabilities_choices;

  static const char *capabilities [][2] =
    {
        { "rfc2833", _("RFC2833") },
        { "info",    _("INFO") },
        { NULL,      NULL }
    };
  for (int i=0 ; capabilities[i][0] != NULL ; ++i)
    capabilities_choices.push_back (boost::make_tuple (capabilities[i][0], capabilities[i][1]));

  /* Add Misc Settings */
  entry =
    gm_pw_entry_new (container, _("Forward _URI"),
                     self->priv->sip_settings, "forward-host",
                     _("The host where calls should be forwarded if call forwarding is enabled"),
                     false);
  g_object_set (entry, "regex", BASIC_URI_REGEX, NULL);
  if (!g_strcmp0 (gtk_entry_get_text (GTK_ENTRY (entry)), ""))
    gtk_entry_set_text (GTK_ENTRY (entry), "sip:");

  /* Packing widget */
  gm_pw_subsection_new (container, _("DTMF Mode"));

  gm_pw_string_option_menu_new (container, _("_Send DTMF as"),
                                capabilities_choices,
                                self->priv->sip_settings, "dtmf-mode",
                                _("Select the mode for DTMFs sending"));
}


static void
gm_pw_init_audio_page (PreferencesWindow *self,
                       GtkWidget *container)
{
  GtkWidget *codecs_list = NULL;
  int pos = 0;

  std::vector<std::string> devices;

  /* Packing widgets */
  GTK_GRID_LAST_ROW (container, pos);
  codecs_list = codecs_box_new_with_type (self->priv->app, Ekiga::Call::Audio);
  gtk_grid_attach (GTK_GRID (container), codecs_list, 0, pos-1, 3, 1);

  /* Here we add the audio codecs options */
  gm_pw_subsection_new (container, _("Settings"));

  gm_pw_toggle_new (container, _("Enable silence _detection"),
                    self->priv->audio_codecs_settings, "enable-silence-detection",
                    _("If enabled, use silence detection with the codecs supporting it"));

  gm_pw_toggle_new (container, _("Enable echo can_celation"),
                    self->priv->audio_codecs_settings, "enable-echo-cancellation",
                    _("If enabled, use echo cancellation"));

  /* Audio Devices */
  gm_pw_subsection_new (container, _("Devices"));

  /* Add all the fields for the audio manager */
  self->priv->audiooutput_core->get_devices (devices);
  self->priv->sound_events_output =
    gm_pw_string_option_menu_new (container,
                                  _("Ringing Device"),
                                  gm_pw_get_device_choices (devices),
                                  self->priv->sound_events_settings,
                                  "output-device",
                                  _("Select the ringing audio device to use"));
  self->priv->audio_player =
    gm_pw_string_option_menu_new (container,
                                  _("Output Device"),
                                  gm_pw_get_device_choices (devices),
                                  self->priv->audio_devices_settings,
                                  "output-device",
                                  _("Select the audio output device to use"));

  /* The recorder */
  self->priv->audioinput_core->get_devices (devices);
  self->priv->audio_recorder =
    gm_pw_string_option_menu_new (container,
                                  _("Input Device"),
                                  gm_pw_get_device_choices (devices),
                                  self->priv->audio_devices_settings,
                                  "input-device",
                                  _("Select the audio input device to use"));

  /* That button will refresh the device list */
  gm_pw_add_update_button (container, _("_Detect devices"),
                           G_CALLBACK (refresh_devices_list_cb),
                           _("Click here to refresh the device list"),
                           self);
}


GtkWidget *
gm_pw_entry_new (GtkWidget *subsection,
                 const gchar *label_txt,
                 boost::shared_ptr<Ekiga::Settings> settings,
                 const std::string & key,
                 const gchar *tooltip,
                 G_GNUC_UNUSED bool indent)
{
  GtkWidget *entry = NULL;
  GtkWidget *label = NULL;

  int pos = 0;

  GTK_GRID_LAST_ROW (subsection, pos);

  label = gtk_label_new_with_mnemonic (label_txt);
  g_object_set (G_OBJECT (label),
                "margin-left", indent ? 12 : 0,
                "halign", GTK_ALIGN_END,
                NULL);
  gtk_grid_attach (GTK_GRID (subsection), label, 0, pos-1, 1, 1);

  entry = gm_entry_new (NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);
  gtk_grid_attach_next_to (GTK_GRID (subsection), entry, label, GTK_POS_RIGHT, 1, 1);

  g_settings_bind (settings->get_g_settings (), key.c_str (),
                   entry, "text", G_SETTINGS_BIND_DEFAULT);

  if (tooltip)
    gtk_widget_set_tooltip_text (entry, tooltip);

  gtk_widget_show_all (subsection);

  return entry;
}

GtkWidget *
gm_pw_string_option_menu_new (GtkWidget *subsection,
                              const gchar *label_txt,
                              const Choices & options,
                              boost::shared_ptr <Ekiga::Settings> settings,
                              const std::string & key,
                              const gchar *tooltip,
                              G_GNUC_UNUSED bool indent)
{
  GtkWidget *label = NULL;
  GtkWidget *option_menu = NULL;
  GList *cells = NULL;

  int pos = 0;

  GTK_GRID_LAST_ROW (subsection, pos);

  label = gtk_label_new_with_mnemonic (label_txt);
  g_object_set (G_OBJECT (label),
                "margin-left", indent ? 12 : 0,
                "halign", GTK_ALIGN_END,
                NULL);
  gtk_grid_attach (GTK_GRID (subsection), label, 0, pos-1, 1, 1);

  option_menu = gtk_combo_box_text_new ();
  cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (option_menu));
  g_object_set (G_OBJECT (cells->data), "ellipsize", PANGO_ELLIPSIZE_END, "width-chars", 20, NULL);
  g_list_free (cells);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), option_menu);
  for (Choices_const_iterator iter = options.begin ();
       iter != options.end ();
       ++iter)
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (option_menu),
                               boost::get<0>(*iter).c_str (),
                               boost::get<1>(*iter).c_str ());
  gtk_grid_attach_next_to (GTK_GRID (subsection), option_menu, label, GTK_POS_RIGHT, 1, 1);

  g_settings_bind (settings->get_g_settings (),
                   key.c_str (),
                   option_menu, "active-id",
                   G_SETTINGS_BIND_DEFAULT);

  if (tooltip)
    gtk_widget_set_tooltip_text (option_menu, tooltip);

  gtk_widget_show_all (subsection);

  return option_menu;
}


void
gm_pw_string_option_menu_update (GtkWidget *option_menu,
                                 const Choices & options,
                                 boost::shared_ptr<Ekiga::Settings> settings,
                                 const std::string & key)
{
  if (options.empty () || key.empty ())
    return;

  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (option_menu));

  for (Choices_const_iterator iter = options.begin ();
       iter != options.end ();
       ++iter)
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (option_menu),
                               boost::get<0>(*iter).c_str (),
                               boost::get<1>(*iter).c_str ());

  // We need to bind again after a remove_all operation
  g_settings_bind (settings->get_g_settings (),
                   key.c_str (),
                   option_menu, "active-id",
                   G_SETTINGS_BIND_DEFAULT);

  // Force the corresponding AudioInputCore/AudioOutputCore
  // to select the most appropriate device if we removed the currently used
  // device
  if (gtk_combo_box_get_active (GTK_COMBO_BOX (option_menu)) == -1)
    settings->set_string (key, ""); // Nothing selected
}


void
gm_pw_string_option_menu_add (GtkWidget *option_menu,
                              const Choice & option,
                              G_GNUC_UNUSED boost::shared_ptr <Ekiga::Settings> settings,
                              G_GNUC_UNUSED const std::string & key)
{
  if (boost::get<0>(option).empty () || boost::get<1>(option).empty ())
    return;

  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (option_menu),
                             boost::get<0>(option).c_str (),
                             boost::get<1>(option).c_str ());
}


void
gm_pw_string_option_menu_remove (GtkWidget *option_menu,
                                 const std::string & option,
                                 boost::shared_ptr <Ekiga::Settings> settings,
                                 const std::string & key)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (option_menu));
  GtkTreeIter iter;
  int pos = 0;
  gchar *s = NULL;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {
    do {
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 0, &s, -1);
      if (s && !strcmp (s, option.c_str ())) {
        g_free (s);
        break;
      }
      g_free (s);
      pos++;
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
  }
  gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (option_menu), pos);

  // Force the corresponding AudioInputCore/AudioOutputCore
  // to select the most appropriate device if we removed the currently used
  // device
  if (gtk_combo_box_get_active (GTK_COMBO_BOX (option_menu)) == -1)
    settings->set_string (key, ""); // Nothing selected
}


GtkWidget *
gm_pw_scale_new (GtkWidget* subsection,
                 const gchar *down_label_txt,
                 const gchar *up_label_txt,
                 boost::shared_ptr <Ekiga::Settings> settings,
                 const std::string & key,
                 const gchar *tooltip,
                 double min,
                 double max,
                 double step,
                 bool indent)
{
  GtkWidget *hbox = NULL;
  GtkAdjustment *adj = NULL;
  GtkWidget *label = NULL;
  GtkWidget *hscale = NULL;

  int pos = 0;

  GTK_GRID_LAST_ROW (subsection, pos);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  label = gtk_label_new_with_mnemonic (down_label_txt);
  g_object_set (G_OBJECT (label),
                "margin-left", indent ? 12 : 0,
                "halign", GTK_ALIGN_END,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (settings->get_int (key),
                        min, max, step,
                        2.0, 1.0);
  g_settings_bind (settings->get_g_settings (), key.c_str (),
                   adj, "value", G_SETTINGS_BIND_DEFAULT);

  hscale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adj);
  gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
  gtk_widget_set_size_request (GTK_WIDGET (hscale), 150, -1);
  gtk_box_pack_start (GTK_BOX (hbox), hscale, FALSE, FALSE, 2);

  label = gtk_label_new_with_mnemonic (up_label_txt);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

  gtk_grid_attach (GTK_GRID (subsection), hbox, 0, pos-1, 2, 1);

  if (tooltip)
    gtk_widget_set_tooltip_text (hscale, tooltip);

  gtk_widget_show_all (subsection);

  return hscale;
}


GtkWidget *
gm_pw_spin_new (GtkWidget* subsection,
                const gchar *label_txt,
                const gchar *label_txt2,
                boost::shared_ptr <Ekiga::Settings> settings,
                const std::string & key,
                const gchar *tooltip,
                double min,
                double max,
                double step,
                bool indent)
{
  GtkAdjustment *adj = NULL;
  GtkWidget *label = NULL;
  GtkWidget *spin_button = NULL;
  GtkWidget *hbox = NULL;

  int pos = 0;

  GTK_GRID_LAST_ROW (subsection, pos);

  label = gtk_label_new_with_mnemonic (label_txt);
  g_object_set (G_OBJECT (label),
                "margin-left", indent ? 12 : 0,
                "halign", GTK_ALIGN_END,
                NULL);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (settings->get_int (key),
                        min, max, step,
                        1.0, 1.0);
  spin_button = gtk_spin_button_new (adj, 1.0, 0);
  g_settings_bind (settings->get_g_settings (), key.c_str (),
                   adj, "value", G_SETTINGS_BIND_DEFAULT);

  if (label_txt2) {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

    gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 2);

    label = gtk_label_new_with_mnemonic (label_txt2);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

    gtk_grid_attach (GTK_GRID (subsection), hbox, 0, pos-1, 2, 1);
  }
  else {
    gtk_grid_attach (GTK_GRID (subsection), label, 0, pos-1, 1, 1);
    gtk_grid_attach_next_to (GTK_GRID (subsection), spin_button, label, GTK_POS_RIGHT, 1, 1);
  }

  if (tooltip)
    gtk_widget_set_tooltip_text (spin_button, tooltip);

  gtk_widget_show_all (subsection);

  return spin_button;
}


GtkWidget *
gm_pw_toggle_new (GtkWidget* subsection,
                  const gchar *label_txt,
                  boost::shared_ptr <Ekiga::Settings> settings,
                  const std::string & key,
                  const gchar *tooltip,
                  G_GNUC_UNUSED bool indent)
{
  GtkWidget *toggle = NULL;

  int pos = 0;

  GTK_GRID_LAST_ROW (subsection, pos);

  toggle = gtk_check_button_new_with_mnemonic (label_txt);
  if (indent)
    g_object_set (G_OBJECT (toggle), "margin-left", 12, NULL);
  g_settings_bind (settings->get_g_settings (), key.c_str (),
                   toggle, "active", G_SETTINGS_BIND_DEFAULT);
  gtk_grid_attach (GTK_GRID (subsection), toggle, 0, pos-1, 2, 1);

  if (tooltip)
    gtk_widget_set_tooltip_text (toggle, tooltip);

  gtk_widget_show_all (subsection);

  return toggle;
}


GtkWidget *
gm_pw_window_subsection_new (PreferencesWindow *self,
                             const gchar *section_name)
{
  GtkWidget *container = NULL;

  if (!self)
    return NULL;

  if (!section_name)
    return NULL;

  container = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (container), 12);
  gtk_grid_set_row_spacing (GTK_GRID (container), 6);
  gtk_container_set_border_width (GTK_CONTAINER (container), 18);

  gtk_grid_set_column_homogeneous (GTK_GRID (container), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (container), FALSE);

  gtk_stack_add_titled (GTK_STACK (self->priv->stack),
                        container, section_name, section_name);

  gtk_widget_show_all (container);

  return container;
}

void
gm_pw_subsection_new (GtkWidget *container,
                      const gchar *name)
{
  GtkWidget *label = NULL;
  gchar *label_txt = NULL;
  int pos = 0;

  GTK_GRID_LAST_ROW (container, pos);

  label = gtk_label_new (NULL);
  label_txt = g_strdup_printf ("<b>%s</b>", name);
  gtk_label_set_markup (GTK_LABEL (label), label_txt);
  g_object_set (G_OBJECT (label),
                "margin-top", 12, // We have 6 pixels between each row
                                  // and the HIG asks for 18 pixels between
                                  // subsections
                "halign", GTK_ALIGN_START, NULL);
  gtk_grid_attach (GTK_GRID (container), label, 0, pos-1, 2, 1);
  g_free (label_txt);
}

static Choices
gm_pw_get_device_choices (const std::vector<std::string> & v)
{
  Choices c;

  if (v.size () == 0)
    c.push_back (boost::make_tuple ("none", _("No device found")));
  else for (std::vector<std::string>::const_iterator iter = v.begin ();
            iter != v.end ();
            ++iter)
    c.push_back (boost::make_tuple (*iter, *iter));

  return c;
}

static void
gm_prefs_window_update_devices_list (PreferencesWindow *self)
{
  g_return_if_fail (self != NULL);

  std::vector<std::string> devices;

  /* The player */
  self->priv->audiooutput_core->get_devices (devices);
  gm_pw_string_option_menu_update (self->priv->audio_player,
                                   gm_pw_get_device_choices (devices),
                                   self->priv->audio_devices_settings,
                                   "output-device");
  gm_pw_string_option_menu_update (self->priv->sound_events_output,
                                   gm_pw_get_device_choices (devices),
                                   self->priv->sound_events_settings,
                                   "output-device");

  /* The recorder */
  self->priv->audioinput_core->get_devices (devices);
  gm_pw_string_option_menu_update (self->priv->audio_recorder,
                                   gm_pw_get_device_choices (devices),
                                   self->priv->audio_devices_settings,
                                   "input-device");
}



/* Callbacks */
static void
refresh_devices_list_cb (G_GNUC_UNUSED GtkWidget *widget,
                         gpointer data)
{
  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  gm_prefs_window_update_devices_list (self);
}


static void
sound_event_changed_cb (GtkWidget *b,
                        gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeIter iter;

  gchar *filename = NULL;
  gchar *key = NULL;
  std::string sound_event;

  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sound_events_list));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                        2, &key, -1);

    if (key) {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (b));
      if (filename) {
        sound_event = self->priv->sound_events_settings->get_string (key);

        if (sound_event.empty () || g_strcmp0 (filename, sound_event.c_str ()))
          self->priv->sound_events_settings->set_string (key, filename);

        g_free (filename);
      }

      g_free (key);
    }
  }
}


static void
sound_event_setting_changed (G_GNUC_UNUSED GSettings *settings,
                             gchar *key,
                             gpointer data)
{
  bool valid = true;

  GtkTreeIter iter;

  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->priv->sound_events_list));

  /* Get the first iter in the list, check it is valid and walk
   *  * through the list, reading each row. */
  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter)) {

    gchar *str = NULL;
    gtk_tree_model_get (model, &iter, 3, &str, -1);

    if (key && str && !g_strcmp0 (key, str)) {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          0, self->priv->sound_events_settings->get_bool (key));
      break;
    }

    g_free (str);
  }
}


static void
sound_event_selected_cb (GtkTreeSelection *selection,
                         gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreeIter iter;

  gchar *key = NULL;
  gchar *filename = NULL;

  std::string sound_event;

  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 2, &key, -1);

    if (key) {

      sound_event = self->priv->sound_events_settings->get_string (key);

      if (!sound_event.empty ()) {

        if (!g_path_is_absolute (sound_event.c_str ()))
          filename = g_build_filename (DATA_DIR, "sounds", PACKAGE_NAME,
                                       sound_event.c_str (), NULL);
        else
          filename = g_strdup (sound_event.c_str ());

        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (self->priv->fsbutton), filename);
        g_free (filename);
      }

      g_free (key);
    }
  }
}


static void
sound_event_play_cb (G_GNUC_UNUSED GtkWidget *widget,
                     gpointer data)
{
  GtkTreeSelection *selection = NULL;
  GtkTreeModel *model = NULL;
  GtkTreeIter selected_iter;

  gchar *key = NULL;

  std::string sound_event;

  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->sound_events_list));

  if (gtk_tree_selection_get_selected (selection, &model, &selected_iter)) {

    gtk_tree_model_get (GTK_TREE_MODEL (model), &selected_iter, 2, &key, -1);

    sound_event = self->priv->sound_events_settings->get_string (key);
    if (!sound_event.empty ()) {
      if (!g_path_is_absolute (sound_event.c_str ()))
        self->priv->audiooutput_core->play_event (sound_event);
      else
        self->priv->audiooutput_core->play_file (sound_event);
    }

    g_free (key);
  }
}


static void
sound_event_toggled_cb (G_GNUC_UNUSED GtkCellRendererToggle *cell,
                        gchar *path_str,
                        gpointer data)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;

  gchar *key = NULL;

  bool fixed = FALSE;

  g_return_if_fail (data != NULL);
  PreferencesWindow *self = PREFERENCES_WINDOW (data);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->priv->sound_events_list));
  path = gtk_tree_path_new_from_string (path_str);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 0, &fixed, 3, &key, -1);

  fixed ^= 1;

  self->priv->sound_events_settings->set_bool (key, fixed);

  g_free (key);
  gtk_tree_path_free (path);
}


void on_audioinput_device_added_cb (const Ekiga::AudioInputDevice & device,
                                    PreferencesWindow *self)
{
  g_return_if_fail (self != NULL);

  gm_pw_string_option_menu_add (self->priv->audio_recorder,
                                boost::make_tuple (device.GetString (), device.GetString ()),
                                self->priv->audio_devices_settings, "input-device");
}

void on_audioinput_device_removed_cb (const Ekiga::AudioInputDevice & device,
                                      bool,
                                      PreferencesWindow *self)
{
  g_return_if_fail (self != NULL);

  gm_pw_string_option_menu_remove (self->priv->audio_recorder, device.GetString(),
                                   self->priv->audio_devices_settings, "input-device");
}

void on_audiooutput_device_added_cb (const Ekiga::AudioOutputDevice & device,
                                     PreferencesWindow *self)
{
  g_return_if_fail (self != NULL);

  gm_pw_string_option_menu_add (self->priv->audio_player,
                                boost::make_tuple (device.GetString (), device.GetString ()),
                                self->priv->audio_devices_settings, "output-device");
  gm_pw_string_option_menu_add (self->priv->sound_events_output,
                                boost::make_tuple (device.GetString (), device.GetString ()),
                                self->priv->sound_events_settings, "output-device");
}

void on_audiooutput_device_removed_cb (const Ekiga::AudioOutputDevice & device,
                                       bool,
                                       PreferencesWindow *self)
{
  g_return_if_fail (self != NULL);

  gm_pw_string_option_menu_remove (self->priv->audio_player, device.GetString(),
                                   self->priv->audio_devices_settings, "output-device");
  gm_pw_string_option_menu_remove (self->priv->sound_events_output, device.GetString(),
                                   self->priv->sound_events_settings, "output-device");
}


/* Implementation of the GObject stuff */
static void
preferences_window_finalize (GObject *obj)
{
  PreferencesWindow *self = PREFERENCES_WINDOW (obj);

  delete self->priv;

  G_OBJECT_CLASS (preferences_window_parent_class)->finalize (obj);
}


static void
preferences_window_init (G_GNUC_UNUSED PreferencesWindow* self)
{
  /* can't do anything here... we're waiting for a core :-/ */
}


static void
preferences_window_class_init (PreferencesWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = preferences_window_finalize;
}


/* Public functions */
GtkWidget *
preferences_window_new (GmApplication *app)
{
  PreferencesWindow *self = NULL;

  g_return_val_if_fail (GM_IS_APPLICATION (app), NULL);

  GtkWidget *container = NULL;
  GtkWidget *box = NULL;
  GtkWidget *stack_sidebar = NULL;
  GtkWidget *headerbar = NULL;

  boost::signals2::connection conn;

  Ekiga::ServiceCore& core = gm_application_get_core (app);

  /* The window */
  self = (PreferencesWindow *) g_object_new (PREFERENCES_WINDOW_TYPE,
                                             "application", GTK_APPLICATION (app),
                                             "key", USER_INTERFACE ".preferences-window",
                                             "hide_on_delete", false,
                                             "hide_on_esc", false, NULL);

  self->priv = new PreferencesWindowPrivate ();
  self->priv->audioinput_core = core.get<Ekiga::AudioInputCore> ("audioinput-core");
  self->priv->audiooutput_core = core.get<Ekiga::AudioOutputCore> ("audiooutput-core");
  self->priv->app = app;

  headerbar = gtk_header_bar_new ();
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), TRUE);
  gtk_header_bar_set_title (GTK_HEADER_BAR (headerbar), _("Preferences"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (headerbar), _("Edit your settings"));
  gtk_window_set_titlebar (GTK_WINDOW (self), headerbar);
  gtk_window_set_icon_name (GTK_WINDOW (self), PACKAGE_NAME);
  gtk_widget_show (headerbar);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (self), box);

  stack_sidebar = gtk_stack_sidebar_new ();
  gtk_box_pack_start (GTK_BOX (box), stack_sidebar, TRUE, TRUE, 0);

  self->priv->stack = gtk_stack_new ();
  gtk_stack_sidebar_set_stack (GTK_STACK_SIDEBAR (stack_sidebar), GTK_STACK (self->priv->stack));
  gtk_box_pack_start (GTK_BOX (box), self->priv->stack, TRUE, TRUE, 0);

  gtk_widget_show_all (GTK_WIDGET (box));

  /* Stuff */
  container = gm_pw_window_subsection_new (self,
                                           _("General"));
  gm_pw_init_general_page (self, container);
  gtk_widget_show_all (GTK_WIDGET (container));

  container = gm_pw_window_subsection_new (self, _("Call Handling"));
  gm_pw_init_call_options_page (self, container);
  gtk_widget_show_all (GTK_WIDGET (container));

  container = gm_pw_window_subsection_new (self,
                                           _("Sound Events"));
  gm_pw_init_sound_events_page (self, container);
  gtk_widget_show_all (GTK_WIDGET (container));

  container = gm_pw_window_subsection_new (self,
                                           _("SIP"));
  gm_pw_init_sip_page (self, container);
  gtk_widget_show_all (GTK_WIDGET (container));

  container = gm_pw_window_subsection_new (self, _("Audio"));
  gm_pw_init_audio_page (self, container);
  gtk_widget_show_all (GTK_WIDGET (container));


  /* Boost Signals */
  conn =
    self->priv->audioinput_core->device_added.connect (boost::bind (&on_audioinput_device_added_cb,
                                                                    _1,
                                                                    self));
  self->priv->connections.add (conn);
  conn =
    self->priv->audioinput_core->device_removed.connect (boost::bind (&on_audioinput_device_removed_cb,
                                                                      _1,
                                                                      _2,
                                                                      self));
  self->priv->connections.add (conn);

  conn =
    self->priv->audiooutput_core->device_added.connect (boost::bind (&on_audiooutput_device_added_cb,
                                                                     _1,
                                                                     self));
  self->priv->connections.add(conn);
  conn =
    self->priv->audiooutput_core->device_removed.connect (boost::bind (&on_audiooutput_device_removed_cb,
                                                                       _1,
                                                                       _2,
                                                                       self));
  self->priv->connections.add (conn);

  /* Connect notifiers for SOUND_EVENTS_SCHEMA settings */
  g_signal_connect (self->priv->sound_events_settings->get_g_settings (),
                    "changed::enable-incoming-call-sound",
                    G_CALLBACK (sound_event_setting_changed), self);

  g_signal_connect (self->priv->sound_events_settings->get_g_settings (),
                    "changed::enable-ring-tone-sound",
                    G_CALLBACK (sound_event_setting_changed), self);

  g_signal_connect (self->priv->sound_events_settings->get_g_settings (),
                    "changed::enable-busy-tone-sound",
                    G_CALLBACK (sound_event_setting_changed), self);

  g_signal_connect (self->priv->sound_events_settings->get_g_settings (),
                    "changed::enable-new-voicemail-sound",
                    G_CALLBACK (sound_event_setting_changed), self);

  g_signal_connect (self->priv->sound_events_settings->get_g_settings (),
                    "changed::enable-new-message-sound",
                    G_CALLBACK (sound_event_setting_changed), self);

  return GTK_WIDGET (self);
}
