/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
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
 *   description          : implementation of an opal account
 *
 */

#include <string.h>
#include <stdlib.h>
#include <sstream>

#include <glib.h>
#include <glib/gi18n.h>

#include <ptlib.h>
#include <ptclib/guid.h>

#include <opal/opal.h>
#include <sip/sippres.h>

#include "config.h"

#include "opal-account.h"

#include "robust-xml.h"
#include "form-request-simple.h"
#include "platform.h"

#include "opal-presentity.h"
#include "sip-endpoint.h"

// remove leading and trailing spaces and tabs (useful for copy/paste)
// also, if no protocol specified, add leading "sip:"
static std::string
canonize_uri (std::string uri)
{
  const size_t begin_str = uri.find_first_not_of (" \t");
  if (begin_str == std::string::npos)  // there is no content
    return "";

  const size_t end_str = uri.find_last_not_of (" \t");
  const size_t range = end_str - begin_str + 1;
  uri = uri.substr (begin_str, range);
  const size_t pos = uri.find (":");
  if (pos == std::string::npos)
    uri = uri.insert (0, "sip:");
  return uri;
}


xmlNodePtr
Opal::Account::build_node(Opal::Account::Type typus,
                          std::string name,
                          std::string host,
                          std::string outbound_proxy,
                          std::string user,
                          std::string auth_user,
                          std::string password,
                          bool enabled,
                          unsigned timeout)
{
  xmlNodePtr node = xmlNewNode (NULL, BAD_CAST "account");

  xmlNewChild (node, NULL, BAD_CAST "name",
               BAD_CAST robust_xmlEscape (node->doc, name).c_str ());
  xmlNewChild (node, NULL, BAD_CAST "host",
               BAD_CAST robust_xmlEscape (node->doc, host).c_str ());
  xmlNewChild (node, NULL, BAD_CAST "user",
               BAD_CAST robust_xmlEscape (node->doc, user).c_str ());
  xmlNewChild (node, NULL, BAD_CAST "auth_user",
               BAD_CAST robust_xmlEscape (node->doc, auth_user).c_str ());
  xmlNewChild (node, NULL, BAD_CAST "password",
               BAD_CAST robust_xmlEscape (node->doc, password).c_str ());
  if (enabled)
    xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "true");
  else
    xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "false");
  {
    std::stringstream sstream;
    sstream << timeout;
    xmlSetProp (node, BAD_CAST "timeout", BAD_CAST sstream.str ().c_str ());
  }

  // FIXME: One improvement could be to use inheritance here and have
  //        specific objects for specific accounts types.
  //        Would it be considered cleaner or overkill?
  switch (typus) {

  case SIP:
  default:
    xmlSetProp (node, BAD_CAST "type", BAD_CAST "SIP");
    break;
  }
  xmlNewChild (node, NULL, BAD_CAST "outbound_proxy",
               BAD_CAST robust_xmlEscape (node->doc, outbound_proxy).c_str ());

  xmlNewChild(node, NULL, BAD_CAST "roster", NULL);

  return node;
}


boost::shared_ptr<Opal::Account>
Opal::Account::create (Bank & bank,
                       boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                       boost::shared_ptr<Ekiga::NotificationCore> _notification_core,
                       boost::shared_ptr<Ekiga::PersonalDetails> _personal_details,
                       boost::shared_ptr<Ekiga::AudioOutputCore> _audiooutput_core,
                       EndPoint& _endpoint,
                       Sip::EndPoint* _sip_endpoint,
                       boost::function0<std::list<std::string> > _existing_groups,
                       xmlNodePtr _node)
{
  boost::shared_ptr<Opal::Account> account =
    boost::shared_ptr<Opal::Account> (new Opal::Account (bank,
                                                         _presence_core,
                                                         _notification_core,
                                                         _personal_details,
                                                         _audiooutput_core,
                                                         _endpoint,
                                                         _sip_endpoint,
                                                         _existing_groups,
                                                         _node));

  return account;
}


