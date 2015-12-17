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
 *                         codec-description.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in January 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of the interface of a codec description.
 *
 */

#include <cstdlib>
#include <sstream>

#include "codec-description.h"


using namespace Ekiga;

bool same_codec_description (CodecDescription a, CodecDescription b)
{
  return (a == b);
}


CodecDescription::CodecDescription ()
  : rate (0), active (true), audio (false)
{
}


CodecDescription::CodecDescription (const std::string & _name,
                                    unsigned _rate,
                                    bool _audio,
                                    std::string _protocols,
                                    bool _active)
  : name (_name), rate (_rate), active (_active), audio (_audio)
{
  gchar** prots = NULL;

  prots = g_strsplit (_protocols.c_str (), ", ", -1);

  for (gchar** ptr = prots;
       *ptr != NULL;
       ptr++) {

    if ((*ptr)[0] != '\0') { // not the empty string

      protocols.push_back (*ptr);
    }
  }

  g_strfreev (prots);

  protocols.unique ();
  protocols.sort ();
}


std::string
CodecDescription::str ()
{
  std::stringstream val;

  val << name << ":" << (active ? "1" : "0");

  return val.str ();
}


bool
CodecDescription::operator== (const CodecDescription & c) const
{
  CodecDescription d = c;
  CodecDescription e = (*this);

  return (e.name == d.name);
}


bool
CodecDescription::operator!= (const CodecDescription & c) const
{
  return (!((*this) == c));
}


void
CodecList::append (const CodecList& other)
{
  insert (end (), other.begin (), other.end ());
  unique (same_codec_description);
}


void
CodecList::append (const CodecDescription& descr)
{
  push_back (descr);
}


bool
CodecList::find (const std::string & display_name)
{
  for (iterator it = begin ();
       it != end ();
       it++) {

    if ((*it).display_name == display_name)
      return true;
  }

  return false;
}

void
CodecList::remove (iterator it)
{
  erase (it);
}


CodecList
CodecList::get_audio_list ()
{
  CodecList result;

  for (iterator it = begin ();
       it != end ();
       it++) {

    if ((*it).audio)
      result.push_back (*it);
  }

  return result;
}


std::list<std::string>
CodecList::slist ()
{
  std::list<std::string> result;

  for (iterator it = begin ();
       it != end ();
       it++) {

    result.push_back ((*it).str ());
  }

  return result;
}


bool
CodecList::operator== (const CodecList & c) const
{
  CodecList::const_iterator it2 = c.begin ();

  if (size () != c.size ())
    return false;

  for (const_iterator it = begin ();
       it != end ();
       it++) {

    if ((*it) != (*it2))
      return false;

    it2++;
  }

  return true;
}


bool
CodecList::operator!= (const CodecList & c) const
{
  return (!(*this == c));
}


std::ostream&
operator<< (std::ostream & os, const CodecList & c)
{
  std::stringstream str;
  for (CodecList::const_iterator it = c.begin ();
       it != c.end ();
       it++) {

    if (it != c.begin ())
      str << " ; ";

    str << (*it).name;
  }

  return os << str.str ();
}
