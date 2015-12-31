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
 *                         ekiga-app.cpp  -  description
 *                         -----------------------------
 *   begin                : written in Feb 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : main Ekiga GtkApplication
 *
 */

#include <gtk/gtk.h>

#include "config.h"
#include "ekiga-settings.h"
#include "revision.h"

#include "form-dialog-gtk.h"
#include "ekiga-app.h"
#include "account-core.h"
#include "contact-core.h"
#include "presence-core.h"
#include "preferences-window.h"
#include "call-window.h"
#include "ekiga-window.h"
#include "statusicon.h"
#include "history-source.h"
#include "opal-bank.h"
#include "book-view-gtk.h"
#include "notification-core.h"
#include "personal-details.h"
#include "audioinput-core.h"
#include "audiooutput-core.h"
#include "call-core.h"
#include "engine.h"
#include "runtime.h"
#include "platform/platform.h"
#include "gactor-menu.h"

#include "gmwindow.h"

#ifdef WIN32
#include "platform/winpaths.h"
#include <windows.h>
#include <shellapi.h>
#include <gdk/gdkwin32.h>
#include <cstdio>
#else
#include <signal.h>
#include <gdk/gdkx.h>
#endif

#ifdef HAVE_DBUS
#include "../../../../src/dbus-helper/dbus.h"
#endif

#include <glib/gi18n.h>
#include <ptlib.h>

#include "scoped-connections.h"


/*
 * The GmApplication
 */
struct _GmApplicationPrivate
{
  Ekiga::ServiceCore core;

  GtkBuilder *builder;
  GtkWidget *ekiga_window;
  GtkWidget *call_window;

#ifdef HAVE_DBUS
  EkigaDBusComponent *dbus_component;
#endif

  Ekiga::GActorMenuStore banks_menu;
  Ekiga::GActorMenuPtr fof_menu;
  unsigned int banks_actions_count;

  Ekiga::scoped_connections conns;
};

G_DEFINE_TYPE (GmApplication, gm_application, GTK_TYPE_APPLICATION);

static GtkWidget *gm_application_show_call_window (GmApplication *self);

static void on_setup_call_cb (boost::shared_ptr<Ekiga::Call> call,
                              gpointer data);

static bool on_handle_questions_cb (Ekiga::FormRequestPtr request,
                                    GmApplication *application);

static void call_window_destroyed_cb (GtkWidget *widget,
                                      gpointer data);

static void engine_call_uri_action_cb (GSimpleAction *simple,
                                       GVariant *parameter,
                                       gpointer data);

static void account_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer app);

static void quit_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer app);

static void window_activated (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer app);

static GActionEntry app_entries[] =
{
    { "account", account_activated, NULL, NULL, NULL, 0 },
    { "preferences", window_activated, NULL, NULL, NULL, 0 },
    { "quit", quit_activated, NULL, NULL, NULL, 0 }
};


/* Private helpers */
static GtkWidget *
gm_application_show_call_window (GmApplication *self)
{
  g_return_val_if_fail (GM_IS_APPLICATION (self), NULL);

  if (!self->priv->call_window)
    self->priv->call_window = call_window_new (self);

  call_window_present (EKIGA_CALL_WINDOW (self->priv->call_window));

  g_signal_connect (G_OBJECT (self->priv->call_window), "destroy",
                    G_CALLBACK (call_window_destroyed_cb), self);

  return self->priv->call_window;
}


/* Private callbacks */
static void
on_setup_call_cb (boost::shared_ptr<Ekiga::Call> call,
                  gpointer data)
{
  g_return_if_fail (GM_IS_APPLICATION (data));

  GmApplication *self = GM_APPLICATION (data);

  GtkWidget *call_window = gm_application_show_call_window (self);
  call_window_add_call (call_window, call);
}


static bool
on_handle_questions_cb (Ekiga::FormRequestPtr request,
                        GmApplication *application)
{
  GtkWidget *window =
    GTK_WIDGET (gtk_application_get_active_window (GTK_APPLICATION (application)));
  FormDialog dialog (request, window);

  dialog.run ();

  return true;
}


static void
call_window_destroyed_cb (G_GNUC_UNUSED GtkWidget *widget,
                          gpointer data)
{
  g_return_if_fail (GM_IS_APPLICATION (data));

  GmApplication *self = GM_APPLICATION (data);
  if (self->priv->call_window)
    self->priv->call_window = NULL;
}


static void
engine_call_uri_action_cb (G_GNUC_UNUSED GSimpleAction *simple,
                           GVariant *parameter,
                           gpointer data)
{
  g_return_if_fail (GM_IS_APPLICATION (data));

  const gchar *url = g_variant_get_string (parameter, NULL);
  GmApplication *self = GM_APPLICATION (data);
  boost::shared_ptr<Ekiga::CallCore> call_core = self->priv->core.get<Ekiga::CallCore> ("call-core");
  call_core->dial (url);
}


