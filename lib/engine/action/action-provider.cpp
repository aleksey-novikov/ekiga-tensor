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
 *                         action-provider.cpp  -  description
 *                         -----------------------------------
 *   begin                : written in February 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An engine action provider.
 *
 */

#include "action-provider.h"

using namespace Ekiga;

void
ActionProvider::add_action (Actor & actor,
                            ActionPtr action)
{
  actor.add_action (action);
}


void
ActionProvider::remove_action (Actor & actor,
                               const std::string & action)
{
  actor.remove_action (action);
}


void
URIActionProviderStore::pull_actions (Actor & actor,
                                      const std::string & name,
                                      const std::string & uri)
{
  for (URIActionProviderStore::iterator it = begin (); it != end (); it++) {
    (*it)->pull_actions (actor, name, uri);
  }
}
