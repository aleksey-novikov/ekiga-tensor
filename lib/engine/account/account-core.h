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
 *                         account-core.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : interface of the main account managing object
 *
 */

#ifndef __ACCOUNT_CORE_H__
#define __ACCOUNT_CORE_H__

#include <list>
#include <string>

#include "form-request.h"
#include "chain-of-responsibility.h"
#include "services.h"

#include "bank.h"

/* declaration of a few helper classes */
namespace Ekiga
{
  /**
   * @defgroup accounts
   * @{
   */

  /** Core object for address account support.
   *
   * Notice that you give banks to this object as references, so they won't
   * be freed here : it's up to you to free them somehow.
   */
  class AccountCore: public Service
  {
  public:

    /** The constructor.
     */
    AccountCore ();

    /** The destructor.
     */
    ~AccountCore ();


    /*** Service Implementation ***/

    /** Returns the name of the service.
     * @return The service name.
     */
    const std::string get_name () const
    { return "account-core"; }


    /** Returns the description of the service.
     * @return The service description.
     */
    const std::string get_description () const
    { return "\tAccount managing object"; }


    /*** Public API ***/

    /** Adds a bank to the AccountCore service.
     * @param The bank to be added.
     */
    void add_bank (BankPtr bank);


    /** Removes a bank from the AccountCore service.
     * @param The bank to be removed.
     */
    void remove_bank (BankPtr bank);


    /** Triggers a callback for all Ekiga::Bank banks of the
     * AccountCore service.
     * @param The callback (the return value means "go on" and allows
     *  stopping the visit)
     */
    void visit_banks (boost::function1<bool, BankPtr> visitor) const;


    /** This signal is emitted when a bank has been added to the core
     */
    boost::signals2::signal<void(BankPtr)> bank_added;

    /** This signal is emitted when a bank has been removed from the core
     */
    boost::signals2::signal<void(BankPtr)> bank_removed;


    /** This chain allows the AccountCore to present forms to the user
     */
    ChainOfResponsibility<FormRequestPtr> questions;

  private:
    std::list<BankPtr> banks;
  };
  /**
   * @}
   */
};

#endif
