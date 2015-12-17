/* Ekiga -- A VoIP and Video-Conferencing application
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
 *                         opal-call-manager.cpp  -  description
 *                         -------------------------------------
 *   begin                : Sat Dec 23 2000
 *   authors              : Damien Sandras
 *   description          : This file contains the engine CallManager.
 *
 */

#include "config.h"

#include <glib/gi18n.h>

#include "opal-call-manager.h"
#include "null-deleter.h"

#include "call-core.h"

/* The engine class */
Opal::CallManager::CallManager (Ekiga::ServiceCore& _core,
                                Opal::EndPoint& _endpoint) : core(_core), endpoint(_endpoint)
{
  /* Setup things */
  Ekiga::SettingsCallback setup_cb = boost::bind (&Opal::CallManager::setup, this, _1);
  audio_codecs_settings = Ekiga::SettingsPtr (new Ekiga::Settings (AUDIO_CODECS_SCHEMA, setup_cb));
  ports_settings = Ekiga::SettingsPtr (new Ekiga::Settings (PORTS_SCHEMA, setup_cb));
  protocols_settings = Ekiga::SettingsPtr (new Ekiga::Settings (PROTOCOLS_SCHEMA, setup_cb));
  call_options_settings = Ekiga::SettingsPtr (new Ekiga::Settings (CALL_OPTIONS_SCHEMA, setup_cb));
  call_forwarding_settings = Ekiga::SettingsPtr (new Ekiga::Settings (CALL_FORWARDING_SCHEMA, setup_cb));
  personal_data_settings = Ekiga::SettingsPtr (new Ekiga::Settings (PERSONAL_DATA_SCHEMA, setup_cb));

  set_display_name (g_get_real_name ());
}


Opal::CallManager::~CallManager ()
{
}


void Opal::CallManager::hang_up ()
{
  endpoint.ClearAllCalls (OpalConnection::EndedByLocalUser, FALSE);
}


void Opal::CallManager::set_reject_delay (unsigned delay)
{
  endpoint.SetNoAnswerDelay (delay);
}


unsigned Opal::CallManager::get_reject_delay () const
{
  return endpoint.GetNoAnswerDelay ();
}


void Opal::CallManager::set_auto_answer (bool enabled)
{
  endpoint.SetAutoAnswer (enabled);
}


bool Opal::CallManager::get_auto_answer () const
{
  return endpoint.GetAutoAnswer ();
}


void Opal::CallManager::set_display_name (const std::string & name)
{
  display_name = name;
  endpoint.SetDefaultDisplayName (name);
}


const std::string & Opal::CallManager::get_display_name () const
{
  return display_name;
}


void Opal::CallManager::set_codecs (Ekiga::CodecList & _codecs)
{
  PStringArray mask, order;
  OpalMediaFormatList formats;
  OpalMediaFormat::GetAllRegisteredMediaFormats (formats);

  codecs = _codecs;

  for (Ekiga::CodecList::const_iterator iter = codecs.begin ();
       iter != codecs.end ();
       iter++)
    if ((*iter).active)
      order += (*iter).name;

  formats.Remove (order);

  for (int i = 0 ; i < formats.GetSize () ; i++)
    mask += (const char *) formats[i];

  endpoint.SetMediaFormatOrder (order);
  endpoint.SetMediaFormatMask (mask);
  PTRACE (4, "Opal::CallManager\tSet codecs: " << setfill(';') << endpoint.GetMediaFormatOrder ());
  PTRACE (4, "Opal::CallManager\tDisabled codecs: " << setfill(';') << endpoint.GetMediaFormatMask ());
}


const Ekiga::CodecList& Opal::CallManager::get_codecs () const
{
  return codecs;
}


void Opal::CallManager::set_echo_cancellation (bool enabled)
{
  endpoint.SetEchoCancellation (enabled);
}


bool Opal::CallManager::get_echo_cancellation () const
{
  return endpoint.GetEchoCancellation ();
}


void Opal::CallManager::set_silence_detection (bool enabled)
{
  endpoint.SetSilenceDetection (enabled);
}


bool Opal::CallManager::get_silence_detection () const
{
  return endpoint.GetSilenceDetection ();
}


void Opal::CallManager::setup (const std::string & setting)
{
  std::cout << "IN Opal::CallManager::setup" << std::endl;

  if (setting.empty () || setting == "enable-silence-detection")
    set_silence_detection (audio_codecs_settings->get_bool ("enable-silence-detection"));

  if (setting.empty () || setting == "enable-echo-cancellation")
    set_echo_cancellation (audio_codecs_settings->get_bool ("enable-echo-cancellation"));

  if (setting.empty () || setting == "rtp-tos-field")
    endpoint.SetMediaTypeOfService (protocols_settings->get_int ("rtp-tos-field"));

  if (setting.empty () || setting == "no-answer-timeout")
    set_reject_delay (call_options_settings->get_int ("no-answer-timeout"));

  if (setting.empty () || setting == "auto-answer")
    set_auto_answer (call_options_settings->get_bool ("auto-answer"));

  if (setting.empty () || setting == "media-list") {

    std::list<std::string> config_codecs = audio_codecs_settings->get_string_list ("media-list");

    // This will add all supported codecs that are not present in the configuration
    // at the end of the list.
    Opal::CodecList all_codecs;
    all_codecs.load (config_codecs);

    // Update the manager codecs
    set_codecs (all_codecs);
  }

  if (setting.empty () || setting == "udp-port-range") {

    int min_port, max_port = 0;
    ports_settings->get_int_tuple ("udp-port-range", min_port, max_port);
    if (min_port < max_port) {
      endpoint.SetUDPPorts (min_port, max_port);
      endpoint.SetRtpIpPorts (min_port, max_port);
    }
  }

  if (setting.empty () || setting == "tcp-port-range") {

    int min_port, max_port = 0;
    ports_settings->get_int_tuple ("tcp-port-range", min_port, max_port);
    if (min_port < max_port) {
      endpoint.SetTCPPorts (min_port, max_port);
    }
  }
}
