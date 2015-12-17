/*
 * Ekiga -- A VoIP and Video-Conferencing application
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
 *                         opal-call.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : declaration of the interface of a call handled by
 *                          Opal.
 *
 */


#include <cctype>
#include <algorithm>

#include <glib/gi18n.h>
#include <opal/opal.h>
#include <ep/pcss.h>
#include <sip/sippdu.h>

#include "call.h"
#include "opal-call.h"
#include "opal-endpoint.h"
#include "notification-core.h"
#include "call-core.h"
#include "runtime.h"
#include "known-codecs.h"

using namespace Opal;

static void
strip_special_chars (std::string& str, char* special_chars, bool start)
{
  std::string::size_type idx;

  unsigned i = 0;
  while (i < strlen (special_chars)) {
    idx = str.find_first_of (special_chars[i]);
    if (idx != std::string::npos) {
      if (start)
        str = str.substr (idx+1);
      else
        str = str.substr (0, idx);
    }
    i++;
  }
}


boost::shared_ptr<Opal::Call>
Opal::Call::create (EndPoint& _manager,
                    const std::string & uri,
                    const unsigned no_answer_delay)
{
  return boost::shared_ptr<Opal::Call> (new Opal::Call (_manager, uri, no_answer_delay));
}


Opal::Call::Call (Opal::EndPoint& _manager,
                  const std::string& _uri,
                  const unsigned _no_answer_delay)
  : OpalCall (_manager),
    Ekiga::Call (),
    remote_uri (_uri),
    call_setup (false),
    outgoing (false)
{
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("hangup", _("Hangup"),
                                                   boost::bind (&Call::hang_up, this))));
  if (!is_outgoing () && !IsEstablished ()) {
    add_action (Ekiga::ActionPtr (new Ekiga::Action ("answer", _("Answer"),
                                                     boost::bind (&Call::answer, this))));
    add_action (Ekiga::ActionPtr (new Ekiga::Action ("reject", _("Reject"),
                                                     boost::bind (&Call::hang_up, this))));

    noAnswerTimer.SetNotifier (PCREATE_NOTIFIER (OnNoAnswerTimeout));
    if (_no_answer_delay > 0)
      noAnswerTimer.SetInterval (0, _no_answer_delay);
  }
}


Opal::Call::~Call ()
{
}


void
Opal::Call::hang_up ()
{
  if (!is_outgoing () && !IsEstablished ())
    Clear (OpalConnection::EndedByAnswerDenied);
  else
    Clear ();
}


void
Opal::Call::answer ()
{
  if (!is_outgoing () && !IsEstablished ()) {
    PSafePtr<OpalPCSSConnection> connection = GetConnectionAs<OpalPCSSConnection>();
    if (connection != NULL) {
      connection->AcceptIncoming ();
      remove_action ("reject");
      remove_action ("answer");
    }
  }
}


void
Opal::Call::transfer ()
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request =
    boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Call::on_transfer_form_submitted, this, _1, _2, _3)));

  request->title (_("Transfer Call"));
  request->action (_("Transfer"));
  request->text ("uri", _("Remote URI"),
                 "",
                 _("sip:username@ekiga.net"),
                 Ekiga::FormVisitor::STANDARD,
                 false, false);

  Ekiga::Call::questions (request);
}


bool
Opal::Call::transfer (std::string uri)
{
  PSafePtr<OpalConnection> connection = GetConnection ();
  if (connection != NULL)
    return connection->TransferConnection (uri);

  return false;
}


bool
Opal::Call::on_transfer_form_submitted (bool submitted,
                                        Ekiga::Form& result,
                                        std::string& error)
{
  std::string::size_type idx;

  if (!submitted)
    return false;

  std::string uri = result.text ("uri");

  /* If the user did not provide a full URI,
   * use the same "domain" than the call local address
   */
  idx = uri.find_first_of ("@");
  if (idx == std::string::npos)
    uri = uri + "@" + (const char *) PURL (remote_uri).GetHostName ();
  if (transfer (uri))
    return true;
  else
    error = _("You supplied an unsupported address");

  return false;
}


void
Opal::Call::toggle_hold ()
{
  bool on_hold = false;
  PSafePtr<OpalConnection> connection = GetConnection ();
  if (connection != NULL) {

    on_hold = connection->IsOnHold (false);
    connection->HoldRemote (!on_hold);
  }
}


