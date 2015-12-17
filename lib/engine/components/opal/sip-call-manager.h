
/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2015 Damien Sandras <dsandras@seconix.com>
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
 *                         sip-call-manager.h  -  description
 *                         ----------------------------------
 *   begin                : Sun Mar 15 2014
 *   authors              : Damien Sandras
 *   description          : This file contains the engine SIP CallManager.
 *
 */


#ifndef __SIP_CALL_MANAGER_H_
#define __SIP_CALL_MANAGER_H_

#include "opal-call-manager.h"
#include "opal-endpoint.h"
#include "sip-endpoint.h"

#include "ekiga-settings.h"

namespace Opal {

  namespace Sip {

    /* This is one engine SIP CallManager implementation.
     * It uses the Opal::CallManager class to implement a
     * more specialized SIP engine CallManager.
     */
    class CallManager :
        public Opal::CallManager,
        public Ekiga::URIActionProvider
    {
  public:
      CallManager (Ekiga::ServiceCore& core,
                   Opal::EndPoint& endpoint,
                   Opal::Sip::EndPoint& sip_endpoint);
      ~CallManager ();

      /* URIActionProvider Methods */
      void pull_actions (Ekiga::Actor & actor,
                         const std::string & name,
                         const std::string & uri);

      /* CallManager methods we implement */
      bool dial (const std::string & uri);

      bool is_supported_uri (const std::string & uri);

      const std::string & get_protocol_name () const;

      const Ekiga::CallManager::InterfaceList get_interfaces () const;

      bool set_listen_port (unsigned port);

      void set_dtmf_mode (unsigned mode);

      unsigned get_dtmf_mode () const;

  private:
      void setup (const std::string & setting = "");

      Ekiga::SettingsPtr sip_settings;
      Ekiga::SettingsPtr call_forwarding_settings;
      Opal::Sip::EndPoint& sip_endpoint;
      std::string protocol_name;
    };
  };
};
#endif
