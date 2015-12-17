/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>

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
 *                         friend-or-foe.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2009 by Julien Puydt
 *   copyright            : (c) 2009-2014 by Julien Puydt
 *   description          : interface of the main IFF object
 *
 */

#include "friend-or-foe.h"

Ekiga::FriendOrFoe::Identification
Ekiga::FriendOrFoe::decide (const std::string domain,
                            const std::string token)
{
  Identification answer = Unknown;
  Identification iter_answer;

  for (helpers_type::const_iterator iter = helpers.begin ();
       iter != helpers.end ();
       ++iter) {

    iter_answer = (*iter)->decide (domain, token);
    (*iter)->pull_actions (*this, std::string (), token);
    if (answer < iter_answer)
      answer = iter_answer;
  }

  return answer;
}

void
Ekiga::FriendOrFoe::add_helper (boost::shared_ptr<Ekiga::FriendOrFoe::Helper> helper)
{
  helpers.push_front (helper);
  helper->questions.connect (boost::ref (Ekiga::Actor::questions));
}
