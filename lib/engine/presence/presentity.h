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
 *                         presentity.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : declaration of a presence entity
 *
 */

#ifndef __PRESENTITY_H__
#define __PRESENTITY_H__

#include <set>
#include <string>

#include "actor.h"

namespace Ekiga
{

/**
 * @addtogroup presence
 * @{
 */

  /** A presentity is a piece of presence information for a single URI.
   */
  class Presentity: public virtual Actor
  {
  public:

    /** The destructor.
     */
    virtual ~Presentity () {}

    /** Returns the name of the Presentity.
     * @return The Presentity's name.
     */
    virtual const std::string get_name () const = 0;

    /** Returns the presence of the Presentity.
    * @return The Presentity's presence.
    */
    virtual const std::string get_presence () const = 0;

    /** Returns the status of the Presentity.
     * @return The Presentity's status.
     */
    virtual const std::string get_status () const = 0;

    /** Returns the set of groups the Presentity belongs to.
     * @return The Presentity's list of groups.
     */
    virtual const std::list<std::string> get_groups () const = 0;

    /** Returns whether the Presentity has this uri.
     * @return Whether the Presentity has this uri.
     */
    virtual bool has_uri (const std::string uri) const = 0;
  };

  typedef boost::shared_ptr<Presentity> PresentityPtr;
/**
 * @}
 */
};

#endif
