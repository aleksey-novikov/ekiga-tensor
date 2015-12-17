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
 *                         sipendpoint.cpp  -  description
 *                         --------------------------------
 *   begin                : Wed 8 Dec 2004
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the SIP Endpoint class.
 *
 */


#include <glib/gi18n.h>
#include "config.h"
#include "sip-endpoint.h"

namespace Opal {

  namespace Sip {

    class RegistrarHandler : public PThread
    {
      PCLASSINFO (RegistrarHandler, PThread);

  public:

      RegistrarHandler (Opal::Account & _account,
                        Opal::Sip::EndPoint& _ep,
                        bool _registering)
        : PThread (1000, AutoDeleteThread),
        account (_account),
        ep (_ep),
        registering (_registering)
      {
        this->Resume ();
      }

      void Main ()
      {
        if (registering) {
          PString _aor;

          SIPRegister::Params params;
          params.m_addressOfRecord = "sip:" + account.get_username () + "@" + account.get_host () + ";transport=tcp";
          params.m_instanceId = ep.GetInstanceID ();
          params.m_compatibility = SIPRegister::e_RFC5626;
          params.m_authID = account.get_authentication_username ();
          params.m_password = account.get_password ();
          params.m_expire = account.is_enabled () ? account.get_timeout () : 0;
          params.m_minRetryTime = PMaxTimeInterval;  // use default value
          params.m_maxRetryTime = PMaxTimeInterval;  // use default value

          if (!account.get_outbound_proxy ().empty ())
            params.m_addressOfRecord = params.m_addressOfRecord + ";OPAL-proxy=" + account.get_outbound_proxy () + "%3Btransport=tcp";

          // Register the given aor to the given registrar
          ep.Register (params, _aor);
        }
        else
          ep.Unregister (account.get_full_uri (""));
      }

  private:
      Opal::Account & account;
      Opal::Sip::EndPoint& ep;
      bool registering;
    };
  };
};


/* The class */
Opal::Sip::EndPoint::EndPoint (Opal::EndPoint & _endpoint,
                               const Ekiga::ServiceCore& _core): SIPEndPoint (_endpoint),
                                                                 core (_core)
{
  /* Timeouts */
  SetRetryTimeouts (500, 4000);
  SetMaxRetries (20);

  /* Update the User Agent */
  SetUserAgent ("Ekiga/" PACKAGE_VERSION);

  /* Ready to take calls */
  GetManager ().AddRouteEntry("sip:.* = pc:*");
  GetManager ().AddRouteEntry("pc:.* = sip:<da>");

  /* Keepalive */
  PTimeInterval timeout;
  KeepAliveType type;
  GetKeepAlive (timeout, type);
  SetKeepAlive (timeout, KeepAliveByOPTION);
}


Opal::Sip::EndPoint::~EndPoint ()
{
}


bool
Opal::Sip::EndPoint::SetUpCall (const std::string & uri)
{
  PString token;
  boost::shared_ptr<Opal::Bank> bank = core.get<Opal::Bank> ("opal-account-store");
  if (bank) {
    Opal::AccountPtr account = bank->find_account (SIPURL (uri).GetHostPort ());
    if (account)
      return GetManager ().SetUpCall ("pc:*", account->get_full_uri (uri), token, (void*) uri.c_str ());
  }

  return GetManager ().SetUpCall ("pc:*", uri, token, (void*) uri.c_str ());
}


bool
Opal::Sip::EndPoint::StartListener (unsigned port)
{
  unsigned udp_min = GetManager ().GetUDPPortBase ();
  unsigned udp_max = GetManager ().GetUDPPortMax ();
  unsigned tcp_min = GetManager ().GetTCPPortBase ();
  unsigned tcp_max = GetManager ().GetTCPPortMax ();

  const std::string protocols[] = { "udp", "tcp", "" };
  const unsigned ports[][2] = { { udp_min, udp_max }, { tcp_min, tcp_max } };

  if (port > 0) {

    RemoveListener (NULL);
    for (int i = 0 ; !protocols[i].empty () ; i++) {

      std::stringstream str;
      str << protocols[i] << "$*:" << port;
      if (!StartListeners (PStringArray (str.str ()))) {

        port = ports[i][0];
        while (port <= ports[i][1]) {
          str << protocols[i] << "$*:" << port;
          if (StartListeners (PStringArray (str.str ()))) {
            PTRACE (4, "Opal::Sip::EndPoint\tSet listen port to " << port << " (" << protocols[i] << ")");
            break;
          }

          port++;
        }
      }
      else
        PTRACE (4, "Opal::Sip::EndPoint\tSet listen port to " << port << " (" << protocols[i] << ")");
    }
  }

  return false;
}


