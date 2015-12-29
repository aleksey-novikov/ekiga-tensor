/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>

 * This program is free software; you can  redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version. This program is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Ekiga is licensed under the GPL license and as a special exception, you
 * have permission to link or otherwise combine this program with the
 * programs OPAL, OpenH323 and PWLIB, and distribute the combination, without
 * applying the requirements of the GNU GPL to the OPAL, OpenH323 and PWLIB
 * programs, as long as you do follow the requirements of the GNU GPL for all
 * the rest of the software thus combined.
 */


/*
 *                         call-core.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : declaration of the interface of a call core.
 *                          A call core manages CallManagers.
 *
 */

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include "config.h"

#include <glib/gi18n.h>
#include <sstream>

#include "call-core.h"

#include "call-manager.h"


using namespace Ekiga;

CallCore::CallCore (boost::shared_ptr<Ekiga::NotificationCore> _notification_core) : notification_core(_notification_core)
{
}


CallCore::~CallCore ()
{
#if DEBUG
  std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
#endif
}


void CallCore::add_manager (boost::shared_ptr<CallManager> manager)
{
  managers.add_object (manager);
  manager_added (manager);
}


void CallCore::remove_manager (boost::shared_ptr<CallManager> manager)
{
  manager_removed (manager);
  managers.remove_object (manager);
}


CallCore::iterator CallCore::begin ()
{
  return managers.begin ();
}


CallCore::const_iterator CallCore::begin () const
{
  return managers.begin ();
}


CallCore::iterator CallCore::end ()
{
  return managers.end ();
}


CallCore::const_iterator CallCore::end () const
{
  return managers.end ();
}


bool CallCore::dial (const std::string & uri)
{
  for (CallCore::iterator iter = begin ();
       iter != end ();
       iter++) {
    if ((*iter)->dial (uri))
      return true;
  }

  return false;
}


void CallCore::hang_up ()
{
  for (CallCore::iterator iter = begin ();
       iter != end ();
       iter++)
    (*iter)->hang_up ();
}


bool CallCore::is_supported_uri (const std::string & uri)
{
  for (CallCore::iterator iter = begin ();
       iter != end ();
       iter++) {
    if ((*iter)->is_supported_uri (uri))
      return true;
  }

  return false;
}


Ekiga::CodecList
CallCore::get_codecs () const
{
  Ekiga::CodecList codecs;
  for (CallCore::const_iterator iter = begin ();
       iter != end ();
       iter++) {
    codecs.append ((*iter)->get_codecs ());
  }

  return codecs;
}


void
CallCore::set_codecs (Ekiga::CodecList & codecs)
{
  for (CallCore::iterator iter = begin ();
       iter != end ();
       iter++) {
    (*iter)->set_codecs (codecs);
  }
}


void CallCore::add_call (const boost::shared_ptr<Call> & call)
{
  calls.add_object (call);

  // Relay signals
  calls.add_connection (call, call->ringing.connect (boost::bind (boost::ref (ringing_call), _1)));
  calls.add_connection (call, call->setup.connect (boost::bind (&CallCore::on_setup_call, this, _1)));
  calls.add_connection (call, call->missed.connect (boost::bind (&CallCore::on_missed_call, this, _1)));
  calls.add_connection (call, call->cleared.connect (boost::bind (boost::ref (cleared_call), _1, _2)));
  calls.add_connection (call, call->established.connect (boost::bind (boost::ref (established_call), _1)));
  calls.add_connection (call, call->held.connect (boost::bind (boost::ref (held_call), _1)));
  calls.add_connection (call, call->retrieved.connect (boost::bind (boost::ref (retrieved_call), _1)));
  calls.add_connection (call, call->stream_opened.connect (boost::bind (boost::ref (stream_opened), _1, _2, _3, _4)));
  calls.add_connection (call, call->stream_closed.connect (boost::bind (boost::ref (stream_closed), _1, _2, _3, _4)));
  calls.add_connection (call, call->stream_paused.connect (boost::bind (boost::ref (stream_paused), _1, _2, _3)));
  calls.add_connection (call, call->stream_resumed.connect (boost::bind (boost::ref (stream_resumed), _1, _2, _3)));

  calls.object_removed.connect (boost::bind (boost::ref (removed_call), _1));

  created_call (call);
}

void CallCore::on_setup_call (const boost::shared_ptr<Call> call)
{
  setup_call (call);
}

void CallCore::on_missed_call (const boost::shared_ptr<Call> call)
{
  boost::shared_ptr<Ekiga::NotificationCore> _notification_core = notification_core.lock ();
  if (_notification_core) {
    size_t pos = call->get_remote_uri().find(':');
    std::string uri = call->get_remote_uri().substr(pos == std::string::npos ? 0 : pos + 1);
    uri = uri.substr(0, uri.find('@'));

    std::stringstream msg;
    msg << _("Missed call from") << " " << (call->get_remote_party_name() == call->get_remote_uri() ? uri : uri + " (" + call->get_remote_party_name() + ")");
    boost::shared_ptr<Ekiga::Notification> notif (new Ekiga::Notification (Ekiga::Notification::Warning,
                                                                           _("Missed call"), msg.str (),
                                                                           _("Call"),
                                                                           boost::bind (&Ekiga::CallCore::dial, this, call->get_remote_uri ())));
    _notification_core->push_notification (notif);
  }

  missed_call (call);
}
