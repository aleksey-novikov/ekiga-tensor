/* Ekiga -- A VoIP and Video-Conferencing application
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
 *                         opal-account.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *                          (c) 2013 by Julien Puydt
 *   description          : declaration of an OPAL account
 *
 */

#ifndef __OPAL_ACCOUNT_H__
#define __OPAL_ACCOUNT_H__

#include <libxml/tree.h>
#include <opal/pres_ent.h>
#include <sip/sippdu.h>

#include "account.h"
#include "notification-core.h"
#include "presence-core.h"
#include "personal-details.h"
#include "audiooutput-core.h"

#include "heap-impl.h"

#include "opal-presentity.h"

namespace Opal
{
  // forward declarations:
  class Bank;
  class CallManager;
  class Presentity;
  class EndPoint;
  namespace Sip { class EndPoint; };

  /**
   * @addtogroup accounts
   * @internal
   * @{
   */
  class Account:
    public Ekiga::Account,
    public Ekiga::PresencePublisher,
    public Ekiga::PresenceFetcher,
    public Ekiga::HeapImpl<Presentity>,
    public Ekiga::DynamicObject<Account>
  {
    friend class Presentity;
public:

    typedef enum { SIP } Type;

    static xmlNodePtr build_node (Account::Type typus,
                                  std::string name,
                                  std::string host,
                                  std::string outbound_proxy,
                                  std::string user,
                                  std::string auth_user,
                                  std::string password,
                                  bool enabled,
                                  unsigned timeout);

    /* The Opal::Account is implemented using the SIP EndPoints from OPAL.
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
    static boost::shared_ptr<Account> create (Bank & bank,
                                              boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                                              boost::shared_ptr<Ekiga::NotificationCore> _notification_core,
                                              boost::shared_ptr<Ekiga::PersonalDetails> _personal_details,
                                              boost::shared_ptr<Ekiga::AudioOutputCore> _audiooutput_core,
                                              EndPoint& _endpoint,
                                              Sip::EndPoint* _sip_endpoint,
                                              boost::function0<std::list<std::string> > _existing_groups,
                                              xmlNodePtr node_);

    ~Account ();

    const std::string get_name () const;

    const std::string get_status () const;

    Ekiga::Account::RegistrationState get_state () const;

    const std::string get_aor () const;

    Type get_type () const;

    std::list<std::string> get_groups () const;

    /** Returns the protocol name of the Opal::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The protocol name of the Ekiga::Contact.
     */
    const std::string get_protocol_name () const;

    const std::string get_host () const;

    const std::string get_outbound_proxy () const;

    /** Returns the user name for the Opal::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The user name of the Ekiga::Account.
     */
    const std::string get_username () const;

    /** Returns the authentication user name for the Opal::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The authentication user name of the Ekiga::Account.
     */
    const std::string get_authentication_username () const;

    /** Returns the password for the Opal::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The password of the Ekiga::Account.
     */
    const std::string get_password () const;

    void set_authentication_settings (const std::string & username,
                                      const std::string & password);

    /** Returns the registration timeout for the Opal::Account.
     * This function is purely virtual and should be implemented by the
     * Ekiga::Account descendant.
     * @return The timeout of the Ekiga::Account.
     */
    unsigned get_timeout () const;

    void enable ();

    void disable ();

    bool is_enabled () const;

    bool is_active () const;

    void remove ();

    void edit ();

    boost::signals2::signal<void(void)> trigger_saving;

    /*
     * This is because an opal account is an Ekiga::PresencePublisher
     */
    void publish (const Ekiga::PersonalDetails& details);

    /* This method is public to be called by an opal endpoint, which will push
     * this Opal::Account's new registration state
     * Notice : it's very wrong to make that a const method, but Opal seems to
     * want its Register method to take a const account...
     */
    void handle_registration_event (RegistrationState state_,
                                    const std::string info,
                                    const std::string & aor);

    /* This method is public to be called by an opal endpoint, which will push
     * this Opal::Account's message waiting information
     */
    void handle_message_waiting_information (const std::string info);

    const PString get_full_uri (const PString & uri) const;

protected:
    void on_rename_group (const std::list<std::string> & groups);

private:
    Account (Bank & bank,
             boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
             boost::shared_ptr<Ekiga::NotificationCore> _notification_core,
             boost::shared_ptr<Ekiga::PersonalDetails> _personal_details,
             boost::shared_ptr<Ekiga::AudioOutputCore> _audiooutput_core,
             EndPoint& _endpoint,
             Sip::EndPoint* _sip_endpoint,
             boost::function0<std::list<std::string> > _existing_groups,
             xmlNodePtr node_);
    boost::shared_ptr<Presentity> load_presentity (boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                                                   boost::function0<std::list<std::string> > _existing_groups,
                                                   xmlNodePtr _node);

    void fetch (const std::string uri);
    void unfetch (const std::string uri);
    bool is_supported_uri (const std::string & uri);

    void decide_type ();

    void add_contact ();

    bool on_add_contact_form_submitted (bool submitted,
                                        Ekiga::Form& result,
                                        std::string& error);

    bool on_edit_form_submitted (bool submitted,
                                 Ekiga::Form &result,
                                 std::string& error);
    void on_consult (const std::string url);

    bool on_rename_group_form_submitted (bool submitted,
                                         Ekiga::Form& result,
                                         std::string& error,
                                         const std::list<std::string> & groups);

    Type type;
    mutable RegistrationState state;
    bool dead;
    std::string status;  // the state, as a string
    int message_waiting_number;
    std::string protocol_name;

    bool failed_registration_already_notified;

    PSafePtr<OpalPresentity> opal_presentity;

    PDECLARE_PresenceChangeNotifier (Account, OnPresenceChange);

    boost::function0<std::list<std::string> > existing_groups;
    xmlNodePtr node;
    xmlNodePtr roster_node;
    void presence_status_in_main (std::string uri,
                                  std::string presence,
                                  std::string status) const;

    Bank & bank;

    boost::weak_ptr<Ekiga::PresenceCore> presence_core;
    boost::weak_ptr<Ekiga::NotificationCore> notification_core;
    boost::weak_ptr<Ekiga::PersonalDetails> personal_details;
    boost::weak_ptr<Ekiga::AudioOutputCore> audiooutput_core;

    EndPoint& endpoint;
    Sip::EndPoint* sip_endpoint;
    PString instance_id;
  };

  typedef boost::shared_ptr<Account> AccountPtr;

  /**
   * @}
   */
};

#endif