void
Opal::Sip::EndPoint::EnableAccount (Account & account)
{
  new RegistrarHandler (account, *this, true);
}


void
Opal::Sip::EndPoint::DisableAccount (Account & account)
{
  new RegistrarHandler (account, *this, false);
}


void
Opal::Sip::EndPoint::SetNoAnswerForwardTarget (const PString & _party)
{
  noAnswerForwardParty = _party;
}


void
Opal::Sip::EndPoint::SetUnconditionalForwardTarget (const PString & _party)
{
  unconditionalForwardParty = _party;
}


void
Opal::Sip::EndPoint::SetBusyForwardTarget (const PString & _party)
{
  busyForwardParty = _party;
}


void
Opal::Sip::EndPoint::SetInstanceID (const PString & id)
{
  if (!id.empty ())
    instanceID = PGloballyUniqueID (id);
}


PGloballyUniqueID &
Opal::Sip::EndPoint::GetInstanceID ()
{
  return instanceID;
}


void
Opal::Sip::EndPoint::OnRegistrationStatus (const RegistrationStatus & status)
{
  std::string info;

  boost::shared_ptr<Opal::Bank> bank = core.get<Opal::Bank> ("opal-account-store");
  if (!bank)
    return;

  Opal::AccountPtr account = bank->find_account (status.m_addressofRecord);
  if (!account)
    return;

  if (status.m_reason == SIP_PDU::Information_Trying)
    return;

  SIPEndPoint::OnRegistrationStatus (status);
  /* Only handle registration state transitions we are interested in.
   *
   * For example, if we were registering, we are interested in knowing if
   * the registration was successful or not. If the registration was not
   * successful and OPAL cleanly unregisters the account (as a consequence),
   * we are not interested in knowing the unregistration worked.
   */
  if (status.m_wasRegistering != account->is_enabled ())
    return;

  /* Successful registration or unregistration */
  if (status.m_reason == SIP_PDU::Successful_OK) {
    Ekiga::Runtime::run_in_main (boost::bind (&Opal::Account::handle_registration_event, account,
                                              status.m_wasRegistering?Account::Registered:Account::Unregistered,
                                              std::string (),
                                              status.m_addressofRecord));
  }
  /* Registration or unregistration failure */
  else {
    SIPURL m_addressOfRecord = SIPURL (status.m_addressofRecord);
    /* Try again in UDP mode */
    if (m_addressOfRecord.GetTransportProto () == "TCP") {
      SIPRegister::Params params;
      PString _aor;
      m_addressOfRecord.SetParamVar ("transport", "udp");
      if (m_addressOfRecord.GetParamVars ().Contains ("OPAL-proxy")) {
        PString proxy = m_addressOfRecord.GetParamVars().Get ("OPAL-proxy");
        PINDEX p = proxy.Find (";");
        if (p != P_MAX_INDEX)
          proxy = proxy.Left (p);
        m_addressOfRecord.SetParamVar ("OPAL-proxy", proxy);
      }
      params.m_addressOfRecord = m_addressOfRecord;
      params.m_instanceId = GetInstanceID ();
      params.m_compatibility = SIPRegister::e_RFC5626;
      params.m_authID = status.m_handler->GetAuthID ();
      params.m_password = status.m_handler->GetPassword ();
      params.m_expire = status.m_handler->GetExpire ();
      params.m_minRetryTime = PMaxTimeInterval;  // use default value
      params.m_maxRetryTime = PMaxTimeInterval;  // use default value
      if (status.m_handler->ShutDown ())
        activeSIPHandlers.Remove (status.m_handler); // Make sure the TCP handler is deleted
                                                     // or it will be retried indefinitely.
      SIPEndPoint::Register (params, _aor);
      return;
    }

    /* all these codes are defined in opal, file include/sip/sippdu.h */
    switch (status.m_reason) {
    case SIP_PDU::IllegalStatusCode:
      info = _("Illegal status code");
      break;

    case SIP_PDU::Local_TransportError:
      info = _("Transport error");
      break;

    case SIP_PDU::Local_BadTransportAddress:
      info = _("Invalid address");
      break;

    case SIP_PDU::Local_Timeout:
      /* Translators: Host of the remote party is offline, this should
       * appear when the remote host does not reply in an acceptable time */
      info = _("Remote party host is offline");
      break;

    case SIP_PDU::Information_Trying:
    case SIP_PDU::Information_Ringing:
    case SIP_PDU::Information_CallForwarded:
    case SIP_PDU::Information_Queued:
    case SIP_PDU::Information_Session_Progress:
    case SIP_PDU::Successful_OK:
    case SIP_PDU::Successful_Accepted:
      break;

    case SIP_PDU::Redirection_MultipleChoices:
      /* Translators: the following strings are answers from the SIP server
       * when the packet it receives has an error, see
       * http://www.ietf.org/rfc/rfc3261.txt, chapter 21 for more information */
      info = _("Multiple choices");
      break;

    case SIP_PDU::Redirection_MovedPermanently:
      info = _("Moved permanently");
      break;

    case SIP_PDU::Redirection_MovedTemporarily:
      info = _("Moved temporarily");
      break;

    case SIP_PDU::Redirection_UseProxy:
      info = _("Use proxy");
      break;

    case SIP_PDU::Redirection_AlternativeService:
      info = _("Alternative service");
      break;

    case SIP_PDU::Failure_BadRequest:
      info = _("Bad request");
      break;

    case SIP_PDU::Failure_UnAuthorised:
      info = _("Unauthorized");
      break;

    case SIP_PDU::Failure_PaymentRequired:
      info = _("Payment required");
      break;

    case SIP_PDU::Failure_Forbidden:
      info = _("Forbidden, please check that username and password are correct");
      break;

    case SIP_PDU::Failure_NotFound:
      info = _("Not found");
      break;

    case SIP_PDU::Failure_MethodNotAllowed:
      info = _("Method not allowed");
      break;

    case SIP_PDU::Failure_NotAcceptable:
      info = _("Not acceptable");
      break;

    case SIP_PDU::Failure_ProxyAuthenticationRequired:
      info = _("Proxy authentication required");
      break;

    case SIP_PDU::Failure_RequestTimeout:
      info = _("Timeout");
      break;

    case SIP_PDU::Failure_Conflict:
      info = _("Conflict");
      break;

    case SIP_PDU::Failure_LengthRequired:
      info = _("Length required");
      break;

    case SIP_PDU::Failure_RequestEntityTooLarge:
      info = _("Request entity too big");
      break;

    case SIP_PDU::Failure_RequestURITooLong:
      info = _("Request URI too long");
      break;

    case SIP_PDU::Failure_UnsupportedMediaType:
      info = _("Unsupported media type");
      break;

    case SIP_PDU::Failure_UnsupportedURIScheme:
      info = _("Unsupported URI scheme");
      break;

    case SIP_PDU::Failure_BadExtension:
      /* Translators:  The extension we are trying to register does not exist.
       * Here extension is a specific "phone number", see
       * http://en.wikipedia.org/wiki/Extension_(telephone)
       * for more information */
      info = _("Bad extension");
      break;

    case SIP_PDU::Failure_ExtensionRequired:
      info = _("Extension required");
      break;

    case SIP_PDU::Failure_IntervalTooBrief:
      info = _("Interval too brief");
      break;

    case SIP_PDU::Failure_TemporarilyUnavailable:
      info = _("Temporarily unavailable");
      break;

    case SIP_PDU::Failure_LoopDetected:
      info = _("Loop detected");
      break;

    case SIP_PDU::Failure_TooManyHops:
      info = _("Too many hops");
      break;

    case SIP_PDU::Failure_AddressIncomplete:
      info = _("Address incomplete");
      break;

    case SIP_PDU::Failure_Ambiguous:
      info = _("Ambiguous");
      break;

    case SIP_PDU::Failure_BusyHere:
      info = _("Busy Here");
      break;

    case SIP_PDU::Failure_RequestTerminated:
      info = _("Request terminated");
      break;

    case SIP_PDU::Failure_NotAcceptableHere:
      info = _("Not acceptable here");
      break;

    case SIP_PDU::Failure_BadEvent:
      info = _("Bad event");
      break;

    case SIP_PDU::Failure_RequestPending:
      info = _("Request pending");
      break;

    case SIP_PDU::Failure_Undecipherable:
      info = _("Undecipherable");
      break;

    case SIP_PDU::Failure_InternalServerError:
      info = _("Internal server error");
      break;

    case SIP_PDU::Failure_NotImplemented:
      info = _("Not implemented");
      break;

    case SIP_PDU::Failure_BadGateway:
      info = _("Bad gateway");
      break;

    case SIP_PDU::Failure_ServiceUnavailable:
      info = _("Service unavailable");
      break;

    case SIP_PDU::Failure_ServerTimeout:
      info = _("Server timeout");
      break;

    case SIP_PDU::Failure_SIPVersionNotSupported:
      info = _("SIP version not supported");
      break;

    case SIP_PDU::Failure_MessageTooLarge:
      info = _("Message too large");
      break;

    case SIP_PDU::GlobalFailure_BusyEverywhere:
      info = _("Busy everywhere");
      break;

    case SIP_PDU::GlobalFailure_Decline:
      info = _("Decline");
      break;

    case SIP_PDU::GlobalFailure_DoesNotExistAnywhere:
      info = _("Does not exist anymore");
      break;

    case SIP_PDU::GlobalFailure_NotAcceptable:
      info = _("Globally not acceptable");
      break;

    case SIP_PDU::Local_NotAuthenticated:
      info = _("Invalid certificates");
      break;

    case SIP_PDU::Failure_TransactionDoesNotExist:
    case SIP_PDU::Failure_Gone:
    case SIP_PDU::MaxStatusCode:
    case SIP_PDU::Local_NoCompatibleListener:
    case SIP_PDU::Local_CannotMapScheme:
    case SIP_PDU::Local_KeepAlive:
    case SIP_PDU::Local_TransportLost:
    case SIP_PDU::Failure_UnresolvableDestination:
    default:
      info = _("Failed");
    }

    /* Opal adds a RequestTerminated, and this should not be shown to user,
     * as a sip code has already been scheduled to be shown
     */
    if (status.m_reason != SIP_PDU::Failure_RequestTerminated) {
      Ekiga::Runtime::run_in_main (boost::bind (&Opal::Account::handle_registration_event, account,
                                                status.m_wasRegistering?Account::RegistrationFailed:Account::UnregistrationFailed,
                                                info, std::string ()));
    }
  }
}