void
Opal::Call::toggle_stream_pause (StreamType type)
{
  OpalMediaStreamPtr stream = NULL;
  PString codec_name;
  std::string stream_name;

  bool paused = false;

  PSafePtr<OpalConnection> connection = GetConnection ();
  if (connection != NULL) {

    stream = connection->GetMediaStream (OpalMediaType::Audio (), false);
    if (stream != NULL) {

      stream_name = std::string ((const char *) stream->GetMediaFormat ().GetEncodingName ());
      std::transform (stream_name.begin (), stream_name.end (), stream_name.begin (), (int (*) (int)) toupper);
      paused = stream->IsPaused ();
      stream->SetPaused (!paused);

      if (paused)
        Ekiga::Runtime::run_in_main (boost::bind (boost::ref (stream_resumed), this->shared_from_this (), stream_name, type));
      else
        Ekiga::Runtime::run_in_main (boost::bind (boost::ref (stream_paused), this->shared_from_this (), stream_name, type));
    }
  }
}


void
Opal::Call::send_dtmf (const char dtmf)
{
  PSafePtr<OpalConnection> connection = GetConnection ();
  if (connection != NULL) {
    connection->SendUserInputTone (dtmf, 180);
  }
}


void
Opal::Call::set_forward_target (const std::string & _forward_uri)
{
  forward_uri = _forward_uri;
}


const std::string
Opal::Call::get_id () const
{
  return GetToken ();
}


const std::string
Opal::Call::get_local_party_name () const
{
  return local_party_name;
}


const std::string
Opal::Call::get_remote_party_name () const
{
  return remote_party_name;
}


const std::string
Opal::Call::get_remote_application () const
{
  return remote_application;
}


const std::string
Opal::Call::get_remote_uri () const
{
  return remote_uri;
}


const std::string
Opal::Call::get_duration () const
{
  std::stringstream duration;

  if (start_time.IsValid () && IsEstablished ()) {

    PTimeInterval t = PTime () - start_time;

    duration << setfill ('0') << setw (2) << t.GetHours () << ":";
    duration << setfill ('0') << setw (2) << (t.GetMinutes () % 60) << ":";
    duration << setfill ('0') << setw (2) << (t.GetSeconds () % 60);
  }

  return duration.str ();
}


time_t
Opal::Call::get_start_time () const
{
  return start_time.GetTimeInSeconds ();
}


const RTCPStatistics &
Opal::Call::get_statistics ()
{
  PSafePtr<OpalConnection> connection = GetConnection ();
  if (connection == NULL)
    return statistics;

  OpalMediaStreamPtr stream;
  stream = connection->GetMediaStream (OpalMediaType::Audio (), false);  // transmission
  if (stream) {
    tr_a_statistics.Update (*stream);
    // GetBitRate is the average bit rate on the last second
    statistics.transmitted_audio_bandwidth  = tr_a_statistics.GetBitRate () / 1024;
    statistics.jitter = tr_a_statistics.m_averageJitter;
  }

  stream = connection->GetMediaStream (OpalMediaType::Audio (), true);  // reception
  if (stream) {
    re_a_statistics.Update (*stream);
    statistics.received_audio_bandwidth  = re_a_statistics.GetBitRate () / 1024;
    statistics.remote_jitter = re_a_statistics.m_averageJitter;
  }

  for (PINDEX i = 0 ; KnownCodecs[i][0] ; i++) {
    if (tr_a_statistics.m_mediaFormat == KnownCodecs[i][0])
      statistics.transmitted_audio_codec = gettext (KnownCodecs[i][1]);
    if (re_a_statistics.m_mediaFormat == KnownCodecs[i][0])
      statistics.received_audio_codec = gettext (KnownCodecs[i][1]);
  }

  // 100 * number of lost packets / by number of packets, on the last second
  if (re_a_statistics.GetPacketRate () + re_v_statistics.GetPacketRate () != 0)
    statistics.lost_packets = 100 * (re_a_statistics.GetLossRate () + re_v_statistics.GetLossRate ()) / (re_a_statistics.GetPacketRate () + re_v_statistics.GetPacketRate ());
  if (tr_a_statistics.GetPacketRate () + tr_v_statistics.GetPacketRate () != 0)
    statistics.remote_lost_packets = 100 * (tr_a_statistics.GetLossRate () + tr_v_statistics.GetLossRate ()) / (tr_a_statistics.GetPacketRate () + tr_v_statistics.GetPacketRate ());

  return statistics;
}


bool
Opal::Call::is_outgoing () const
{
  return outgoing;
}


// if the parameter is not valid utf8, remove from it all the chars
//   after the first invalid utf8 char, so that it becomes valid utf8
static void
make_valid_utf8 (string & str)
{
  const char *pos;
  if (!g_utf8_validate (str.c_str(), -1, &pos)) {
    PTRACE (4, "Ekiga\tTrimming invalid UTF-8 string: " << str.c_str());
    str = str.substr (0, pos - str.c_str()).append ("...");
  }
}


