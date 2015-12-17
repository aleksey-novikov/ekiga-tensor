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
 *                         actor.h  -  description
 *                         -----------------------
 *   begin                : written in February 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An engine actor.
 *
 */

#ifndef __ACTOR_H__
#define __ACTOR_H__


#include "action.h"
#include "scoped-connections.h"
#include "chain-of-responsibility.h"
#include "form-request.h"

#include <string>

namespace Ekiga {

  /**
   * @defgroup actions Actor
   * @{
   */


  /* An actor is an object able to execute Actions.
   *
   * An Actor can register actions through the add_action method.
   * It can remove them using the remove_action and remove_actions methods.
   *
   */
  class Actor
  {
    friend class ActionProvider;

  public:
    typedef std::list < ActionPtr >::const_iterator const_iterator;
    typedef std::list < ActionPtr >::iterator iterator;

    /**
     * Those signals are emitted when an Action is enabled/disabled
     * in the ActionMap.
     */
    boost::signals2::signal<void(const std::string &)> action_enabled;
    boost::signals2::signal<void(const std::string &)> action_disabled;


    /**
     * Those signals are emitted when an Action is added/removed
     * to/from the ActionMap.
     */
    boost::signals2::signal<void(const std::string &)> action_added;
    boost::signals2::signal<void(const std::string &)> action_removed;


    /** Returns an iterator to the first Action of the collection
     */
    iterator begin ();

    /** Returns an iterator to the last Action of the collection
     */
    iterator end ();

    /** Returns a const iterator to the first Action of the collection
     */
    const_iterator begin () const;

    /** Returns a const iterator to the last Action of the collection
     */
    const_iterator end () const;

    /** This chain allows the object to present forms to the user
     */
    ChainOfResponsibility<FormRequestPtr> questions;

  protected:
    /** Add an action to the given Actor.
     *
     * Actions that are not "added" using this method will not be usable
     * from menus.
     *
     * @param An Action.
     */
    virtual void add_action (ActionPtr action);


    /** Remove an action from the given Actor.
     *
     * @param An Action name.
     * @return true if the Action was successfully removed, false otherwise.
     */
    virtual bool remove_action (const std::string & name);


    /** Enable an action from the given Actor.
     *
     * @param An Action name.
     * @return true if the Action was successfully enabled, false otherwise.
     */
    virtual bool enable_action (const std::string & name);


    /** Disable an action from the given Actor.
     *
     * @param An Action name.
     * @return true if the Action was successfully disabled, false otherwise.
     */
    virtual bool disable_action (const std::string & name);


    /** Remove all actions from the given Actor.
     *
     */
    virtual void remove_actions ();


    /** Return an action by name.
     *
     * @param An Action name.
     * @return A smart pointer to the Action if it was found. An empty pointer
     *         otherwise.
     */
    virtual ActionPtr get_action (const std::string & name);


  private:

    /**
     * This is the Actor ActionStore.
     * It contains all actions supported by the current Actor.
     */
    Ekiga::scoped_connections conns;
    std::list< ActionPtr > actions; // FIXME: Use a RefLister here.
  };
  typedef boost::shared_ptr< Actor > ActorPtr;

  /**
   * @}
   */
}

#endif

