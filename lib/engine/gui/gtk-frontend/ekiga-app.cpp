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
#include "addressbook-window.h"
#include "assistant-window.h"
#include "preferences-window.h"
#include "call-window.h"
#include "ekiga-window.h"
#include "statusicon.h"
#include "roster-view-gtk.h"
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
#define WIN32_HELP_DIR "help"
#define WIN32_HELP_FILE "index.html"
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

static void gm_application_populate_application_menu (GmApplication *app);

static GtkWidget *gm_application_show_call_window (GmApplication *self);

static void on_setup_call_cb (boost::shared_ptr<Ekiga::Call> call,
                              gpointer data);

static bool on_visit_banks_cb (Ekiga::BankPtr bank,
                               gpointer data);

static bool on_handle_questions_cb (Ekiga::FormRequestPtr request,
                                    GmApplication *application);

static void on_account_modified_cb (Ekiga::AccountPtr account,
                                    GmApplication *app);

static void call_window_destroyed_cb (GtkWidget *widget,
                                      gpointer data);

static void engine_call_uri_action_cb (GSimpleAction *simple,
                                       GVariant *parameter,
                                       gpointer data);

static void quit_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer app);

static void about_activated (GSimpleAction *action,
                             GVariant *parameter,
                             gpointer app);

static void help_activated (GSimpleAction *action,
                            GVariant *parameter,
                            gpointer app);

static void window_activated (GSimpleAction *action,
                              GVariant *parameter,
                              gpointer app);

static GActionEntry app_entries[] =
{
    { "preferences", window_activated, NULL, NULL, NULL, 0 },
    { "addressbook", window_activated, NULL, NULL, NULL, 0 },
    { "help", help_activated, NULL, NULL, NULL, 0 },
    { "about", about_activated, NULL, NULL, NULL, 0 },
    { "quit", quit_activated, NULL, NULL, NULL, 0 }
};


/* Private helpers */
static void
gm_application_populate_application_menu (GmApplication *app)
{
  g_return_if_fail (GM_IS_APPLICATION (app));
  GMenuModel *app_menu = G_MENU_MODEL (gtk_builder_get_object (app->priv->builder, "appmenu"));

  boost::shared_ptr<Ekiga::AccountCore> account_core
    = app->priv->core.get<Ekiga::AccountCore> ("account-core");
  g_return_if_fail (account_core);

  for (int i = app->priv->banks_menu.size () ;
       i > 0 ;
       i--)
    g_menu_remove (G_MENU (app_menu), 0);

  app->priv->banks_menu.clear ();
  app->priv->banks_actions_count = 0;
  account_core->visit_banks (boost::bind (&on_visit_banks_cb, _1, (gpointer) app));

  for (std::list<Ekiga::GActorMenuPtr>::iterator it = app->priv->banks_menu.begin ();
       it != app->priv->banks_menu.end ();
       it++) {
    g_menu_insert_section (G_MENU (app_menu), 0, NULL, (*it)->get_model ());
    app->priv->banks_actions_count += (*it)->size ();
  }
}


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
on_visit_banks_cb (Ekiga::BankPtr bank,
                   gpointer data)
{
  g_return_val_if_fail (GM_IS_APPLICATION (data), false);

  GmApplication *self = GM_APPLICATION (data);

  self->priv->banks_menu.push_back (Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*bank, "", "app")));
  self->priv->conns.add (bank->account_added.connect (boost::bind (&on_account_modified_cb, _1, self)));
  self->priv->conns.add (bank->account_removed.connect (boost::bind (&on_account_modified_cb, _1, self)));

  return true;
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
on_account_modified_cb (G_GNUC_UNUSED Ekiga::AccountPtr account,
                        GmApplication *app)
{
  g_return_if_fail (GM_IS_APPLICATION (app));

  gm_application_populate_application_menu (app);
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
  g_application_quit (G_APPLICATION (app));
}


static void
about_activated (G_GNUC_UNUSED GSimpleAction *action,
                 G_GNUC_UNUSED GVariant *parameter,
                 gpointer app)
{
  gm_application_show_about (GM_APPLICATION (app));
}


