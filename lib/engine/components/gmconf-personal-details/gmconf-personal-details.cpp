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
 *                         gmconf-personal-details.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of the representation of personal details
 *                          using gmconf
 *
 */

#include "gmconf-personal-details.h"

Gmconf::PersonalDetails::PersonalDetails ()
{
  personal_details = new Ekiga::Settings (PERSONAL_DATA_SCHEMA);
  personal_details->changed.connect (boost::bind (&PersonalDetails::setup, this, _1));
  display_name = g_get_real_name ();
  if (display_name.empty ())
    display_name = g_get_user_name ();

  setup ();
}

Gmconf::PersonalDetails::~PersonalDetails ()
{
  delete personal_details;
}

void
Gmconf::PersonalDetails::setup (std::string setting)
{
  std::string value;
  if (setting.empty () || setting == "short-status")  {
    value = personal_details->get_string ("short-status");
    if (value != presence) {
      presence = value;
      updated ();
    }
  }
  if (setting.empty () || setting == "long-status")  {
    value = personal_details->get_string ("long-status");
    if (value != status) {
      status = value;
      updated ();
    }
  }
}

const std::string
Gmconf::PersonalDetails::get_display_name () const
{
  return display_name;
}

const std::string
Gmconf::PersonalDetails::get_presence () const
{
  return presence;
}

const std::string
Gmconf::PersonalDetails::get_status () const
{
  return status;
}

void
Gmconf::PersonalDetails::set_display_name (G_GNUC_UNUSED const std::string display_name_)
{
  // Ignored
}

void
Gmconf::PersonalDetails::set_presence (const std::string presence_)
{
  personal_details->set_string ("short-status", presence_);
}

void
Gmconf::PersonalDetails::set_status (const std::string status_)
{
  personal_details->set_string ("long-status", status_);
}

void
Gmconf::PersonalDetails::set_presence_info (const std::string _presence,
                                            const std::string _status)
{
  presence = _presence;
  status = _status;

  set_presence (_presence);
  set_status (_status);

  updated ();
}