static void
quit_activated (G_GNUC_UNUSED GSimpleAction *action,
                G_GNUC_UNUSED GVariant *parameter,
                gpointer app)
{
  g_return_if_fail (GM_IS_APPLICATION (app));

  GmApplication *self = GM_APPLICATION (app);
  if (self->priv->ekiga_window)
    ekiga_window_quit_requested (self->priv->ekiga_window);
  else
    g_application_quit (G_APPLICATION (app));
}


static void
window_activated (GSimpleAction *action,
                  G_GNUC_UNUSED GVariant *parameter,
                  gpointer app)
{
  GmApplication *self = GM_APPLICATION (app);

  g_return_if_fail (self);

  if (!g_strcmp0 (g_action_get_name (G_ACTION (action)), "preferences"))
    gm_application_show_preferences_window (self);
}


static void
account_activated (GSimpleAction *action,
                  G_GNUC_UNUSED GVariant *parameter,
                  gpointer app)
{
  GmApplication *self = GM_APPLICATION (app);

  g_return_if_fail (self);

  if (!g_strcmp0 (g_action_get_name (G_ACTION (action)), "account")) {
    boost::shared_ptr<Opal::Bank> b = boost::shared_ptr<Opal::Bank> (self->priv->core.get<Opal::Bank> ("opal-account-store"));
    b->edit_account();
  }
}


/* Public api */
void
ekiga_main (int argc,
            char **argv)
{
  GmApplication *app = gm_application_new ();

  g_application_set_inactivity_timeout (G_APPLICATION (app), 10000);

  if (g_application_get_is_remote (G_APPLICATION (app))) {
    g_application_run (G_APPLICATION (app), argc, argv);
    return;
  }

  Ekiga::Runtime::init ();
  engine_init (app->priv->core, argc, argv);

  // Connect signals
  {
    boost::shared_ptr<Ekiga::CallCore> call_core = app->priv->core.get<Ekiga::CallCore> ("call-core");
    call_core->setup_call.connect (boost::bind (&on_setup_call_cb, _1, (gpointer) app));

    boost::shared_ptr<Ekiga::AccountCore> account_core = app->priv->core.get<Ekiga::AccountCore> ("account-core");
    app->priv->conns.add (account_core->questions.connect (boost::bind (&on_handle_questions_cb, _1, app)));
  }

  /* Create the main application window */
  app->priv->ekiga_window = gm_ekiga_window_new (app);
  gm_application_show_ekiga_window (app);

  status_icon_new (app);

#ifdef HAVE_DBUS
  app->priv->dbus_component = ekiga_dbus_component_new (app);
#endif

  boost::shared_ptr<Ekiga::Settings> general_settings (new Ekiga::Settings (GENERAL_SCHEMA));
  const int schema_version = MAJOR_VERSION * 1000 + MINOR_VERSION * 10 + BUILD_NUMBER;
  if (general_settings->get_int ("version") < schema_version) {
    general_settings->set_int ("version", schema_version);
  }

  g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);
}


/* GObject stuff */
static void
gm_application_activate (GApplication *self)
{
  GmApplication *app = GM_APPLICATION (self);

  gm_application_show_ekiga_window (app);
}

static void
gm_application_startup (GApplication *app)
{
  GmApplication *self = GM_APPLICATION (app);
  GVariantType *type_string = NULL;
  GSimpleAction *action = NULL;
  gchar *path = NULL;

  G_APPLICATION_CLASS (gm_application_parent_class)->startup (app);

#ifndef WIN32
  signal (SIGPIPE, SIG_IGN);
#endif

  /* initialize platform-specific code */
  gm_platform_init ();
#ifdef WIN32
  // plugins (i.e. the audio ptlib/opal codecs) are searched in ./plugins
  chdir (win32_datadir ());
#endif

  /* Gettext initialization */
  path = g_build_filename (DATA_DIR, "locale", NULL);
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, path);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  g_free (path);

  /* Application name */
  g_set_application_name (_("Ekiga Softphone"));
#ifndef WIN32
  setenv ("PULSE_PROP_application.name", _("Ekiga Softphone"), true);
  setenv ("PA_PROP_MEDIA_ROLE", "phone", true);
#endif

  /* Priv building */
  self->priv->builder = gtk_builder_new ();

  /* Menu */
  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   self);

  self->priv->call_window = NULL;

  // We add DBUS specific actions, based on the Engine actions
  type_string = g_variant_type_new ("s");
  action = g_simple_action_new ("call-uri", type_string);
  g_signal_connect (action, "activate", G_CALLBACK (engine_call_uri_action_cb), self);
  g_action_map_add_action (G_ACTION_MAP (g_application_get_default ()),
                           G_ACTION (action));
  g_variant_type_free (type_string);
  g_object_unref (action);
}


