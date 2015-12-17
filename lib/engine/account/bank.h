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
 *                         bank.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of a partial implementation
 *                          of a Bank
 *
 */

#ifndef __BANK_H__
#define __BANK_H__

#include "account.h"
#include "actor.h"

namespace Ekiga
{
  class AccountCore;

  /**
   * @addtogroup accounts
   * @{
   */

  class Bank;
  typedef boost::shared_ptr<Bank> BankPtr;

  class Bank: public virtual Actor
  {
  public:


    virtual ~Bank () { }


    /** Visit all accounts of the bank and trigger the given callback.
     * @param The callback (the return value means "go on" and allows
     *  stopping the visit)
     */
    virtual void visit_accounts (boost::function1<bool, AccountPtr> visitor) const = 0;


    /** This signal is emitted when a account has been added.
     */
    boost::signals2::signal<void(AccountPtr)> account_added;

    /** This signal is emitted when a account has been removed.
     */
    boost::signals2::signal<void(AccountPtr)> account_removed;

    /** This signal is emitted when a account has been updated.
     */
    boost::signals2::signal<void(AccountPtr)> account_updated;
  };

  /**
   * @}
   */

};
#endif
