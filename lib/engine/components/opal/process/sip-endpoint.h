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
 *                         sipendpoint.h  -  description
 *                         -----------------------------
 *   begin                : Wed 24 Nov 2004
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the SIP Endpoint class.
 *
 */


#ifndef _SIP_ENDPOINT_H_
#define _SIP_ENDPOINT_H_

#include <ptlib.h>

#include <opal/opal.h>
#include <sip/sip.h>

#include "presence-core.h"
#include "call-manager.h"
#include "opal-bank.h"
#include "call-core.h"
#include "services.h"

#include "opal-call-manager.h"
#include "opal-endpoint.h"

namespace Opal {

  namespace Sip {

    class EndPoint : public SIPEndPoint
    {
      PCLASSINFO(EndPoint, SIPEndPoint);

    public:

      EndPoint (Opal::EndPoint& ep,
                const Ekiga::ServiceCore& core);

      ~EndPoint ();

      bool SetUpCall (const std::string & uri);


      bool StartListener (unsigned port);

      //
      // a message waiting information was received
      // the parameters are the aor and the info
      boost::signals2::signal<void(std::string, std::string)> mwi_event;


      /* Enable / Disable accounts. The account given as argument
       * will be updated to reflect the current account state once
       * the operation has been successful.
       */
      void EnableAccount (Account & account);

      void DisableAccount (Account & account);

      void SetNoAnswerForwardTarget (const PString & party);

      void SetUnconditionalForwardTarget (const PString & party);

      void SetBusyForwardTarget (const PString & party);

      void SetInstanceID (const PString & id);

      PGloballyUniqueID & GetInstanceID ();

    private:
      /* OPAL Methods */
      void OnRegistrationStatus (const RegistrationStatus & status);

      void OnMWIReceived (const PString & party,
                          OpalManager::MessageWaitingType type,
                          const PString & info);

      bool OnIncomingConnection (OpalConnection &connection,
                                 unsigned options,
                                 OpalConnection::StringOptions * stroptions);

      void OnDialogInfoReceived (const SIPDialogNotification & info);

      const Ekiga::ServiceCore & core;

      PString noAnswerForwardParty;
      PString unconditionalForwardParty;
      PString busyForwardParty;
      PGloballyUniqueID instanceID;
    };
  };
};
#endif