static void
gm_application_shutdown (GApplication *app)
{
  GmApplication *self = GM_APPLICATION (app);

  g_return_if_fail (self);

  self->priv->fof_menu.reset ();
  self->priv->banks_menu.clear ();
  Ekiga::Runtime::quit ();

  gm_platform_shutdown ();

  /* Destroy all windows to make sure the UI is gone
   * and we do not block the ServiceCore from
   * destruction.
   */
  while (GList *windows = gtk_application_get_windows (GTK_APPLICATION (self))) {
    GList *windows_it = g_list_first (windows);
    if (windows_it->data && GTK_IS_WIDGET (windows_it->data))
      gtk_widget_destroy (GTK_WIDGET (windows_it->data));
    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  engine_close (self->priv->core);

#ifdef HAVE_DBUS
  g_object_unref (self->priv->dbus_component);
#endif
  g_object_unref (self->priv->builder);

  delete self->priv;
  self->priv = NULL;

  G_APPLICATION_CLASS (gm_application_parent_class)->shutdown (app);
}


static gint
gm_application_handle_local_options (GApplication *app,
                                     GVariantDict *options)
{
  GmApplication *self = GM_APPLICATION (app);
  GVariant *value = NULL;

  g_return_val_if_fail (self, -1);

  value = g_variant_dict_lookup_value (options, "call", G_VARIANT_TYPE_STRING);
  if (value) {
    g_action_group_activate_action (G_ACTION_GROUP (app), "call-uri", value);
    g_variant_unref (value);
    return 0;
  }
  else if (g_variant_dict_contains (options, "hangup")) {
    g_action_group_activate_action (G_ACTION_GROUP (app), "hangup", NULL);
    return 0;
  }
  else if (g_variant_dict_contains (options, "version")) {
    g_print ("%s - Version %d.%d.%d\n",
             g_get_application_name (),
             MAJOR_VERSION, MINOR_VERSION, BUILD_NUMBER);
    g_application_quit (app);
    return 0;
  }

  g_application_activate (app);

  return -1;
}


static void
gm_application_class_init (GmApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  app_class->startup = gm_application_startup;
  app_class->activate = gm_application_activate;
  app_class->shutdown = gm_application_shutdown;
  app_class->handle_local_options = gm_application_handle_local_options;
}


static void
gm_application_init (G_GNUC_UNUSED GmApplication *self)
{
  self->priv = new GmApplicationPrivate ();
  self->priv->banks_actions_count = 0;

  static GOptionEntry options [] =
    {
        {
          "help", '?', 0, G_OPTION_ARG_NONE, NULL,
          N_("Show the application's help"), NULL
        },

        /* Version */
        {
          "version", 'V', 0, G_OPTION_ARG_NONE, NULL,
          N_("Show the application's version"), NULL
        },
        {
          "debug", 'd', 0, G_OPTION_ARG_INT, NULL,
          N_("Prints debug messages in the console (level between 1 and 8)"),
          NULL
        },
        {
          "call", 'c', 0, G_OPTION_ARG_STRING, NULL,
          N_("Makes Ekiga call the given URI"),
          NULL
        },
        {
          "hangup", '\0', 0, G_OPTION_ARG_NONE, NULL,
          N_("Hangup the current call (if any)"),
          NULL
        },
        {
          NULL, 0, 0, (GOptionArg)0, NULL,
          NULL,
          NULL
        }
    };

  g_application_add_main_option_entries (G_APPLICATION (self), options);
}


GmApplication *
gm_application_new ()
{
  GmApplication *self =
    GM_APPLICATION (g_object_new (GM_TYPE_APPLICATION,
                                  "application-id", "org.gnome.ekiga",
                                  NULL));

  g_application_register (G_APPLICATION (self), NULL, NULL);

  return self;
}


Ekiga::ServiceCore&
gm_application_get_core (GmApplication *self)
{
  return self->priv->core;
}


void
gm_application_show_ekiga_window (GmApplication *self)
{
  g_return_if_fail (GM_IS_APPLICATION (self));

  gtk_window_present (GTK_WINDOW (self->priv->ekiga_window));
}


void
gm_application_hide_ekiga_window (GmApplication *self)
{
  g_return_if_fail (GM_IS_APPLICATION (self));

  gtk_widget_hide (self->priv->ekiga_window);
}


GtkWidget *
gm_application_get_ekiga_window (GmApplication *self)
{
  g_return_val_if_fail (GM_IS_APPLICATION (self), NULL);

  return self->priv->ekiga_window;
}


void
gm_application_show_preferences_window (GmApplication *self)
{
  GtkWindow *parent = NULL;
  GtkWindow *window = NULL;

  g_return_if_fail (GM_IS_APPLICATION (self));

  parent = gtk_application_get_active_window (GTK_APPLICATION (self));

  window = GTK_WINDOW (preferences_window_new (self));
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
  gtk_window_present (window);
}