void
Opal::Call::parse_info (OpalConnection & connection)
{
  char start_special_chars [] = "$";
  char end_special_chars [] = "([;=";

  std::string l_party_name;
  std::string r_party_name;
  std::string app;

  if (!PIsDescendant(&connection, OpalPCSSConnection)) {

    remote_uri = (const char *) connection.GetRemotePartyURL ();

    l_party_name = (const char *) connection.GetLocalPartyName ();
    r_party_name = (const char *) connection.GetRemotePartyName ();
    app = (const char *) connection.GetRemoteProductInfo ().AsString ();
    start_time = connection.GetConnectionStartTime ();
    if (!start_time.IsValid ())
      start_time = PTime ();

    if (!l_party_name.empty ())
      local_party_name = (const char *) SIPURL (l_party_name).GetUserName ();
    if (!r_party_name.empty ())
      remote_party_name = r_party_name;
    if (!app.empty ())
      remote_application = app;

    make_valid_utf8 (remote_party_name);
    make_valid_utf8 (remote_application);
    make_valid_utf8 (remote_uri);

    strip_special_chars (remote_party_name, end_special_chars, false);
    strip_special_chars (remote_application, end_special_chars, false);
    strip_special_chars (remote_uri, end_special_chars, false);

    strip_special_chars (remote_party_name, start_special_chars, true);
    strip_special_chars (remote_uri, start_special_chars, true);
  }
}


PSafePtr<OpalConnection>
Opal::Call::GetConnection ()
{
  PSafePtr<OpalConnection> connection;
  for (PSafePtr<OpalConnection> iter (connectionsActive, PSafeReference); iter != NULL; ++iter) {

    if (PSafePtrCast<OpalConnection, OpalPCSSConnection> (iter) == NULL) {
      connection = iter;
      if (!connection.SetSafetyMode (PSafeReadWrite))
        connection.SetNULL();
      break;
    }
  }

  return connection;
}


PBoolean
Opal::Call::OnEstablished (OpalConnection & connection)
{
  OpalMediaStreamPtr stream;

  noAnswerTimer.Stop (false);

  if (!PIsDescendant(&connection, OpalPCSSConnection)) {

    add_action (Ekiga::ActionPtr (new Ekiga::Action ("hold", _("Hold"),
                                                     boost::bind (&Call::toggle_hold, this))));
    add_action (Ekiga::ActionPtr (new Ekiga::Action ("transfer", _("Transfer"),
                                                     boost::bind (&Call::transfer, this))));
    remove_action ("answer");
    remove_action ("reject");

    parse_info (connection);
    Ekiga::Runtime::run_in_main (boost::bind (boost::ref (established), this->shared_from_this ()));
  }

  return OpalCall::OnEstablished (connection);
}


void
Opal::Call::OnReleased (OpalConnection & connection)
{
  parse_info (connection);

  OpalCall::OnReleased (connection);
}


void
Opal::Call::OnCleared ()
{
  std::string reason;

  noAnswerTimer.Stop (false);

  OpalCall::OnCleared ();

    switch (GetCallEndReason ()) {

    case OpalConnection::EndedByAnswerDenied:
    case OpalConnection::EndedByRefusal:
    case OpalConnection::EndedByNoAccept:
    case OpalConnection::EndedByLocalBusy:
    case OpalConnection::EndedByRemoteBusy:
      reason = _("Call rejected");
      break;

    case OpalConnection::EndedByCallerAbort:
      reason = _("Call canceled");
      break;
    case OpalConnection::EndedByLocalCongestion:
    case OpalConnection::EndedByRemoteCongestion:
    case OpalConnection::EndedByConnectFail:
    case OpalConnection::EndedByTransportFail:
    case OpalConnection::EndedByHostOffline:
    case OpalConnection::EndedByUnreachable:
    case OpalConnection::EndedByNoEndPoint:
    case OpalConnection::EndedByOutOfService:
    case OpalConnection::EndedByNoDialTone:
    case OpalConnection::EndedByNoRingBackTone:
      reason = _("Abnormal call termination");
      break;
    case OpalConnection::EndedByTemporaryFailure:
      reason = _("User is offline");
      break;
    case OpalConnection::EndedBySecurityDenial:
    case OpalConnection::EndedByGatekeeper:
    case OpalConnection::EndedByGkAdmissionFailed:
      reason = _("Call forbidden");
      break;
    case OpalConnection::EndedByCertificateAuthority:
      reason = _("Remote certificate not authenticated");
      break;
    case OpalConnection::EndedByNoUser:
      reason = _("Wrong number or address");
      break;
    case OpalConnection::EndedByIllegalAddress:
      reason = _("Invalid number or address");
      break;
    case OpalConnection::EndedByNoBandwidth:
      reason = _("Insufficient bandwidth");
      break;
    case OpalConnection::EndedByCapabilityExchange:
    case OpalConnection::EndedByMediaFailed:
      reason = _("No common codec");
      break;
    case OpalConnection::EndedByCallForwarded:
      reason = _("Call forwarded");
      break;
    case OpalConnection::EndedByNoAnswer:
      reason = _("No answer");
      break;
    case OpalConnection::EndedByLocalUser:
    case OpalConnection::EndedByRemoteUser:
    case OpalConnection::EndedByCustomCode:
    case OpalConnection::EndedByQ931Cause:
    case OpalConnection::EndedByDurationLimit:
    case OpalConnection::EndedByInvalidConferenceID:
    case OpalConnection::EndedByAcceptingCallWaiting:
    case OpalConnection::EndedByCallCompletedElsewhere:
    case OpalConnection::NumCallEndReasons:
    default:
      reason = _("Call completed");
    }

    if (IsEstablished () || is_outgoing ())
      Ekiga::Runtime::run_in_main (boost::bind (boost::ref (cleared), this->shared_from_this (), reason));
    else
      Ekiga::Runtime::run_in_main (boost::bind (boost::ref (missed), this->shared_from_this ()));
}


