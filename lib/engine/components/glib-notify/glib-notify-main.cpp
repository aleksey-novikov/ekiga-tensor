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
 *                         glib-notify-main.cpp  -  description
 *                         ------------------------------------
 *   begin                : written in 2009 by Damien Sandras
 *   copyright            : (c) 2009 by Julien Puydt
 *   description          : code to push user notifications to the desktop
 *
 */

#include <map>
#include <boost/smart_ptr.hpp>

#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <sstream>

#include "config.h"

#include "kickstart.h"
#include "notification-core.h"
#include "call-core.h"
#include "scoped-connections.h"


class GNotify:
  public Ekiga::Service
{
public:

  GNotify (Ekiga::ServiceCore& core);

  ~GNotify ();

  const std::string get_name () const
  { return "libnotify"; }

  const std::string get_description () const
  { return "\tService pushing user notifications to the desktop"; }

private:

  Ekiga::scoped_connections connections;

  void on_notification_added (boost::shared_ptr<Ekiga::Notification> notif);
  void on_notification_removed (boost::shared_ptr<Ekiga::Notification> notif);
  void on_call_notification (boost::shared_ptr<Ekiga::Call> call);
  void on_call_notification_closed ();

  typedef std::map<boost::shared_ptr<Ekiga::Notification>, std::pair<boost::signals2::connection, boost::shared_ptr<GNotification> > > container_type;
  container_type live;
};


static void
g_notification_action_activated_cb (GSimpleAction *action,
                                    G_GNUC_UNUSED GVariant *variant,
                                    gpointer data)
{
  Ekiga::Notification *notif = (Ekiga::Notification *) data;
  gchar *name = NULL;

  g_object_get (action, "name", &name, NULL);

  notif->action_trigger ();
  notif->removed ();

  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()), name);
  g_free (name);
}


struct GNOTIFYSpark: public Ekiga::Spark
{
  GNOTIFYSpark (): result(false)
  {}

  bool try_initialize_more (Ekiga::ServiceCore& core,
			    int* /*argc*/,
			    char** /*argv*/[])
  {
    Ekiga::ServicePtr service = core.get ("GNOTIFY");

    if (!service) {
      core.add (Ekiga::ServicePtr (new GNotify (core)));
      result = true;
    }

    return result;
  }

  Ekiga::Spark::state get_state () const
  { return result?FULL:BLANK; }

  const std::string get_name () const
  { return "GNOTIFY"; }

  bool result;
};


void
gnotify_init (Ekiga::KickStart& kickstart)
{
  boost::shared_ptr<Ekiga::Spark> spark(new GNOTIFYSpark);
  kickstart.add_spark (spark);
}


GNotify::GNotify (Ekiga::ServiceCore& core)
{
  boost::shared_ptr<Ekiga::NotificationCore> notification_core = core.get<Ekiga::NotificationCore> ("notification-core");
  boost::shared_ptr<Ekiga::CallCore> call_core = core.get<Ekiga::CallCore> ("call-core");

  /* Notifications coming from various components */
  connections.add (notification_core->notification_added.connect (boost::bind (&GNotify::on_notification_added, this, _1)));

  /* Specific notifications */
//  connections.add (call_core->setup_call.connect (boost::bind (&GNotify::on_call_notification, this, _1)));
}


GNotify::~GNotify ()
{
}


void
GNotify::on_notification_added (boost::shared_ptr<Ekiga::Notification> notification)
{
  GNotification* notif = g_notification_new (notification->get_title ().c_str ());
  GSimpleAction *action = NULL;

  g_notification_set_body (notif, notification->get_body ().c_str ());

  if (notification->get_level () == Ekiga::Notification::Error)
    g_notification_set_priority (notif, G_NOTIFICATION_PRIORITY_HIGH);

  if (!notification->get_action_name ().empty ()) {

    g_notification_set_priority (notif, G_NOTIFICATION_PRIORITY_URGENT);
    std::ostringstream id;
    std::ostringstream action_id;

    id << notification.get ();
    action = g_simple_action_new (id.str ().c_str (), NULL); // We use a unique ID and not
                                                             // a global action as the action
                                                             // will disappear as soon as the notification
                                                             // disappears.
    g_action_map_add_action (G_ACTION_MAP (g_application_get_default ()),
                             G_ACTION (action));
    g_signal_connect (action, "activate",
                      G_CALLBACK (g_notification_action_activated_cb),
                      (gpointer) notification.get ());
    action_id << "app." << notification.get ();
    g_notification_add_button (notif,
                               notification->get_action_name ().c_str (),
                               action_id.str ().c_str ());
    g_object_unref (action);
  }

  boost::signals2::connection conn =
    notification->removed.connect (boost::bind (&GNotify::on_notification_removed,
                                                this, notification));
  live[notification] = std::pair<boost::signals2::connection, boost::shared_ptr<GNotification> > (conn, boost::shared_ptr<GNotification> (notif, g_object_unref));
  g_application_send_notification (g_application_get_default (), NULL, notif);
}

void
GNotify::on_notification_removed (boost::shared_ptr<Ekiga::Notification> notification)
{
  container_type::iterator iter = live.find (notification);

  if (iter != live.end ()) {
    iter->second.first.disconnect ();
    live.erase (iter);
  }
}


void
GNotify::on_call_notification_closed ()
{
  g_application_withdraw_notification (g_application_get_default (), "ekiga-call-notif");
}


void
GNotify::on_call_notification (boost::shared_ptr<Ekiga::Call> call)
{
  GNotification *notif = NULL;

  if (call->is_outgoing ())
    return; // Ignore

  gchar *title = g_strdup_printf (_("Incoming call from %s"), call->get_remote_party_name ().c_str ());
  gchar *body = g_strdup_printf (_("Remote URI: %s"), call->get_remote_uri ().c_str ());

  notif = g_notification_new (title);
  g_notification_set_body (notif, body);

  g_notification_add_button (notif, _("Reject"), "app.reject");
  g_notification_add_button (notif, _("Answer"), "app.answer");

  g_notification_set_priority (notif, G_NOTIFICATION_PRIORITY_URGENT);

  connections.add (call->established.connect (boost::bind (&GNotify::on_call_notification_closed, this)));
  connections.add (call->missed.connect (boost::bind (&GNotify::on_call_notification_closed, this)));
  connections.add (call->cleared.connect (boost::bind (&GNotify::on_call_notification_closed, this)));

  g_application_send_notification (g_application_get_default (), "ekiga-call-notif", notif);

  g_free (title);
  g_free (body);

  g_object_unref (notif);
}
