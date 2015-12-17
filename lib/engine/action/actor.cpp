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
 *                         actor.cpp  -  description
 *                         -------------------------
 *   begin                : written in February 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An engine actor.
 *
 */

#include "actor.h"
#include "runtime.h"

using namespace Ekiga;


void
Actor::add_action (ActionPtr action)
{
  remove_action (action->get_name ()); // Remove any other action with the same name.

  actions.push_back (action);

  conns.add (action->enabled.connect (boost::bind (boost::ref (action_enabled), action->get_name ())));
  conns.add (action->disabled.connect (boost::bind (boost::ref (action_disabled), action->get_name ())));

  Ekiga::Runtime::run_in_main (boost::bind (boost::ref(action_added), action->get_name ()));
}


bool
Actor::remove_action (const std::string & name)
{
  ActionPtr a = get_action (name);
  if (!a)
    return false;

  Ekiga::Runtime::run_in_main (boost::bind (boost::ref(action_removed), name));
  actions.remove (a);

  return true;
}


bool
Actor::enable_action (const std::string & name)
{
  ActionPtr a = get_action (name);
  if (!a)
    return false;

  a->enable ();

  return true;
}


bool
Actor::disable_action (const std::string & name)
{
  ActionPtr a = get_action (name);
  if (!a)
    return false;

  a->disable ();

  return true;
}


ActionPtr
Actor::get_action (const std::string & name)
{
  for (iterator it = begin (); it != end () ; ++it) {
    if ((*it)->get_name () == name) {
      return (*it);
    }
  }

  return ActionPtr ();
}


void
Actor::remove_actions ()
{
  for (iterator it = begin (); it != end () ; ++it) {
    action_removed ((*it)->get_name ());
  }
  actions.clear ();
}


Actor::iterator
Actor::begin ()
{
  return actions.begin ();
}


Actor::iterator
Actor::end ()
{
  return actions.end ();
}


Actor::const_iterator
Actor::begin () const
{
  return actions.begin ();
}


Actor::const_iterator
Actor::end () const
{
  return actions.end ();
}
