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
 *                         opal-codec-description.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in January 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : Opal codec description.
 *
 */

#include <glib/gi18n.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "opal-codec-description.h"
#include "known-codecs.h"


using namespace Opal;


CodecDescription::CodecDescription (const OpalMediaFormat & _format,
                                    bool _active)
{
  name = (const char *) _format;
  if (name == "G722")  // G722 has the wrong rate in RFC
    rate = 16000;
  else
    rate = _format.GetClockRate ();
  audio = (_format.GetMediaType () == OpalMediaType::Audio ());
  video = (_format.GetMediaType () == OpalMediaType::Video ());
  if (_format.IsValidForProtocol ("SIP"))
    protocols.push_back ("SIP");
  protocols.sort ();
  for (PINDEX i = 0 ; KnownCodecs[i][0] ; i++) {
    if (name == KnownCodecs[i][0]) {
      display_name = gettext (KnownCodecs[i][1]);
      display_info = gettext (KnownCodecs[i][2]);
      break;
    }
  }
  if (display_name.empty ())
    display_name = name;

  format = _format;
  active = _active;
}


void
CodecList::load (const std::list<std::string> & codecs_config)
{
  OpalMediaFormatList formats;
  GetAllowedFormats (formats);

  clear ();

  // FIXME: This is not very efficient.
  // We add each codec of the string list to our own internal list
  for (std::list<std::string>::const_iterator iter = codecs_config.begin ();
       iter != codecs_config.end ();
       iter++) {

    std::vector<std::string> strs;
    boost::split (strs, *iter, boost::is_any_of (":"));

    for (int i = 0 ; i < formats.GetSize () ; i++) {
      // Found our codec in the formats, add it to our
      // internal lists
      if (strs[0] == (const char *) formats[i]) {
        CodecDescription d (formats[i], (strs[1] == "1"));
        append (d);
        formats -= formats[i];
        break;
      }
    }
  }

  // We will now add codecs which were not part of the codecs_config
  // list but that we support (ie all codecs from "list").
  for (int i = 0 ; i < formats.GetSize () ; i++) {
    CodecDescription d (formats[i], false);
    append (d);
  }
}


void
CodecList::GetAllowedFormats (OpalMediaFormatList & formats)
{
  OpalMediaFormat::GetAllRegisteredMediaFormats (formats);
  formats.RemoveNonTransportable ();

  OpalMediaFormatList black_list;

  black_list += "Linear-16-Stereo-48kHz";
  black_list += "LPC-10";
  black_list += "Speex*";
  black_list += "FECC*";
  black_list += "RFC4175*";

  // Blacklist NSE, since it is unused in ekiga and might create
  // problems with some registrars (such as Eutelia)
  black_list += "NamedSignalEvent";

  // Only keep OPUS in mono mode (for VoIP chat)
  // and with the maximum sample rate
  black_list += "Opus-8*";
  black_list += "Opus-12*";
  black_list += "Opus-16*";
  black_list += "Opus-24*";
  black_list += "Opus-48S";

  // Only include the VP8 RFC version of the capability
  black_list += "VP8-OM";

  // Purge blacklisted codecs
  formats -= black_list;

  // Only keep audio and video codecs
  for (int i = 0 ; i < formats.GetSize () ; i++) {
    if (formats[i].GetMediaType () != OpalMediaType::Audio ()
        && formats[i].GetMediaType () != OpalMediaType::Video ())
      formats -= formats[i];
  }

  PTRACE(4, "Ekiga\tAll available audio & video media formats: " << setfill (',') << formats);
}
