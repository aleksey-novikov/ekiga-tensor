
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
 *                         sip-call-manager.cpp  -  description
 *                         ------------------------------------
 *   begin                : Sun Mar 15 2014
 *   authors              : Damien Sandras
 *   description          : This file contains the engine SIP CallManager.
 *
 */

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include "config.h"

#include <glib/gi18n.h>

#include "sip-call-manager.h"
#include "sip-endpoint.h"


/* The engine class */
Opal::Sip::CallManager::CallManager (Ekiga::ServiceCore& _core,
                                     Opal::EndPoint& _endpoint,
                                     Opal::Sip::EndPoint& _sip_endpoint)
        : Opal::CallManager (_core, _endpoint),
          sip_endpoint (_sip_endpoint), protocol_name ("sip")
{
  /* Setup things */
  Ekiga::SettingsCallback setup_cb = boost::bind (&Opal::Sip::CallManager::setup, this, _1);
  sip_settings = Ekiga::SettingsPtr (new Ekiga::Settings (SIP_SCHEMA, setup_cb));
  call_forwarding_settings = Ekiga::SettingsPtr (new Ekiga::Settings (CALL_FORWARDING_SCHEMA, setup_cb));

  /* Initial setup */
  setup ("");

  /* Generate a RFC 5626 compatible instance id if needed */
  std::string instance_id = sip_settings->get_string ("instance-id");
  if (instance_id.empty ()) {
    instance_id = (const char*) PGloballyUniqueID ().AsString ();
    sip_settings->set_string ("instance-id", instance_id);
  }

  /* STUN must be enabled at the very beginning */
  Ekiga::SettingsPtr nat_settings = Ekiga::SettingsPtr (new Ekiga::Settings (NAT_SCHEMA, setup_cb));
  if (nat_settings->get_bool ("enable-stun") && !nat_settings->get_string ("stun-server").empty ())
    _endpoint.SetStunServer (nat_settings->get_string ("stun-server"));
  else
    _endpoint.SetStunServer (PString::Empty ());
}


Opal::Sip::CallManager::~CallManager ()
{
#if DEBUG
  std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
#endif
}


/* URIActionProvider Methods */
void Opal::Sip::CallManager::pull_actions (Ekiga::Actor & actor,
                                           G_GNUC_UNUSED const std::string & name,
                                           const std::string & uri)
{
  if (is_supported_uri (uri))
    Ekiga::URIActionProvider::add_action (actor, Ekiga::ActionPtr (new Ekiga::Action ("call", _("Call"), boost::bind (&Opal::CallManager::dial, this, uri))));
}


bool Opal::Sip::CallManager::dial (const std::string & uri)
{
  if (!is_supported_uri (uri))
    return false;

  return sip_endpoint.SetUpCall (uri);
}


bool
Opal::Sip::CallManager::is_supported_uri (const std::string & uri)
{
  return (!uri.empty () && (uri.find ("sip:") == 0 || uri.find (':') == string::npos));
}


const std::string & Opal::Sip::CallManager::get_protocol_name () const
{
  return protocol_name;
}


const Ekiga::CallManager::InterfaceList Opal::Sip::CallManager::get_interfaces () const
{
  Ekiga::CallManager::InterfaceList ilist;

  OpalListenerList listeners = sip_endpoint.GetListeners ();
  for (int i = 0 ; i < listeners.GetSize () ; i++) {
    Ekiga::CallManager::Interface iface;
    PIPSocket::Address address;
    WORD port;
    PString proto_prefix = listeners[i].GetLocalAddress ().GetProtoPrefix ();
    listeners[i].GetLocalAddress ().GetIpAndPort (address, port);

    iface.voip_protocol = get_protocol_name ();
    iface.id = "*";
    iface.protocol = (const char*) proto_prefix.Left (proto_prefix.GetLength () - 1); // Strip final $ delimiter
    iface.port = (unsigned int) port;

    ilist.push_back (iface);
  }

  return ilist;
}


bool
Opal::Sip::CallManager::set_listen_port (unsigned port)
{
  return sip_endpoint.StartListener (port);
}


void
Opal::Sip::CallManager::set_dtmf_mode (unsigned mode)
{
  switch (mode) {

  case 0:  // RFC2833
    PTRACE (4, "Opal::Sip::CallManager\tSet DTMF Mode to RFC2833");
    sip_endpoint.SetSendUserInputMode (OpalConnection::SendUserInputAsInlineRFC2833);
    break;
  case 1:  // SIP Info
  default:
    PTRACE (4, "Opal::Sip::CallManager\tSet DTMF Mode to SIP INFO");
    sip_endpoint.SetSendUserInputMode (OpalConnection::SendUserInputAsTone);
    break;
  }
}


unsigned
Opal::Sip::CallManager::get_dtmf_mode () const
{
  switch (sip_endpoint.GetSendUserInputMode ()) {
  case OpalConnection::SendUserInputAsInlineRFC2833:  // RFC2833
    return 0;
  case OpalConnection::SendUserInputAsTone:  // SIP Info
  default:
    return 1;
  }
}


void Opal::Sip::CallManager::setup (const std::string & setting)
{
  if (!endpoint.IsReady ()) {
    endpoint.ready.connect (boost::bind (&Opal::Sip::CallManager::setup, this, ""));
    return;
  }

  if (setting.empty () || setting == "instance-id") {
    std::string instance_id = sip_settings->get_string ("instance-id");
    if (!instance_id.empty ())
      sip_endpoint.SetInstanceID (instance_id);
  }

  if (setting.empty () || setting == "listen-port")
    set_listen_port (sip_settings->get_int ("listen-port"));

  if (setting.empty () || setting == "keepalive-interval")  {
    int delay = sip_settings->get_int ("keepalive-interval");
    PTRACE (4, "Opal::Sip::CallManager\tKeepalive interval set to " << delay);
    sip_endpoint.SetKeepAlive (PTimeInterval (0, delay), SIPEndPoint::KeepAliveByOPTION);
  }

  if (setting.empty () || setting == "dtmf-mode")
    set_dtmf_mode (sip_settings->get_enum ("dtmf-mode"));

  /* Setup the various forwarding targets.
   * The no answer delay is defined in the opal-call-manager (our parent).
   */
  if (setting.empty () || setting == "forward-on-no-anwer" || setting == "forward-host")
    sip_endpoint.SetNoAnswerForwardTarget (call_forwarding_settings->get_bool ("forward-on-no-answer") ? sip_settings->get_string ("forward-host") : "");

  if (setting.empty () || setting == "forward-on-busy" || setting == "forward-host")
    sip_endpoint.SetBusyForwardTarget (call_forwarding_settings->get_bool ("forward-on-busy") ? sip_settings->get_string ("forward-host") : "");

  if (setting.empty () || setting == "always-forward" || setting == "forward-host")
    sip_endpoint.SetUnconditionalForwardTarget (call_forwarding_settings->get_bool ("always-forward") ? sip_settings->get_string ("forward-host") : "");

  Opal::CallManager::setup (setting);
}