void
Opal::Sip::EndPoint::OnMWIReceived (const PString & party,
                                    OpalManager::MessageWaitingType /*type*/,
                                    const PString & info)
{
  std::string mwi = info;
  std::transform (mwi.begin(), mwi.end(), mwi.begin(), ::tolower);
  if (mwi == "no")
    mwi = "0/0";

  /* Signal */
  Ekiga::Runtime::run_in_main (boost::bind (boost::ref(mwi_event), party, mwi));
}


bool
Opal::Sip::EndPoint::OnIncomingConnection (OpalConnection &connection,
                                           unsigned options,
                                           OpalConnection::StringOptions *stroptions)
{
  PTRACE (3, "Opal::Sip::EndPoint\tIncoming connection");

  if (!SIPEndPoint::OnIncomingConnection (connection, options, stroptions))
    return false;

  /* Unconditional call forward? */
  if (!unconditionalForwardParty.IsEmpty ()) {
    PTRACE (3, "Opal::Sip::EndPoint\tIncoming connection forwarded to " << busyForwardParty << " (Unconditional)");
    connection.ForwardCall (unconditionalForwardParty);
    return false;
  }

  /* Busy call forward? */
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReference); conn != NULL; ++conn) {
    if (conn->GetCall().GetToken() != connection.GetCall().GetToken() && !conn->IsReleased ()) {
      if (!busyForwardParty.IsEmpty ()) {
        PTRACE (3, "Opal::Sip::EndPoint\tIncoming connection forwarded to " << busyForwardParty << " (busy)");
        connection.ForwardCall (busyForwardParty);
      }
      else {
        PTRACE (3, "Opal::Sip::EndPoint\tIncoming connection rejected (busy)");
        connection.ClearCall (OpalConnection::EndedByLocalBusy);
      }
      return false;
    }
  }

  /* No Answer Call Forward or Reject */
  Opal::Call *call = dynamic_cast<Opal::Call *> (&connection.GetCall ());
  if (call)
    call->set_forward_target (noAnswerForwardParty);

  return true;
}