Opal::Account::Account (Opal::Bank & _bank,
                        boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                        boost::shared_ptr<Ekiga::NotificationCore> _notification_core,
                        boost::shared_ptr<Ekiga::PersonalDetails> _personal_details,
                        boost::shared_ptr<Ekiga::AudioOutputCore> _audiooutput_core,
                        Opal::EndPoint& _endpoint,
                        Opal::Sip::EndPoint* _sip_endpoint,
                        boost::function0<std::list<std::string> > _existing_groups,
                        xmlNodePtr _node):
  existing_groups(_existing_groups),
  node(_node),
  bank(_bank),
  presence_core(_presence_core),
  notification_core(_notification_core),
  personal_details(_personal_details),
  audiooutput_core(_audiooutput_core),
  endpoint(_endpoint),
  sip_endpoint(_sip_endpoint)
{
  state = Unregistered;
  status = _("Unregistered");
  message_waiting_number = 0;
  failed_registration_already_notified = false;
  dead = false;

  decide_type ();

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "roster", child->name)) {

      roster_node = child;
      for (xmlNodePtr presnode = roster_node->children; presnode != NULL; presnode = presnode->next)
        load_presentity (presence_core, existing_groups, presnode);
    }
  }

  /* Actor stuff */

  /* Translators: Example: Add ekiga.net Contact */
  char *text = g_strdup_printf (_("A_dd %s Contact"), get_host ().c_str ());
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("add-contact", text,
                                                   boost::bind (&Opal::Account::add_contact, this))));
  g_free (text);
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("edit-account", _("_Edit"),
                                                   boost::bind (&Opal::Account::edit, this))));
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("remove-account", _("_Remove"),
                                                   boost::bind (&Opal::Account::remove, this))));
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("enable-account", _("_Enable"),
                                                   boost::bind (&Opal::Account::enable, this), !is_enabled ())));
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("disable-account", _("_Disable"),
                                                   boost::bind (&Opal::Account::disable, this), is_enabled ())));

  if (sip_endpoint)
    instance_id = sip_endpoint->GetInstanceID ().AsString ();
}


Opal::Account::~Account ()
{
}


std::list<std::string>
Opal::Account::get_groups () const
{
  std::list<std::string> result;

  for (Ekiga::HeapImpl<Opal::Presentity>::const_iterator iter = Ekiga::HeapImpl<Opal::Presentity>::begin ();
       iter != Ekiga::HeapImpl<Opal::Presentity>::end ();
       ++iter) {

    std::list<std::string> groups = (*iter)->get_groups ();
    result.merge (groups);
    result.sort ();
    result.unique ();
  }

  return result;
}


const std::string
Opal::Account::get_name () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "name", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      } else {
        result = _("Unnamed");
      }
    }
  }

  return result;
}

const std::string
Opal::Account::get_status () const
{
  std::string result;
  if (message_waiting_number > 0) {

    gchar* str = NULL;
    /* translators : the result will look like :
     * "registered (with 2 voice mail messages)"
     */
    str = g_strdup_printf (ngettext ("%s (with %d voice mail message)",
                                     "%s (with %d voice mail messages)",
                                     message_waiting_number),
                           status.c_str (), message_waiting_number);
    result = str;
    g_free (str);
  } else {

    result = status;
  }

  return result;
}

Ekiga::Account::RegistrationState
Opal::Account::get_state () const
{
  return state;
}

const std::string
Opal::Account::get_aor () const
{
  std::stringstream str;

  str << "sip:" << get_username ();

  if (get_username ().find ("@") == string::npos)
    str << "@" << get_host ();

  return str.str ();
}

const std::string
Opal::Account::get_protocol_name () const
{
  std::string result = "SIP";
  xmlChar* xml_str = xmlGetProp (node, BAD_CAST "type");

  if (xml_str != NULL) {

    result = (const char*)xml_str;
    xmlFree (xml_str);
    if (result.compare ("Ekiga") == 0 || result.compare ("DiamondCard") == 0)
      result = "SIP";
  }

  return result;
}


const std::string
Opal::Account::get_host () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "host", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      }
    }
  }

  return result;
}


const std::string
Opal::Account::get_outbound_proxy () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "outbound_proxy", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      }
    }
  }

  return result;
}


const std::string
Opal::Account::get_username () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "user", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      }
    }
  }

  return result;
}


const std::string
Opal::Account::get_authentication_username () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "auth_user", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      }
    }
  }

  return result;
}


const std::string
Opal::Account::get_password () const
{
  std::string result;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL && xmlStrEqual (BAD_CAST "password", child->name)) {

      xml_str = xmlNodeGetContent (child);
      if (xml_str != NULL) {

        result = (const char*)xml_str;
        xmlFree (xml_str);
      }
    }
  }

  return result;
}


