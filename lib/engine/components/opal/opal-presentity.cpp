/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2013 Damien Sandras <dsandras@seconix.com>

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
 *                         opal-presentity.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2013 by Julien Puydt
 *   copyright            : (c) 2013 by Julien Puydt
 *   description          : implementation of a presentity for an opal account roster
 *
 */

#include <algorithm>
#include <set>
#include <glib/gi18n.h>

#include "form-request-simple.h"
#include "robust-xml.h"

#include "opal-presentity.h"
#include "opal-account.h"


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


/* we call the presence core for help, which needs a smart pointer
 * to this... so we make a dummy one!
 */
struct null_deleter
{
    void operator()(void const *) const
    {
    }
};


xmlNodePtr
Opal::Presentity::build_node (const std::string name,
                              const std::string uri,
                              const std::list<std::string> groups)
{
  xmlNodePtr node = xmlNewNode (NULL, BAD_CAST "entry");
  xmlSetProp (node, BAD_CAST "uri", BAD_CAST uri.c_str ());
  xmlNewChild (node, NULL,
               BAD_CAST "name",
               BAD_CAST robust_xmlEscape (node->doc,
                                          name).c_str ());
  for (std::list<std::string>::const_iterator iter = groups.begin ();
       iter != groups.end ();
       ++iter)
    xmlNewChild (node, NULL,
                 BAD_CAST "group",
                 BAD_CAST robust_xmlEscape (node->doc,
                                            *iter).c_str ());

  return node;
}


boost::shared_ptr<Opal::Presentity>
Opal::Presentity::create (Account & _account,
                          boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                          boost::function0<std::list<std::string> > _existing_groups,
                          xmlNodePtr _node)
{
  boost::shared_ptr<Opal::Presentity> presentity =
    boost::shared_ptr<Opal::Presentity> (new Opal::Presentity (_account,
                                                               _presence_core,
                                                               _existing_groups,
                                                               _node));
  presentity->add_actions ();

  return presentity;
}


Opal::Presentity::Presentity (Opal::Account & account_,
                              boost::weak_ptr<Ekiga::PresenceCore> presence_core_,
                              boost::function0<std::list<std::string> > existing_groups_,
                              xmlNodePtr node_):
  account(account_),
  presence_core(presence_core_),
  existing_groups(existing_groups_),
  node(node_),
  presence("unknown")
{
}


Opal::Presentity::~Presentity ()
{
}


void
Opal::Presentity::add_actions ()
{
  /* Pull actions */
  boost::shared_ptr<Ekiga::PresenceCore> pcore = presence_core.lock ();
  if (pcore)
    pcore->pull_actions (*this, get_name (), get_uri ());

  add_action (Ekiga::ActionPtr (new Ekiga::Action ("edit", _("_Edit"),
                                                   boost::bind (&Opal::Presentity::edit_presentity, this))));
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("remove", _("_Remove"),
                                                   boost::bind (&Opal::Presentity::remove, this))));
  add_action (Ekiga::ActionPtr (new Ekiga::Action ("rename", _("Rename _Groups"),
                                                   boost::bind (&Opal::Account::on_rename_group,
                                                                (Opal::Account *) &account, get_groups ()))));
}


const std::string
Opal::Presentity::get_name () const
{
  std::string name;
  xmlChar* xml_str = NULL;

  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST ("name"), child->name)) {

        xml_str = xmlNodeGetContent (child);
        if (xml_str != NULL) {

          name = (const char*)xml_str;
          xmlFree (xml_str);
        } else {

          name = _("Unnamed");
        }
      }
    }
  }

  return name;
}


const std::string
Opal::Presentity::get_presence () const
{
  return presence;
}


const std::string
Opal::Presentity::get_status () const
{
  return status;
}


const std::list<std::string>
Opal::Presentity::get_groups () const
{
  std::list<std::string> groups;

  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST ("group"), child->name)) {

        xmlChar* xml_str = xmlNodeGetContent (child);
        if (xml_str != NULL) {

          groups.push_back ((const char*) xml_str);
          xmlFree (xml_str);
        }
      }
    }
  }

  return groups;
}


const std::string
Opal::Presentity::get_uri () const
{
  std::string uri;
  xmlChar* xml_str = NULL;

  xml_str = xmlGetProp (node, BAD_CAST "uri");
  if (xml_str != NULL) {

    uri = (const char*)xml_str;
    xmlFree (xml_str);
  }

  return uri;
}


bool
Opal::Presentity::has_uri (const std::string uri) const
{
  return uri == get_uri ();
}


void
Opal::Presentity::set_presence (const std::string presence_)
{
  presence = presence_;
  updated (this->shared_from_this ());
}


void
Opal::Presentity::set_status (const std::string status_)
{
  status = status_;
  updated (this->shared_from_this ());
}


