/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2013 Damien Sandras <dsandras@seconix.com>
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
 *                         opal-bank.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *                          (c) 2013 by Julien Puydt
 *   description          : declaration of an OPAL bank
 *
 */

#ifndef __OPAL_BANK_H__
#define __OPAL_BANK_H__

#include "config.h"

#include "contact-core.h"
#include "presence-core.h"

#include "ekiga-settings.h"

#include "sip-endpoint.h"

#include "cluster-impl.h"
#include "bank-impl.h"

namespace Opal
{
  /**
   * @addtogroup accounts
   * @internal
   * @{
   */
  class Bank:
      public Ekiga::BankImpl<Account>,
      public Ekiga::ClusterImpl<Account>,
      public Ekiga::PresencePublisher,
      public Ekiga::DynamicObject<Bank>,
      public Ekiga::Service
  {
    friend class Account;
public:

    /* The Opal::Bank is implemented using the SIP EndPoints from OPAL.
     *
     * We pass them as arguments instead of using the Opal::CallManager
     * to prevent useless references to the CallManager. We are using
     * shared_ptr's, they control the destruction order of objects, which
     * is not what Opal was designed for.
     *
     * We can safely rely on the SIP EndPoints as long as:
     *   1) The Opal::Bank and its Opal::Accounts are destroyed
     *      before the Opal EndPoints.
     *   2) They are destroyed before the Opal::CallManager.
     *
     * However, the SIP EndPoints have a null_deleter. That means
     * that Opal is taking care of deleting them. They are not deleted when
     * the last object having a reference to them is deleted.
     */
    static boost::shared_ptr<Bank> create (Ekiga::ServiceCore& _core,
                                           Opal::EndPoint& _endpoint,
                                           Opal::Sip::EndPoint* _sip_endpoint);

    ~Bank ();

    const std::string get_name () const
    { return "opal-account-store"; }

    const std::string get_description () const
    { return "\tStores the opal accounts"; }

    /*
     * this object is an Ekiga::PresencePublisher
     */
    void publish (const Ekiga::PersonalDetails& details);

    void new_account (Account::Type acc_type,
                      std::string username = "",
                      std::string password = "");


    /** Find the account with the given address of record in the Bank
     * @param aor is the address of record of the Account or the host to look
     *        for
     * @return The Opal::Account if an Account was found, false otherwise.
     *         The returned account should not be freed.
     */
    AccountPtr find_account (const std::string& aor);

    const std::list<std::string> existing_groups () const;

    /* this is useful when we want to do something with some uri and
       would like to avoid creating a brand-new presentity on it */
    Ekiga::PresentityPtr find_presentity_for_uri (const std::string uri) const;

    // FIXME: only here for the transition off gconf
    static void migrate_from_gconf (const std::list<std::string> old);

private:
    Bank (Ekiga::ServiceCore& _core,
          Opal::EndPoint& _endpoint,
          Opal::Sip::EndPoint* _sip_endpoint);

    boost::shared_ptr<Account> load_account (boost::function0<std::list<std::string> > _existing_groups,
                                             xmlNodePtr _node);

    void load ();
    void set_ready ();
    bool is_ready;

    boost::weak_ptr<Ekiga::PresenceCore> presence_core;
    boost::shared_ptr<Ekiga::NotificationCore> notification_core;
    boost::shared_ptr<Ekiga::PersonalDetails> personal_details;
    boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core;

    boost::shared_ptr<xmlDoc> doc;
    xmlNodePtr node;

    bool on_new_account_form_submitted (bool submitted,
                                        Ekiga::Form& form,
                                        std::string& error,
                                        Account::Type acc_type);

    void add (Account::Type acc_type,
              std::string name,
              std::string host,
              std::string outbound_proxy,
              std::string user,
              std::string auth_user,
              std::string password,
              bool enabled,
              unsigned timeout);

    void save () const;

    void on_account_removed (boost::shared_ptr<Account> account);

    void on_mwi_event (std::string aor,
                       std::string info);

    void update_sip_endpoint_aor_map ();

    void add_actions ();

    void activate (boost::shared_ptr<Account> account);

    Ekiga::Settings *protocols_settings;

    Opal::EndPoint& endpoint;
    Opal::Sip::EndPoint* sip_endpoint;
  };

  /**
   * @}
   */
};

#endif
