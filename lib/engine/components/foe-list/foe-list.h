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
 *                         foe-list.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2014 by Julien Puydt
 *   copyright            : (c) 2014 by Julien Puydt
 *   description          : interface of a delegate
 *
 */

#ifndef __FOE_LIST_H__
#define __FOE_LIST_H__

#include "contact-core.h"
#include "friend-or-foe.h"

namespace Ekiga
{

  class FoeList:
    public Service,
    public FriendOrFoe::Helper
  {

  public:
    FoeList(boost::shared_ptr<FriendOrFoe> friend_or_foo);

    ~FoeList();

    /* Ekiga::Service api */

    const std::string get_name () const
    { return "foe-list"; }

    const std::string get_description () const
    { return "List of persons the user does not want to hear about"; }


    /* Ekiga::FriendOrFoe::Helper api */
    FriendOrFoe::Identification decide (const std::string domain,
					const std::string token);

    /* specific api */
    void add_foe (const std::string token);

  private:
    void edit_foes ();
    bool on_edit_foes_form_submitted (bool submitted,
                                      Ekiga::Form& result,
                                      G_GNUC_UNUSED std::string& error);

    /* ActionProvider api */
    void pull_actions (Actor & actor,
                       const std::string & display_name,
                       const std::string & uri);


    // beware of dependency loops!
    boost::weak_ptr<FriendOrFoe> friend_or_foe;
  };
};

#endif