static void
help_activated (G_GNUC_UNUSED GSimpleAction *action,
                G_GNUC_UNUSED GVariant *parameter,
                gpointer app)
{
  gm_application_show_help (GM_APPLICATION (app), NULL);
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

  else if (!g_strcmp0 (g_action_get_name (G_ACTION (action)), "addressbook"))
    gm_application_show_addressbook_window (self);
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

    boost::shared_ptr<Ekiga::FriendOrFoe> friend_or_foe = app->priv->core.get<Ekiga::FriendOrFoe> ("friend-or-foe");
    app->priv->conns.add (friend_or_foe->questions.connect (boost::bind (&on_handle_questions_cb, _1, app)));
    // Persistent FriendOrFoe menu
    app->priv->fof_menu = Ekiga::GActorMenuPtr (new Ekiga::GActorMenu (*friend_or_foe));
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
    gm_application_show_assistant_window (app);
    general_settings->set_int ("version", schema_version);
  }

  gm_application_populate_application_menu (app);

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
  GMenuModel *app_menu = NULL;
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
  gtk_builder_add_from_string (self->priv->builder,
                               "<?xml version=\"1.0\"?>"
                               "<interface>"
                               "  <menu id=\"appmenu\">"
                               "    <section id=\"banks\">"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">Edit _Blacklist</attribute>"
                               "        <attribute name=\"action\">app.blacklist-edit</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">Address _Book</attribute>"
                               "        <attribute name=\"action\">app.addressbook</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">_Preferences</attribute>"
                               "        <attribute name=\"action\">app.preferences</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">_Help</attribute>"
                               "        <attribute name=\"action\">app.help</attribute>"
                               "        <attribute name=\"accel\">F1</attribute>"
                               "      </item>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">_About</attribute>"
                               "        <attribute name=\"action\">app.about</attribute>"
                               "      </item>"
                               "    </section>"
                               "    <section>"
                               "      <item>"
                               "        <attribute name=\"label\" translatable=\"yes\">_Quit</attribute>"
                               "        <attribute name=\"action\">app.quit</attribute>"
                               "        <attribute name=\"accel\">&lt;Primary&gt;q</attribute>"
                               "      </item>"
                               "    </section>"
                               "  </menu>"
                               "</interface>", -1, NULL);

  app_menu = G_MENU_MODEL (gtk_builder_get_object (self->priv->builder, "appmenu"));
  gtk_application_set_app_menu (GTK_APPLICATION (self), app_menu);

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


gboolean
gm_application_show_help (GmApplication *app,
                          G_GNUC_UNUSED const gchar *link_id)
{
  g_return_val_if_fail (GM_IS_APPLICATION (app), FALSE);

  GtkWindow *parent = gtk_application_get_active_window (GTK_APPLICATION (app));

#ifdef WIN32
  gchar *locale, *loc_ , *index_path;
  int hinst = 0;

  locale = g_win32_getlocale ();
  if (strlen (locale) > 0) {

    /* try returned locale first, it may be fully qualified e.g. zh_CN */
    index_path = g_build_filename (WIN32_HELP_DIR, locale,
                                   WIN32_HELP_FILE, NULL);
    hinst = (int) ShellExecute (NULL, "open", index_path, NULL,
                                DATA_DIR, SW_SHOWNORMAL);
    g_free (index_path);
  }

  if (hinst <= 32 && (loc_ = g_strrstr (locale, "_"))) {
    /* on error, try short locale */
    *loc_ = 0;
    index_path = g_build_filename (WIN32_HELP_DIR, locale,
                                   WIN32_HELP_FILE, NULL);
    hinst = (int) ShellExecute (NULL, "open", index_path, NULL,
                                DATA_DIR, SW_SHOWNORMAL);
    g_free (index_path);
  }

  g_free (locale);

  if (hinst <= 32) {

    /* on error or missing locale, try default locale */
    index_path = g_build_filename (WIN32_HELP_DIR, "C", WIN32_HELP_FILE, NULL);
    (void)ShellExecute (NULL, "open", index_path, NULL,
                        DATA_DIR, SW_SHOWNORMAL);
    g_free (index_path);
  }
#else /* !WIN32 */
  GError *err = NULL;
  gboolean success = FALSE;

  success = gtk_show_uri (NULL, "ghelp:" PACKAGE_NAME, GDK_CURRENT_TIME, &err);

  if (!success) {
    GtkWidget *d;
    d = gtk_message_dialog_new (NULL,
                                (GtkDialogFlags) (GTK_DIALOG_MODAL),
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                "%s", _("Unable to open help file."));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
                                              "%s", err->message);
    g_signal_connect (d, "response", G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_window_set_transient_for (GTK_WINDOW (d), GTK_WINDOW (parent));
    gtk_window_present (GTK_WINDOW (d));
    g_error_free (err);
    return FALSE;
  }
#endif

  return TRUE;
}


