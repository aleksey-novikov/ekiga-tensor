/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
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
 *                         action.cpp  -  description
 *                         --------------------------
 *   begin                : written in February 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An engine action.
 *
 */

#include "action.h"

using namespace Ekiga;


Action::Action (const std::string & _name,
                const std::string & _description,
                boost::function0<void> _callback,
                bool _enabled) : name (_name), description (_description), callback (_callback), action_enabled (_enabled)
{
  if (!callback.empty ())
    conns.add (activated.connect (boost::cref (callback)));
}


Action::~Action ()
{
}

const std::string &
Action::get_name () const
{
  return name;
}



const std::string &
Action::get_description () const
{
  return description;
}



void
Action::activate ()
{
  activated ();
}


void
Action::enable ()
{
  action_enabled = true;
  enabled ();
}


void
Action::disable ()
{
  action_enabled = false;
  disabled ();
}


bool
Action::is_enabled () const
{
  return action_enabled;
}