void
Opal::Sip::EndPoint::OnDialogInfoReceived (const SIPDialogNotification & info)
{
  gchar* _status = NULL;
  std::string status;
  std::string presence;
  std::string uri = (const char *) info.m_entity;
  PString remote_uri = info.m_remote.m_identity;
  PString remote_display_name = info.m_remote.m_display.IsEmpty () ? remote_uri : info.m_remote.m_display;
  if (uri.find ("sip:") == string::npos)
    uri = "sip:" + uri;

  switch (info.m_state) {
  case SIPDialogNotification::Proceeding:
  case SIPDialogNotification::Early:
    if (!remote_display_name.IsEmpty ())
      _status = g_strdup_printf (_("Incoming call from %s"), (const char *) remote_display_name);
    else
      _status = g_strdup_printf (_("Incoming call"));
    status = _status;
    presence = "ringing";
    break;
  case SIPDialogNotification::Confirmed:
    if (!remote_display_name.IsEmpty ())
      _status = g_strdup_printf (_("In a call with %s"), (const char *) remote_display_name);
    else
      _status = g_strdup_printf (_("In a call"));
    presence = "inacall";
    status = _status;
    break;
  case SIPDialogNotification::EndStates:  // bookkeeping code
    break;
  case SIPDialogNotification::Trying:
  case SIPDialogNotification::Terminated:
  default:
    break;
  }
}