OpalConnection::AnswerCallResponse
Opal::Call::OnAnswerCall (OpalConnection & connection,
                          const PString & caller)
{
  remote_party_name = (const char *) caller;

  parse_info (connection);

  /*
  if (auto_answer)
    return OpalConnection::AnswerCallNow;
*/
  std::cout << "FIXME" << std::endl << std::flush;

  return OpalCall::OnAnswerCall (connection, caller);
}


PBoolean
Opal::Call::OnSetUp (OpalConnection & connection)
{
  outgoing = !IsNetworkOriginated ();
  parse_info (connection);

  call_setup = true;

  OpalCall::OnSetUp (connection);
  Ekiga::Runtime::run_in_main (boost::bind (boost::ref (setup),
                                            this->shared_from_this ()));

  return true;
}


PBoolean
Opal::Call::OnAlerting (OpalConnection & connection)
{
  if (!PIsDescendant(&connection, OpalPCSSConnection))
    Ekiga::Runtime::run_in_main (boost::bind (boost::ref (ringing), this->shared_from_this ()));

  return OpalCall::OnAlerting (connection);
}


void
Opal::Call::OnHold (OpalConnection & /*connection*/,
                    bool /*from_remote*/,
                    bool on_hold)
{
  if (on_hold)
    Ekiga::Runtime::run_in_main (boost::bind (boost::ref (held), this->shared_from_this ()));
  else
    Ekiga::Runtime::run_in_main (boost::bind (boost::ref (retrieved), this->shared_from_this ()));
}


void
Opal::Call::OnOpenMediaStream (OpalMediaStream & stream)
{
  StreamType type = Audio;
  bool is_transmitting = false;
  std::string stream_name;

  stream_name = std::string ((const char *) stream.GetMediaFormat ().GetEncodingName ());
  std::transform (stream_name.begin (), stream_name.end (), stream_name.begin (), (int (*) (int)) toupper);
  is_transmitting = !stream.IsSource ();

  Ekiga::Runtime::run_in_main (boost::bind (boost::ref (stream_opened), this->shared_from_this (), stream_name, type, is_transmitting));
}


void
Opal::Call::OnClosedMediaStream (OpalMediaStream & stream)
{
  StreamType type = Audio;
  bool is_transmitting = false;
  std::string stream_name;

  stream_name = std::string ((const char *) stream.GetMediaFormat ().GetEncodingName ());
  std::transform (stream_name.begin (), stream_name.end (), stream_name.begin (), (int (*) (int)) toupper);
  is_transmitting = !stream.IsSource ();

  Ekiga::Runtime::run_in_main (boost::bind (boost::ref (stream_closed), this->shared_from_this (), stream_name, type, is_transmitting));
}


void
Opal::Call::DoSetUp (OpalConnection & connection)
{
  OpalCall::OnSetUp (connection);
}


void
Opal::Call::OnNoAnswerTimeout (PTimer &,
                               INT)
{
  if (!forward_uri.empty ()) {

    PSafePtr<OpalConnection> connection = GetConnection ();
    if (connection != NULL)
      connection->ForwardCall (forward_uri);
  }
  else
    Clear (OpalConnection::EndedByNoAnswer);
}