unsigned
Opal::Account::get_timeout () const
{
  unsigned result = 0;
  xmlChar* xml_str = xmlGetProp (node, BAD_CAST "timeout");

  if (xml_str != NULL) {

    result = std::strtoul ((const char*)xml_str, NULL, 0);
    xmlFree (xml_str);
  }

  return result;
}


void
Opal::Account::set_authentication_settings (const std::string& username,
                                            const std::string& password)
{
  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

    if (child->type == XML_ELEMENT_NODE && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST "user", child->name))
        robust_xmlNodeSetContent (node, &child, "user", username);
      if (xmlStrEqual (BAD_CAST "auth_user", child->name))
        robust_xmlNodeSetContent (node, &child, "auth_user", username);
      if (xmlStrEqual (BAD_CAST "password", child->name))
        robust_xmlNodeSetContent (node, &child, "password", password);
    }
  }

  enable ();
}


void
Opal::Account::enable ()
{
  PString _aor;
  if (!is_enabled ()) {
    xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "true");
    trigger_saving ();
  }

  state = Processing;
  status = _("Processing...");

  /* Actual registration code */
  switch (type) {
  case Account::SIP:
  default:
    // Register the given aor to the given registrar
    if (sip_endpoint)
      sip_endpoint->EnableAccount (*this);
    break;
  }

  updated (this->shared_from_this ());

  disable_action ("enable-account");
  enable_action ("disable-account");
}


void
Opal::Account::disable ()
{
  if (is_enabled ()) {
    xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "false");
    trigger_saving ();
  }

  /* Actual unregistration code */
  switch (type) {
  case Account::SIP:
  default:
    if (opal_presentity) {

      for (Ekiga::HeapImpl<Opal::Presentity>::iterator iter = Ekiga::HeapImpl<Opal::Presentity>::begin ();
           iter != Ekiga::HeapImpl<Opal::Presentity>::end ();
           ++iter) {
        (*iter)->set_presence ("unknown");
        (*iter)->set_status ("");
      }

      if (sip_endpoint)
        sip_endpoint->Unsubscribe (SIPSubscribe::MessageSummary, get_full_uri (get_aor ()));

      opal_presentity->Close ();
    }
    if (sip_endpoint) {
      // Register the given aor to the given registrar
      sip_endpoint->DisableAccount (*this);
    }
    break;
  }

  // Translators: this is a state, not an action, i.e. it should be read as
  // "(you are) unregistered", and not as "(you have been) unregistered"
  status = _("Unregistered");
  state = Unregistered;

  updated (this->shared_from_this ());

  enable_action ("enable-account");
  disable_action ("disable-account");
}


bool
Opal::Account::is_enabled () const
{
  bool result = false;
  xmlChar* xml_str = xmlGetProp (node, BAD_CAST "enabled");

  if (xml_str != NULL) {

    if (xmlStrEqual (xml_str, BAD_CAST "true"))
      result = true;
    else
      result = false;
    xmlFree (xml_str);
  }

  return result;
}


bool
Opal::Account::is_active () const
{
  if (!is_enabled ())
    return false;

  return (state == Registered);
}


void
Opal::Account::remove ()
{
  dead = true;
  if (state == Registered || state == Processing) {
    disable();
    return;
  }

  xmlUnlinkNode (node);
  xmlFreeNode (node);

  trigger_saving ();
  removed (this->shared_from_this ());
}