void
gm_application_show_about (GmApplication *app)
{
  g_return_if_fail (GM_IS_APPLICATION (app));

  GtkWidget *pixmap = NULL;
  gchar *filename = NULL;

  const gchar *authors [] = {
      "Damien Sandras <dsandras@seconix.com>",
      "",
      N_("Contributors:"),
      "Eugen Dedu <eugen.dedu@univ-fcomte.fr>",
      "Julien Puydt <julien.puydt@laposte.net>",
      "Robert Jongbloed <rjongbloed@postincrement.com>",
      "",
      N_("Artwork:"),
      "Fabian Deutsch <fabian.deutsch@gmx.de>",
      "Vinicius Depizzol <vdepizzol@gmail.com>",
      "Andreas Kwiatkowski <post@kwiat.org>",
      "Carlos Pardo <me@m4de.com>",
      "Jakub Steiner <jimmac@ximian.com>",
      "",
      N_("See AUTHORS file for full credits"),
      NULL
  };

  authors [2] = gettext (authors [2]);
  authors [7] = gettext (authors [7]);
  authors [14] = gettext (authors [14]);

  const gchar *documenters [] = {
    "Damien Sandras <dsandras@seconix.com>",
    "Christopher Warner <zanee@kernelcode.com>",
    "Matthias Redlich <m-redlich@t-online.de>",
    NULL
  };

  const gchar *license[] = {
N_("This program is free software; you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation; either version 2 of the License, or \
(at your option) any later version. "),
N_("This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \
GNU General Public License for more details. \
You should have received a copy of the GNU General Public License \
along with this program; if not, write to the Free Software Foundation, \
Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA."),
N_("Ekiga is licensed under the GPL license and as a special exception, \
you have permission to link or otherwise combine this program with the \
programs OPAL and PWLIB, and distribute the combination, \
without applying the requirements of the GNU GPL to the OPAL \
and PWLIB programs, as long as you do follow the requirements of the \
GNU GPL for all the rest of the software thus combined.")
  };

  gchar *license_trans;

  /* Translators: Please write translator credits here, and
   * separate names with \n */
  const gchar *translator_credits = _("translator-credits");
  if (g_strcmp0 (translator_credits, "translator-credits") == 0)
    translator_credits = "No translators, English by\n"
        "Damien Sandras <dsandras@seconix.com>";

  const gchar *comments =  _("Ekiga is full-featured SIP compatible VoIP and IP-Telephony application that allows you to make audio calls to remote users with SIP hardware or software.");

  license_trans = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n",
                               _(license[2]), "\n\n", NULL);

  filename = g_build_filename (DATA_DIR, "pixmaps", PACKAGE_NAME,
                               PACKAGE_NAME "-logo.png", NULL);
  pixmap =  gtk_image_new_from_file (filename);

  gtk_show_about_dialog (GTK_WINDOW (gtk_application_get_active_window (GTK_APPLICATION (app))),
                         "name", "Ekiga",
                         "version", VERSION,
                         "copyright", "Copyright © 2000-2014 Damien Sandras",
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", translator_credits,
                         "comments", comments,
                         "logo", gtk_image_get_pixbuf (GTK_IMAGE (pixmap)),
                         "license", license_trans,
                         "wrap-license", TRUE,
                         "website", "http://www.ekiga.org",
                         NULL);

  g_free (license_trans);
  g_free (filename);
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


void
gm_application_show_addressbook_window (GmApplication *self)
{
  g_return_if_fail (GM_IS_APPLICATION (self));


  gtk_window_present (GTK_WINDOW (addressbook_window_new (self)));
}


void
gm_application_show_assistant_window (GmApplication *self)
{
  GtkWindow *parent = NULL;
  GtkWindow *window = NULL;

  g_return_if_fail (GM_IS_APPLICATION (self));

  parent = gtk_application_get_active_window (GTK_APPLICATION (self));

  window = GTK_WINDOW (assistant_window_new (self));
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
  gtk_window_present (window);
}
