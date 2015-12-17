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
 *                         action-provider.h  -  description
 *                         ---------------------------------
 *   begin                : written in February 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An engine action provider.
 *
 */

#ifndef __ACTION_PROVIDER_H__
#define __ACTION_PROVIDER_H__

#include <string>

#include "scoped-connections.h"

#include "actor.h"

namespace Ekiga {

  /**
   * @defgroup actions ActionProvider
   * @{
   */

  /* An ActionProvider is an object able to provide Actions to
   * Actors.
   */
  class ActionProvider
  {
  public:
    virtual void pull_actions (Actor & /*actor*/) {};
  protected:
    virtual void add_action (Actor & actor,
                             ActionPtr action);
    virtual void remove_action (Actor & actor,
                                const std::string & action);
  };
  typedef boost::shared_ptr< ActionProvider > ActionProviderPtr;

  class URIActionProvider : public ActionProvider
  {
  public:
    virtual void pull_actions (Actor & /*actor*/,
                               const std::string & /*display_name*/,
                               const std::string & /*uri*/) = 0;
  };
  typedef boost::shared_ptr< URIActionProvider > URIActionProviderPtr;

  class URIActionProviderStore : public std::list< URIActionProviderPtr >
  {
  public:
    virtual void pull_actions (Actor & /*actor*/,
                               const std::string & /*display_name*/,
                               const std::string & /*uri*/);
  };


  /**
   * @}
   */
}

#endif