void
Opal::Account::edit ()
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request = boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Account::on_edit_form_submitted, this, _1, _2, _3)));
  std::stringstream str;

  str << get_timeout ();

  /* Translators: This is Edit name of the Account
   * e.g. Editing Ekiga.net Account.
   */
  char *title = g_strdup_printf (_("Editing %s Account"), get_name ().c_str ());
  request->title (title);
  g_free (title);

  std::string uri = "sip:" + get_username () + "@" + get_host ();

  switch (type) {
  case Opal::Account::SIP:
  default:
    request->text ("name", _("_Name"), get_name (), _("Ekiga.Net Account"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    request->text ("host", _("_Registrar"), get_host (), _("ekiga.net"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    request->text ("user", _("_User"), get_username (), _("jon"),
                   Ekiga::FormVisitor::STANDARD, false, false);
    /* Translators:
     * SIP knows two usernames: The name for the client ("User") and the name
     * for the authentication procedure ("Authentication user"), aka Login
     * to make it understandable
     */
    request->text ("authentication_user", _("_Login"), get_authentication_username () != get_username () ? get_authentication_username () : "", _("jon.doe"),
                   Ekiga::FormVisitor::STANDARD, true, true);
    request->text ("password", _("_Password"), get_password (), _("1234"),
                   Ekiga::FormVisitor::PASSWORD, false, false);
    request->text ("outbound_proxy", _("Outbound _Proxy"), get_outbound_proxy (), _("proxy.company.com"),
                   Ekiga::FormVisitor::STANDARD, true, true);
    request->text ("timeout", _("_Timeout"), "3600", "3600",
                   Ekiga::FormVisitor::NUMBER, true, false);
  }
  request->boolean ("enabled", _("_Enable Account"), is_enabled ());

  Ekiga::Account::questions (request);
}


bool
Opal::Account::on_edit_form_submitted (bool submitted,
                                       Ekiga::Form &result,
                                       std::string &error)
{
  if (!submitted)
    return false;

  std::string new_name = result.text ("name");
  std::string new_host = result.text ("host");

  std::string new_outbound_proxy = result.text ("outbound_proxy");
  std::string new_user = result.text ("user");
  // This should only happen with Ekiga.net accounts
  if (!new_user.compare (0, 4, "sip:"))
    new_user = new_user.substr (4, string::npos);  // remove leading sip:
  std::size_t pos = new_user.find_first_of ("@");
  if (pos != std::string::npos) {
    new_user = new_user.substr (0, pos);  // remove trailing @ekiga.net
    new_host = new_user.substr (pos);
  }
  std::string new_authentication_user = result.text ("authentication_user");
  if (new_authentication_user.empty ())
    new_authentication_user = new_user;
  std::string new_password = result.text ("password");
  bool new_enabled = result.boolean ("enabled");
  bool should_enable = false;
  bool should_disable = false;
  unsigned new_timeout = atoi (result.text ("timeout").c_str ());

  if (new_name.empty ())
    error = _("You did not supply a name for that account.");
  else if (new_host.empty ())
    error = _("You did not supply a host to register to.");
  else if (new_user.empty ())
    error = _("You did not supply a user name for that account.");
  else if (new_timeout < 10)
    error = _("The timeout should be at least 10 seconds.");

  if (!error.empty ()) {

    return false;
  }
  else {

    // Account was enabled and is now disabled
    // Disable it
    if (is_enabled () != new_enabled && !new_enabled) {
      should_disable = true;
    }
    // Account was disabled and is now enabled
    // or account was already enabled
    else if (new_enabled) {
      // Some critical setting just changed
      if (get_host () != new_host
          || get_outbound_proxy () != new_outbound_proxy
          || get_username () != new_user
          || get_authentication_username () != new_authentication_user
          || get_password () != new_password
          || get_timeout () != new_timeout
          || is_enabled () != new_enabled) {

        should_enable = true;
      }
    }

    if (new_enabled)
      xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "true");
    else
      xmlSetProp (node, BAD_CAST "enabled", BAD_CAST "false");

    {
      std::stringstream sstream;
      sstream << new_timeout;
      xmlSetProp (node, BAD_CAST "timeout", BAD_CAST sstream.str ().c_str ());
    }

    for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

      if (child->type == XML_ELEMENT_NODE && child->name != NULL) {

        if (xmlStrEqual (BAD_CAST "name", child->name))
          robust_xmlNodeSetContent (node, &child, "name", new_name);
        if (xmlStrEqual (BAD_CAST "host", child->name))
          robust_xmlNodeSetContent (node, &child, "host", new_host);
        if (xmlStrEqual (BAD_CAST "outbound_proxy", child->name))
          robust_xmlNodeSetContent (node, &child, "outbound_proxy", new_outbound_proxy);
        if (xmlStrEqual (BAD_CAST "user", child->name))
          robust_xmlNodeSetContent (node, &child, "user", new_user);
        if (xmlStrEqual (BAD_CAST "auth_user", child->name))
          robust_xmlNodeSetContent (node, &child, "auth_user", new_authentication_user);
        if (xmlStrEqual (BAD_CAST "password", child->name))
          robust_xmlNodeSetContent (node, &child, "password", new_password);
      }
    }

    decide_type ();

    if (should_enable)
      enable ();
    else if (should_disable)
      disable ();

    updated (this->shared_from_this ());
    trigger_saving ();
  }

  return true;
}

void
Opal::Account::add_contact ()
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request =
    boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Account::on_add_contact_form_submitted, this, _1, _2, _3)));
  std::list<std::string> groups = existing_groups ();

  request->title (_("Add Contact"));
  request->text ("name", _("_Name"), std::string (), _("John Doe"),
                 Ekiga::FormVisitor::STANDARD, false, false);

  request->text ("uri", _("_URI"), "sip:", "sip:john.doe@ekiga.net",
                 Ekiga::FormVisitor::URI, false, false);

  request->editable_list ("groups",
                          _("Groups"),
                          std::list<std::string>(), groups);

  Ekiga::Heap::questions (request);
}

