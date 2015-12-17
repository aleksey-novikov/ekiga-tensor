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
 *                         friend-or-foe.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2009 by Julien Puydt
 *   copyright            : (c) 2009 by Julien Puydt
 *   description          : interface of the main IFF object
 *
 */

#ifndef __FRIEND_OR_FOE_H__
#define __FRIEND_OR_FOE_H__

/* IFF stands for "Identification Friend or Foe" and is a military system
 * used to determine if an aircraft is a friend or a foe.
 *
 * In ekiga, there's no aircraft in sight, and the question isn't that of
 * shooting or not : the problem is to determine if an incoming call should
 * be rejected, accepted... or if we ring to let the user decide.
 *
 * Since we want several pieces of code to be able to provide information,
 * we add in a helper class, which the main Ekiga::FriendOrFoe object uses
 * to determines its answer. And since we want to be extensible, the question
 * depends on a domain : "call", "message", etc.
 *
 * The code which gets a determination by Ekiga::FriendOrFoe can of course do
 * whatever it wants with the answer!
 */

#include "services.h"
#include "actor.h"
#include "action-provider.h"

namespace Ekiga
{
  class FriendOrFoe:
      public Ekiga::Service,
      public virtual Actor
  {
  public:

    /* beware of the order : we prefer erring on the side of safety */
    typedef enum { Unknown, Foe, Neutral, Friend } Identification;

    class Helper : public URIActionProvider, public Actor
    {
    friend class FriendOrFoe;
    public:
      virtual ~Helper ()
      {}

      virtual Identification decide (const std::string domain,
                                     const std::string token) = 0;

    protected:
      virtual void pull_actions (Actor & actor,
                                 const std::string & display_name,
                                 const std::string & uri) = 0;
    };

    Identification decide (const std::string domain,
                           const std::string token);

    void add_helper (boost::shared_ptr<Helper> helper);

    /* this turns us into a service */
    const std::string get_name () const
    { return "friend-or-foe"; }

    const std::string get_description () const
    { return "\tObject helping determine if an incoming call is acceptable"; }

  private:
    typedef std::list<boost::shared_ptr<Helper> > helpers_type;
    helpers_type helpers;
  };
};

#endif
