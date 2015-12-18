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
 *                         opal-account.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *                          (c) 2013 by Julien Puydt
 *   description          : implementation of an opal account
 *
 */

#include "config.h"

#include <stdlib.h>

#include <glib/gi18n.h>

#include "form-request-simple.h"

#include "opal-bank.h"
#include "opal-presentity.h"


boost::shared_ptr<Opal::Bank>
Opal::Bank::create (Ekiga::ServiceCore& core,
                    Opal::EndPoint& _endpoint,
                    Opal::Sip::EndPoint* _sip_endpoint)
{
  boost::shared_ptr<Opal::Bank> bank =
    boost::shared_ptr<Opal::Bank> (new Opal::Bank (core,
                                                   _endpoint,
                                                   _sip_endpoint));

  bank->load ();

  return bank;
}


Opal::Bank::Bank (Ekiga::ServiceCore& core,
                  Opal::EndPoint& _endpoint,
                  Opal::Sip::EndPoint* _sip_endpoint):
  presence_core(core.get<Ekiga::PresenceCore> ("presence-core")),
  notification_core(core.get<Ekiga::NotificationCore> ("notification-core")),
  personal_details(core.get<Ekiga::PersonalDetails> ("personal-details")),
  audiooutput_core(core.get<Ekiga::AudioOutputCore> ("audiooutput-core")),
  endpoint (_endpoint),
  sip_endpoint(_sip_endpoint)
{
  // FIXME
  sip_endpoint->mwi_event.connect (boost::bind(&Opal::Bank::on_mwi_event, this, _1, _2));
}


Opal::Bank::~Bank ()
{
  delete protocols_settings;
}


boost::shared_ptr<Opal::Account>
Opal::Bank::load_account (boost::function0<std::list<std::string> > _existing_groups,
                          xmlNodePtr _node)
{
  boost::shared_ptr<Opal::Account> account =
    Opal::Account::create (*this,
                           presence_core,
                           notification_core,
                           personal_details,
                           audiooutput_core,
                           endpoint,
                           sip_endpoint,
                           _existing_groups,
                           _node);

  accounts.add_connection (account, account->trigger_saving.connect (boost::bind (&Opal::Bank::save, this)));
  accounts.add_connection (account, account->removed.connect (boost::bind (&Opal::Bank::on_account_removed, this, _1), boost::signals2::at_front));  // slot from DynamicObjectStore must be the last called

  add_account (account);
  add_heap (account);

  activate (account);

  return account;
}


void
Opal::Bank::load ()
{
  protocols_settings = new Ekiga::Settings (PROTOCOLS_SCHEMA);

  const std::string raw = protocols_settings->get_string ("accounts");

  /* Actor stuff */
  add_actions ();

  /* Populate Accounts */
  doc = boost::shared_ptr<xmlDoc> (xmlRecoverMemory (raw.c_str (), raw.length ()), xmlFreeDoc);
  if (!doc)
    doc = boost::shared_ptr<xmlDoc> (xmlNewDoc (BAD_CAST "1.0"), xmlFreeDoc);

  node = xmlDocGetRootElement (doc.get ());
  if (node == NULL) {

    node = xmlNewDocNode (doc.get (), NULL, BAD_CAST "accounts", NULL);
    xmlDocSetRootElement (doc.get (), node);
  }

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL
        && xmlStrEqual(BAD_CAST "account", child->name))
      load_account (boost::bind(&Opal::Bank::existing_groups, this), child);
  }
}