bool
Opal::Account::on_add_contact_form_submitted (bool submitted,
                                              Ekiga::Form& result,
                                              std::string& error)
{
  if (!submitted)
    return false;

  const std::string name = result.text ("name");
  std::string uri;
  const std::list<std::string> groups = result.editable_list ("groups");

  uri = result.text ("uri");
  uri = canonize_uri (uri);

  if (!is_supported_uri (uri)) {
    error = _("You supplied an unsupported address");
    return false;
  }

  if (is_supported_uri (uri)) {
    xmlNodePtr presnode = Opal::Presentity::build_node (name, uri, groups);
    xmlAddChild (roster_node, presnode);
    trigger_saving ();

    boost::shared_ptr<Opal::Presentity> pres = load_presentity (presence_core, existing_groups, presnode);
    fetch (pres->get_uri ());

    return true;
  }
  else {
    if (is_supported_uri (uri))
      error = _("You supplied an unsupported address");
    else
      error = _("You already have a contact with this address!");
  }

  return false;
}

void
Opal::Account::on_consult (const std::string url)
{
  gm_platform_open_uri (url.c_str ());
}

void
Opal::Account::publish (const Ekiga::PersonalDetails& details)
{
  if (opal_presentity) {
    OpalPresenceInfo opi = OpalPresenceInfo (OpalPresenceInfo::Available);
    opi.m_activities = PString (details.get_presence ());
    opi.m_note = details.get_status ();
    opi.m_service = instance_id;

    opal_presentity->SetLocalPresence (opi);
    PTRACE (4, "Ekiga\tSent its own presence (publish) for " << get_aor() << ": " << details.get_presence () << ", note " << details.get_status ());
  }
}


boost::shared_ptr<Opal::Presentity>
Opal::Account::load_presentity (boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                                boost::function0<std::list<std::string> > _existing_groups,
                                xmlNodePtr _node)
{
  Opal::PresentityPtr pres = Opal::Presentity::create (*this, _presence_core, _existing_groups, _node);

  // When the presentity emits trigger_saving, we relay it "upstream" so that the
  // Bank can save everything.
  presentities.add_connection (pres, pres->trigger_saving.connect (boost::ref (trigger_saving)));
  presentities.add_connection (pres, pres->removed.connect (boost::bind (&Opal::Account::unfetch, this, pres->get_uri ()), boost::signals2::at_front));  // slot from DynamicObjectStore must be the last called
  add_presentity (pres);

  return pres;
}


void
Opal::Account::fetch (const std::string uri)
{
  // Check if this is a presentity we watch
  if (!is_supported_uri (uri))
    return;

  // Account is disabled, bye
  if (!is_enabled ())
    return;

  // Subscribe now
  if (state == Registered) {
    PTRACE(4, "Ekiga\tSubscribeToPresence for " << uri.c_str () << " (fetch)");
    opal_presentity->SubscribeToPresence (get_full_uri (uri));
  }
}


void
Opal::Account::unfetch (const std::string uri)
{
  if (is_supported_uri (uri) && opal_presentity) {
    opal_presentity->UnsubscribeFromPresence (get_full_uri (uri));
    Ekiga::Runtime::run_in_main (boost::bind (&Opal::Account::presence_status_in_main, this, uri, "unknown", ""));
  }
}


