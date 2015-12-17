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
 *                         account.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of the interface of an AccountManager
 *                          Account
 *
 */

#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include <set>
#include <map>
#include <string>

#include <boost/smart_ptr.hpp>
#include "actor.h"

namespace Ekiga
{

  /**
   * @addtogroup accounts
   * @{
   */

  class Account: public virtual Actor
  {
  public:

    /* Not all accounts have a notion of registration. However,
     * in that case, Registered will stand for "Active", and
     * Unregistered for "Inactive".
     *
     * Not all accounts need to go through all states.
     */
    typedef enum { Processing, Registered, Unregistered, RegistrationFailed, UnregistrationFailed } RegistrationState;

    /** The destructor.
     */
    virtual ~Account () { }


    /** Returns the name of the Ekiga::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The name of the Ekiga::Contact.
     */
    virtual const std::string get_name () const = 0;


    /** Returns the status of the Ekiga::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The status of the Ekiga::Account
     */
    virtual const std::string get_status () const = 0;


    /** Returns the registration state of the Ekiga::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The status of the Ekiga::Account
     */
    virtual RegistrationState get_state () const = 0;


    /** Returns a boolean indicating whether the account is enabled.
     *
     * @return Whether the account is enabled
     */
    virtual bool is_enabled () const = 0;


    /** Returns a boolean indicating whether the account is active.
     * An account can be enabled but inactive. It can happen if
     * the connection to the server failed (for example).
     * This method returns true if the account is in a sane state.
     *
     * @return Whether the account is active
     */
    virtual bool is_active () const = 0;
  };

  typedef boost::shared_ptr<Account> AccountPtr;

  /**
   * @}
   */

};
#endif
