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
 *                         notification-core.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2009 by Julien Puydt
 *   copyright            : (c) 2009 by Julien Puydt
 *   description          : declaration of the interface for user notifications
 *
 */

#ifndef __NOTIFICATION_CORE_H__
#define __NOTIFICATION_CORE_H__

#include "services.h"

#include <boost/smart_ptr.hpp>

namespace Ekiga
{
  /* the following class is mostly a trivial structure, but it comes
   * with a decent memory management and a signal to know if it's still
   * there
   */
  class Notification
  {
  public:

    typedef enum { Info, Warning, Error } NotificationLevel;

    Notification (NotificationLevel level_,
                  const std::string title_,
                  const std::string body_,
                  const std::string action_name_ = "",
                  boost::function0<void> action_callback_ = NULL)
      : level(level_), title(title_), body(body_), action_name(action_name_), action_callback(action_callback_)
    {}

    ~Notification () {}

    NotificationLevel get_level () const
    { return level; }

    const std::string get_title () const
    { return title; }

    const std::string get_body () const
    { return body; }

    const std::string get_action_name () const
    { return action_name; }

    void action_trigger ()
    { if (action_callback) action_callback (); }

    boost::signals2::signal<void(void)> removed;

  private:

    NotificationLevel level;
    std::string title;
    std::string body;
    std::string action_name;
    boost::function0<void> action_callback;
  };

  class NotificationCore: public Service
  {
  public:

    /* First the boilerplate methods */

    NotificationCore () {}

    ~NotificationCore () {}

    const std::string get_name () const
    { return "notification-core"; }

    const std::string get_description () const
    { return "\tCentral notification object"; }

    /*** Public API ***/

    void push_notification (boost::shared_ptr<Notification> notification)
    { notification_added (notification); }

    boost::signals2::signal<void(boost::shared_ptr<Notification>)> notification_added;
  };
};

#endif