bool
Opal::Account::is_supported_uri (const std::string & uri)
{
  size_t pos = uri.find ("@");
  if (pos == string::npos)
    return false;

  std::string uri_host = uri.substr (++pos);
  if (uri_host != get_host ())
    return false;

  return true;
}


void
Opal::Account::handle_registration_event (Ekiga::Account::RegistrationState state_,
                                          const std::string info,
                                          const std::string & aor)
{
  if (state == state_)
    return; // The state did not change...

  switch (state_) {

  case Registered:

    if (state != Registered) {
      if (!aor.empty () && !opal_presentity)
        opal_presentity = endpoint.AddPresentity (aor);

      // Translators: this is a state, not an action, i.e. it should be read as
      // "(you are) registered", and not as "(you have been) registered"
      status = _("Registered");
      state = state_;
      failed_registration_already_notified = false;

      if (opal_presentity && false) {

        opal_presentity->SetPresenceChangeNotifier (PCREATE_PresenceChangeNotifier (OnPresenceChange));
        opal_presentity->GetAttributes().Set(OpalPresentity::AuthNameKey, get_authentication_username ());
        opal_presentity->GetAttributes().Set(OpalPresentity::AuthPasswordKey, get_password ());
        opal_presentity->GetAttributes().Set(SIP_Presentity::SubProtocolKey, "Agent");
        PTRACE (4, "Created presentity for " << get_aor());

        opal_presentity->Open ();

        for (Ekiga::HeapImpl<Opal::Presentity>::iterator iter = Ekiga::HeapImpl<Opal::Presentity>::begin ();
             iter != Ekiga::HeapImpl<Opal::Presentity>::end ();
             ++iter)
          fetch ((*iter)->get_uri());

        if (sip_endpoint)
          sip_endpoint->Subscribe (SIPSubscribe::MessageSummary, 3600, get_full_uri (get_aor ()));
      }
      boost::shared_ptr<Ekiga::PersonalDetails> details = personal_details.lock ();
      if (details && false)
        const_cast<Account*>(this)->publish (*details);
    }
    break;

  case Unregistered:

    // Translators: this is a state, not an action, i.e. it should be read as
    // "(you are) unregistered", and not as "(you have been) unregistered"
    status = _("Unregistered");
    failed_registration_already_notified = false;
    state = state_;

    /* delay destruction of this account until the
       unsubscriber thread has called back */
    if (dead)
      removed (this->shared_from_this ());
    break;

  case UnregistrationFailed:

    state = state_;
    status = _("Could not unregister");
    failed_registration_already_notified = false;
    if (!info.empty ())
      status = status + " (" + info + ")";
    break;

  case RegistrationFailed:

    state = state_;
    PTRACE (4, "Register failed, aborting registration");
    status = _("Could not register");
    if (!info.empty ())
      status = status + " (" + info + ")";
    if (!failed_registration_already_notified) {
      std::stringstream msg;
      msg << _("Could not register to ") << get_name ();
      boost::shared_ptr<Ekiga::Notification> notif (new Ekiga::Notification (Ekiga::Notification::Warning, msg.str (), info, _("Edit"), boost::bind (&Opal::Account::edit, (Opal::Account*) this)));
      boost::shared_ptr<Ekiga::NotificationCore> ncore = notification_core.lock ();
      if (ncore)
        ncore->push_notification (notif);
      failed_registration_already_notified = true;
    }
    break;

  case Processing:

    state = state_;
    status = _("Processing...");
  default:

    state = state_;
    break;
  }

  updated (this->shared_from_this ());
}


void
Opal::Account::handle_message_waiting_information (const std::string info)
{
  std::string::size_type loc = info.find ("/", 0);

  if (loc != std::string::npos) {

    std::stringstream new_messages;
    new_messages << info.substr (0, loc);
    new_messages >> message_waiting_number;
    if (message_waiting_number > 0) {
      boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput = audiooutput_core.lock ();
      if (audiooutput)
        audiooutput->play_event ("new-voicemail-sound");
    }
    updated (this->shared_from_this ());
  }
}


Opal::Account::Type
Opal::Account::get_type () const
{
  return type;
}