void
Opal::Bank::new_account (Account::Type acc_type,
                         std::string username,
                         std::string password)
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request = boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Bank::on_new_account_form_submitted, this, _1, _2, _3, acc_type)));

  request->title (_("Add Account"));

  switch (acc_type) {

  case Opal::Account::SIP:
  default:
    request->text ("name", _("_Name"), std::string (), _("My SIP Account"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    request->text ("host", _("_Registrar"), std::string (), _("ekiga.net"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    request->text ("user", _("_User"), username, _("jon"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    request->text ("authentication_user", _("_Login"), std::string (), _("jon.doe"),
                   Ekiga::FormVisitor::STANDARD, true, true);
    request->text ("password", _("_Password"), password, _("1234"),
                   Ekiga::FormVisitor::PASSWORD, false, false);
    request->text ("outbound_proxy", _("Outbound _Proxy"), "", _("proxy.company.com"),
                   Ekiga::FormVisitor::STANDARD, true, true);
    request->text ("timeout", _("_Timeout"), "3600", "3600",
                   Ekiga::FormVisitor::NUMBER, true, false);
    break;
  }
  request->boolean ("enabled", _("_Enable account"), true);

  questions (request);
}


bool
Opal::Bank::on_new_account_form_submitted (bool submitted,
                                           Ekiga::Form& result,
                                           G_GNUC_UNUSED std::string& error,
                                           Account::Type acc_type)
{
  if (!submitted)
    return false;

  boost::shared_ptr<Ekiga::FormRequestSimple> request = boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Bank::on_new_account_form_submitted, this, _1, _2, _3, acc_type)));

  std::string new_name = result.text ("name");
  std::string new_host = result.text ("host");
  std::string new_user = result.text ("user");
  std::string new_authentication_user = (acc_type == Opal::Account::SIP && ! result.text ("authentication_user").empty ()) ? result.text ("authentication_user") : new_user;
  std::string new_password = result.text ("password");
  std::string new_outbound_proxy = result.text ("outbound_proxy");
  bool new_enabled = result.boolean ("enabled");
  unsigned new_timeout = atoi (result.text ("timeout").c_str ());

  // This should only happen with Ekiga.net accounts
  if (!new_user.compare (0, 4, "sip:")) {
    new_user = new_user.substr (4, string::npos);  // remove leading sip:
  }
  std::size_t pos = new_user.find_first_of ("@");
  if (pos != std::string::npos) {
    new_user = new_user.substr (0, pos);  // remove trailing @ekiga.net
  }
  if (!new_authentication_user.compare (0, 4, "sip:")) {
    new_authentication_user = new_authentication_user.substr (4, string::npos);  // remove leading sip:
  }
  pos = new_authentication_user.find_first_of ("@");
  if (pos != std::string::npos) {
    new_authentication_user = new_authentication_user.substr (0, pos);  // remove trailing @ekiga.net
  }

  result.visit (*request);

  add (acc_type, new_name, new_host, new_outbound_proxy, new_user, new_authentication_user,
       new_password, new_enabled, new_timeout);

  return true;
}


void
Opal::Bank::add (Account::Type acc_type,
                 std::string name,
                 std::string host,
                 std::string outbound_proxy,
                 std::string user,
                 std::string auth_user,
                 std::string password,
                 bool enabled,
                 unsigned timeout)
{
  xmlNodePtr child = Opal::Account::build_node (acc_type, name, host, outbound_proxy, user, auth_user, password, enabled, timeout);
  xmlAddChild (node, child);

  save ();

  load_account (boost::bind(&Opal::Bank::existing_groups, this), child);
}


Opal::AccountPtr
Opal::Bank::find_account (const std::string& _aor)
{
  AccountPtr result;
  std::string aor = _aor;
  std::string::size_type t = aor.find_first_of (";");

  if (t != std::string::npos)
    aor = aor.substr (0, t);

  for (Ekiga::BankImpl<Opal::Account>::iterator iter = Ekiga::BankImpl<Opal::Account>::begin ();
       iter != Ekiga::BankImpl<Opal::Account>::end ();
       ++iter) {
    if (aor.find ("@") != std::string::npos && (*iter)->get_aor () == aor)  // find by account name+host (aor)
        return *iter;
    else if ((*iter)->get_host () == aor)  // find by host
      return *iter;
  }
  return result;
}


void
Opal::Bank::save () const
{
  xmlChar *buffer = NULL;
  int doc_size = 0;

  xmlDocDumpMemory (doc.get (), &buffer, &doc_size);

  protocols_settings->set_string ("accounts", (const char*)buffer);

  xmlFree (buffer);
}

void
Opal::Bank::on_account_removed (boost::shared_ptr<Account> account)
{
  boost::shared_ptr<Ekiga::PresenceCore> pcore = presence_core.lock ();
  if (pcore)
    pcore->remove_presence_fetcher (account);
}


void
Opal::Bank::publish (const Ekiga::PersonalDetails& details)
{
  for (Ekiga::BankImpl<Opal::Account>::iterator iter = Ekiga::BankImpl<Opal::Account>::begin ();
       iter != Ekiga::BankImpl<Opal::Account>::end ();
       iter++)
    (*iter)->publish (details);
}


void
Opal::Bank::on_mwi_event (std::string aor,
                          std::string info)
{
  AccountPtr account = find_account (aor);

  if (account)
    account->handle_message_waiting_information (info);
}


const std::list<std::string>
Opal::Bank::existing_groups () const
{
  std::list<std::string> result;

  for (Ekiga::BankImpl<Opal::Account>::const_iterator iter = Ekiga::BankImpl<Opal::Account>::begin ();
       iter != Ekiga::BankImpl<Opal::Account>::end ();
       ++iter) {

    std::list<std::string> groups = (*iter)->get_groups ();
    result.merge (groups);
  }

  result.push_back (_("Family"));
  result.push_back (_("Friend"));
  /* Translator: http://www.ietf.org/rfc/rfc4480.txt proposes several
     relationships between you and your contact; associate means
     someone who is at the same "level" than you.
  */
  result.push_back (_("Associate"));
  /* Translator: http://www.ietf.org/rfc/rfc4480.txt proposes several
     relationships between you and your contact; assistant means
     someone who is at a lower "level" than you.
  */
  result.push_back (_("Assistant"));
  /* Translator: http://www.ietf.org/rfc/rfc4480.txt proposes several
     relationships between you and your contact; supervisor means
     someone who is at a higher "level" than you.
  */
  result.push_back (_("Supervisor"));
  /* Translator: http://www.ietf.org/rfc/rfc4480.txt proposes several
     relationships between you and your contact; self means yourself.
  */
  result.push_back (_("Self"));

  result.sort ();
  result.unique ();

  return result;
}

struct find_presentity_helper
{
  find_presentity_helper (const std::string uri_): uri(uri_)
  {}

  bool operator() (Ekiga::PresentityPtr pres)
  {
    Opal::PresentityPtr presentity = boost::dynamic_pointer_cast<Opal::Presentity> (pres);

    if (presentity && presentity->get_uri () == uri)
      result = presentity;

    return !result;
  }

  std::string uri;
  Ekiga::PresentityPtr result;
};

Ekiga::PresentityPtr
Opal::Bank::find_presentity_for_uri (const std::string uri) const
{
  find_presentity_helper helper(uri);

  for (Ekiga::ClusterImpl<Opal::Account>::const_iterator iter = Ekiga::ClusterImpl<Opal::Account>::begin ();
       iter != Ekiga::ClusterImpl<Opal::Account>::end () && !helper.result;
       ++iter)
    (*iter)->visit_presentities(boost::ref(helper));

  return helper.result;
}

void
Opal::Bank::migrate_from_gconf (const std::list<std::string> old)
{
  xmlDoc* doc = xmlNewDoc (BAD_CAST "1.0");
  xmlNodePtr node = xmlNewDocNode (doc, NULL, BAD_CAST "accounts", NULL);
  xmlDocSetRootElement (doc, node);

  for (std::list<std::string>::const_iterator iter = old.begin ();
       iter != old.end ();
       ++iter) {

    std::string protocol_name;
    Opal::Account::Type acc_type;
    std::string name;
    std::string host;
    std::string user;
    std::string auth_user;
    std::string password;
    bool enabled = false;
    unsigned timeout = 0;
    int ii = 0;
    char* pch = strtok ((char*)iter->c_str (), "|");
    while (pch != NULL) {

      switch (ii) {

      case 0:
        enabled = atoi (pch);
        break;

      case 3:
        name = pch;
        break;

      case 4:
        protocol_name = pch;
        break;

      case 5:
        host = pch;
        break;

      case 7:
        user = pch;
        break;

      case 8:
        auth_user = pch;
        break;

      case 9:
        password = pch;
        if (password == " ")
          password = "";
        break;

      case 10:
        timeout = atoi (pch);
        break;

      case 1:
      case 2:
      case 6:
      case 11:
      default:
        break;
      }
      pch = strtok (NULL, "|");
      ii++;
    }

    acc_type = Account::SIP;

    xmlNodePtr child = Opal::Account::build_node (acc_type, name, host, "", user, auth_user, password, enabled, timeout);

    xmlAddChild (node, child);
  }

  xmlChar* buffer = NULL;
  int doc_size = 0;
  Ekiga::Settings* settings = new Ekiga::Settings (PROTOCOLS_SCHEMA);

  xmlDocDumpMemory (doc, &buffer, &doc_size);
  settings->set_string ("accounts", (const char*)buffer);

  delete settings;
  xmlFreeDoc (doc);
}


void
Opal::Bank::add_actions ()
{
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("add-account-sip", _("_Add a SIP Account"),
                                                   boost::bind (&Opal::Bank::new_account, this,
                                                                Opal::Account::SIP, "", ""))));
}


void
Opal::Bank::activate (boost::shared_ptr<Opal::Account> account)
{
  if (!endpoint.IsReady ()) {
    endpoint.ready.connect (boost::bind (&Opal::Bank::activate, this, account));
    return;
  }

  boost::shared_ptr<Ekiga::PresenceCore> pcore = presence_core.lock ();
  if (pcore)
    pcore->add_presence_fetcher (account);
  if (account->is_enabled ())
    account->enable ();
}