void
Opal::Presentity::edit_presentity ()
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request =
    boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Opal::Presentity::edit_presentity_form_submitted, this, _1, _2, _3)));

  /* Translators: This is Edit name of the contact
   * e.g. Editing Contact Claire Fleury.
   */
  char *title = g_strdup_printf (_("Editing Contact %s"), get_name ().c_str ());
  request->title (title);
  g_free (title);

  request->action (_("Done"));
  request->text ("name", _("Name"),
                 get_name (),
                 _("John Doe"),
                 Ekiga::FormVisitor::STANDARD,
                 false, false);
  request->text ("uri", _("URI"),
                 get_uri (),
                 _("sip:username@ekiga.net"),
                 Ekiga::FormVisitor::URI,
                 false, false);

  request->editable_list ("groups", _("Groups"),
                          get_groups (), existing_groups ());

  questions (request);
}


bool
Opal::Presentity::edit_presentity_form_submitted (bool submitted,
                                                  Ekiga::Form &result,
                                                  std::string &error)
{
  if (!submitted)
    return false;

  const std::string new_name = result.text ("name");
  const std::list<std::string> groups = get_groups ();
  const std::list<std::string> new_groups = result.editable_list ("groups");
  std::string new_uri = result.text ("uri");
  const std::string uri = get_uri ();
  std::set<xmlNodePtr> nodes_to_remove;

  if (new_name.empty ()) {
    error = _("You did not provide a valid name");
    return false;
  }
  else if (new_uri.empty ()) {
    error = _("You did not provide a valid address");
    return false;
  }

  new_uri = canonize_uri (new_uri);

  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL)
      if (xmlStrEqual (BAD_CAST ("name"), child->name))
        robust_xmlNodeSetContent (node, &child, "name", new_name);
  }

  if (uri != new_uri) {
    xmlSetProp (node, (const xmlChar*)"uri", (const xmlChar*)new_uri.c_str ());
    account.unfetch (uri);
    account.fetch (new_uri);
    Ekiga::Runtime::run_in_main (boost::bind (&Opal::Account::presence_status_in_main, &account, new_uri, "unknown", ""));
  }

  // the first loop looks at groups we were in: are we still in?
  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST ("group"), child->name)) {

        xmlChar* xml_str = xmlNodeGetContent (child);

        if (xml_str != NULL) {
          if (std::find (new_groups.begin (), new_groups.end (), (const char*) xml_str) == new_groups.end ())
            nodes_to_remove.insert (child); // don't free what we loop on!
          xmlFree (xml_str);
        }
      }
    }
  }

  // ok, now we can clean up!
  for (std::set<xmlNodePtr>::iterator iter = nodes_to_remove.begin ();
       iter != nodes_to_remove.end ();
       ++iter) {

    xmlUnlinkNode (*iter);
    xmlFreeNode (*iter);
  }

  // the second loop looks for groups we weren't in but are now
  for (std::list<std::string>::const_iterator iter = new_groups.begin ();
       iter != new_groups.end ();
       iter++) {

    if (std::find (groups.begin (), groups.end (), *iter) == groups.end ())
      xmlNewChild (node, NULL,
                   BAD_CAST "group",
                   BAD_CAST robust_xmlEscape (node->doc, *iter).c_str ());
  }

  updated (this->shared_from_this ());
  trigger_saving ();

  return true;
}


void
Opal::Presentity::rename_group (const std::string old_name,
                                const std::string new_name)
{
  bool old_name_present = false;
  bool already_in_new_name = false;
  std::set<xmlNodePtr> nodes_to_remove;

  /* remove the old name's node
   * and check if we aren't already in the new name's group
   */
  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST ("group"), child->name)) {

        xmlChar* xml_str = xmlNodeGetContent (child);

        if (xml_str != NULL) {

          if (!xmlStrcasecmp ((const xmlChar*)old_name.c_str (), xml_str)) {
            nodes_to_remove.insert (child); // don't free what we loop on!
            old_name_present = true;
          }

          if (!xmlStrcasecmp ((const xmlChar*)new_name.c_str (), xml_str))
            already_in_new_name = true;

          xmlFree (xml_str);
        }
      }
    }
  }

  // ok, now we can clean up!
  for (std::set<xmlNodePtr>::iterator iter = nodes_to_remove.begin ();
       iter != nodes_to_remove.end ();
       ++iter) {

    xmlUnlinkNode (*iter);
    xmlFreeNode (*iter);
  }

  if (old_name_present && !already_in_new_name)
    xmlNewChild (node, NULL,
                 BAD_CAST "group",
                 BAD_CAST robust_xmlEscape (node->doc,
                                            new_name).c_str ());


  updated (this->shared_from_this ());
  trigger_saving ();
}


void
Opal::Presentity::remove ()
{
  xmlUnlinkNode (node);
  xmlFreeNode (node);

  trigger_saving ();
  removed (this->shared_from_this ());
}