void
Opal::Account::OnPresenceChange (OpalPresentity& /*presentity*/,
                                 const std::auto_ptr<OpalPresenceInfo> info)
{
  std::string new_presence;
  std::string new_status = "";

  SIPURL sip_uri = SIPURL (info->m_entity);
  sip_uri.Sanitise (SIPURL::ExternalURI);
  std::string uri = sip_uri.AsString ();
  PCaselessString note = info->m_note;

  PTRACE (4, "Ekiga\tReceived a presence change (notify) for " << info->m_entity << ": state " << info->m_state << ", activities " << info->m_activities << ", note " << info->m_note);

  if (!uri.compare (0, 5, "pres:"))
    uri.replace (0, 5, "sip:");  // replace "pres:" sith "sip:" FIXME

  new_status = (const char*) info->m_note;
  switch (info->m_state) {

  case OpalPresenceInfo::Unchanged:
    if (info->m_infoData.Find("closed") != P_MAX_INDEX)
      new_presence = "offline";
    else
      // do not change presence
      return;
    break;
  case OpalPresenceInfo::Available:
    new_presence = "available";
    // First we rely on the note content
    // for older PABX system having custom implementations
    if (note.Find ("dnd") != P_MAX_INDEX
        || note.Find ("meeting") != P_MAX_INDEX
        || note.Find ("do not disturb") != P_MAX_INDEX
        || note.Find ("busy") != P_MAX_INDEX
        || info->m_activities.Contains ("busy"))
      new_presence = "busy";
    else if (note.Find ("away") != P_MAX_INDEX
             || note.Find ("out") != P_MAX_INDEX
             || note.Find ("vacation") != P_MAX_INDEX
             || note.Find ("holiday") != P_MAX_INDEX
             || note.Find ("lunch") != P_MAX_INDEX
             || info->m_activities.Contains ("away"))
      new_presence = "away";
    else if (note.Find ("phone") != P_MAX_INDEX
             || note.Find ("ringing") != P_MAX_INDEX
             || note.Find ("call") != P_MAX_INDEX)
      new_presence = "inacall";
    // Then we rely on the activities as defined in RFC 4480
    // they are order by importance as a user could have several
    // activities running on simultaneously, but we do not want
    // to handle that.
    else if (info->m_activities.Contains ("appointment")) {
      new_presence = "away";
      new_status = _("Appointment");
    }
    else if (info->m_activities.Contains ("breakfast")) {
      new_presence = "away";
      new_status = _("Breakfast");
    }
    else if (info->m_activities.Contains ("dinner")) {
      new_presence = "away";
      new_status = _("Dinner");
    }
    else if (info->m_activities.Contains ("vacation")
             || info->m_activities.Contains ("holiday")) {
      new_presence = "away";
      new_status = _("Holiday");
    }
    else if (info->m_activities.Contains ("in-transit")) {
      new_presence = "away";
      new_status = _("In transit");
    }
    else if (info->m_activities.Contains ("looking-for-work")) {
      new_presence = "away";
      new_status = _("Looking for work");
    }
    else if (info->m_activities.Contains ("lunch")) {
      new_presence = "away";
      new_status = _("Lunch");
    }
    else if (info->m_activities.Contains ("meal")) {
      new_presence = "away";
      new_status = _("Meal");
    }
    else if (info->m_activities.Contains ("meeting")) {
      new_presence = "away";
      new_status = _("Meeting");
    }
    else if (info->m_activities.Contains ("on-the-phone")) {
      new_presence = "inacall";
      new_status = _("On the phone");
    }
    else if (info->m_activities.Contains ("playing")) {
      new_presence = "away";
      new_status = _("Playing");
    }
    else if (info->m_activities.Contains ("shopping")) {
      new_presence = "away";
      new_status = _("Shopping");
    }
    else if (info->m_activities.Contains ("sleeping")) {
      new_presence = "away";
      new_status = _("Sleeping");
    }
    else if (info->m_activities.Contains ("working")) {
      new_presence = "busy";
      new_status = _("Working");
    }
    else if (info->m_activities.Contains ("other")) {
      new_presence = "away";
      new_status = "";
    }
    else if (info->m_activities.Contains ("performance")) {
      new_presence = "away";
      new_status = _("Performance");
    }
    else if (info->m_activities.Contains ("permanent-absence")) {
      new_presence = "away";
      new_status = _("Permanent Absence");
    }
    else if (info->m_activities.Contains ("presentation")) {
      new_presence = "away";
      new_status = _("Presentation");
    }
    else if (info->m_activities.Contains ("spectator")) {
      new_presence = "away";
      new_status = _("Spectator");
    }
    else if (info->m_activities.Contains ("steering")) {
      new_presence = "away";
      new_status = _("Steering");
    }
    else if (info->m_activities.Contains ("travel")) {
      new_presence = "away";
      new_status = _("Business or personal trip");
    }
    else if (info->m_activities.Contains ("tv")) {
      new_presence = "away";
      new_status = _("Watching TV");
    }
    else if (info->m_activities.Contains ("worship")) {
      new_presence = "away";
      new_status = _("Worship");
    }
    break;
  case OpalPresenceInfo::NoPresence:
    new_presence = "offline";
    break;
  case OpalPresenceInfo::EndState:
  case OpalPresenceInfo::StateCount:
    // the above two items are bookkeeping code, so do not consider them
    // shut up the compiler which checks all cases in switch
  case OpalPresenceInfo::InternalError:
  case OpalPresenceInfo::Forbidden:
  case OpalPresenceInfo::Unavailable:
  case OpalPresenceInfo::UnknownUser:
    // the 4 above states could lead to a visual indication
    // in Ekiga, but we do not handle it yet
  default:
    return;
    break;
  }

  Ekiga::Runtime::run_in_main (boost::bind (&Opal::Account::presence_status_in_main, this, uri, new_presence, new_status));
}


void
Opal::Account::presence_status_in_main (std::string uri,
                                        std::string uri_presence,
                                        std::string uri_status) const
{
  for (Ekiga::HeapImpl<Opal::Presentity>::const_iterator iter = Ekiga::HeapImpl<Opal::Presentity>::begin ();
       iter != Ekiga::HeapImpl<Opal::Presentity>::end ();
       ++iter) {

    if ((*iter)->has_uri (uri)) {

      (*iter)->set_presence (uri_presence);
      (*iter)->set_status (uri_status);
    }
  }
  presence_received (uri, uri_presence);
  status_received (uri, uri_status);
}


void
Opal::Account::on_rename_group (const std::list<std::string> & groups)
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request =
    boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Account::on_rename_group_form_submitted, this, _1, _2, _3, groups)));

  request->title (_("Renaming Groups"));
  request->editable_list ("groups", "",
                          groups, std::list<std::string>(),
                          false, true);

  Ekiga::Heap::questions (request);
}


struct rename_group_form_submitted_helper
{
  rename_group_form_submitted_helper (const std::string old_name_,
                                      const std::string new_name_):
    old_name(old_name_),
    new_name(new_name_)
  {}

  const std::string old_name;
  const std::string new_name;

  bool operator() (Ekiga::PresentityPtr pres)
  {
    Opal::PresentityPtr presentity = boost::dynamic_pointer_cast<Opal::Presentity> (pres);
    if (presentity)
      presentity->rename_group (old_name, new_name);
    return true;
  }
};


bool
Opal::Account::on_rename_group_form_submitted (bool submitted,
                                               Ekiga::Form& result,
                                               std::string& /*error*/,
                                               const std::list<std::string> & groups)
{
  if (!submitted)
    return false;

  std::list <std::string> new_groups = result.editable_list ("groups");

  std::list <std::string>::const_iterator nit = new_groups.begin ();
  std::list <std::string>::const_iterator it = groups.begin ();

  for (std::pair <std::list<std::string>::const_iterator, std::list<std::string>::const_iterator> i(it, nit);
       i.first != groups.end ();
       ++i.first, ++i.second) {

    if (*i.first != *i.second) {

      rename_group_form_submitted_helper helper (*i.first, *i.second);
      visit_presentities (boost::ref (helper));
    }
  }

  return true;
}


// FIXME: This is awful
void
Opal::Account::decide_type ()
{
  const std::string host = get_host ();

  type = Account::SIP;
}


const PString
Opal::Account::get_full_uri (const PString & uri) const
{
  PString parameters;
  PINDEX j;
  if (opal_presentity && (j = opal_presentity->GetAOR ().AsString ().Find (";")) != P_MAX_INDEX)
    parameters = opal_presentity->GetAOR ().AsString ().Mid (j);

  if (uri.IsEmpty () && opal_presentity)
    return opal_presentity->GetAOR ().AsString ();

  return uri + parameters;
}
