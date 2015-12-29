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
 *                         history-book.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : implementation of the book of the call history
 *
 */

#include "config.h"
#include "history-book.h"

#include <glib/gi18n.h>

#define CALL_HISTORY_KEY "call-history"


boost::shared_ptr<History::Book>
History::Book::create (Ekiga::ServiceCore & core)
{
  boost::shared_ptr<History::Book> book = boost::shared_ptr<History::Book> (new History::Book (core));
  book->load ();

  return book;
}


History::Book::Book (Ekiga::ServiceCore& core):
  contact_core(core.get<Ekiga::ContactCore>("contact-core")), doc()
{
  boost::shared_ptr<Ekiga::CallCore> call_core = core.get<Ekiga::CallCore> ("call-core");

  connections.add (call_core->missed_call.connect (boost::bind (&History::Book::on_missed_call, this, _1)));
  connections.add (call_core->cleared_call.connect (boost::bind (&History::Book::on_cleared_call, this, _1, _2)));

  add_action (Ekiga::ActionPtr (new Ekiga::Action ("history_book_clear",
                                                   _("Clear History"),
                                                   boost::bind (&History::Book::clear,
                                                                this))));
}


History::Book::~Book ()
{
}


const std::string
History::Book::get_name () const
{
  return "Call history";
}

void
History::Book::visit_contacts (boost::function1<bool, Ekiga::ContactPtr> visitor) const
{
  for (std::list<ContactPtr>::const_iterator iter = ordered_contacts.begin ();
       iter != ordered_contacts.end();
       ++iter)
    visitor (*iter);
}

void
History::Book::add (xmlNodePtr node)
{
  boost::shared_ptr<Ekiga::ContactCore> ccore = contact_core.lock ();
  common_add (History::Contact::create (ccore, doc, node));
}

void
History::Book::add (const std::string & name,
		    const std::string & uri,
                    const time_t & call_start,
                    const std::string & call_duration,
		    const call_type c_t)
{
  boost::shared_ptr<Ekiga::ContactCore> ccore = contact_core.lock ();

  if ( !uri.empty ()) {
    size_t pos;

    pos = uri.find(':');
    std::string _uri = uri.substr(pos == std::string::npos ? 0 : pos + 1);
    _uri = _uri.substr(0, _uri.find('@'));

    xmlNodePtr root = xmlDocGetRootElement (doc.get ());

    boost::shared_ptr<History::Contact> contact = History::Contact::create (ccore, doc,
      (name == uri ? _uri : _uri + " (" + name + ")"), uri, call_start, call_duration, c_t);

    xmlAddChild (root, contact->get_node ());

    save ();

    common_add (contact);

    enforce_size_limit ();
  }
}

const std::list<std::string>
History::Book::existing_groups () const
{
  // here it's more logical to lie
  return std::list<std::string> ();
}

const std::string
History::Book::get_status () const
{
  return ""; // nothing special here
}

void
History::Book::load ()
{
  xmlNodePtr root = NULL;

  contacts_settings = boost::shared_ptr<Ekiga::Settings> (new Ekiga::Settings (CONTACTS_SCHEMA));
  std::string raw = contacts_settings->get_string (CALL_HISTORY_KEY);

  if (!raw.empty ()) {

    doc = boost::shared_ptr<xmlDoc> (xmlRecoverMemory (raw.c_str (), raw.length ()), xmlFreeDoc);
    if ( !doc)
      doc = boost::shared_ptr<xmlDoc> (xmlNewDoc (BAD_CAST "1.0"), xmlFreeDoc);

    root = xmlDocGetRootElement (doc.get ());
    if (root == NULL) {

      root = xmlNewDocNode (doc.get (), NULL, BAD_CAST "list", NULL);
      xmlDocSetRootElement (doc.get (), root);
    }

    for (xmlNodePtr child = root->children;
	 child != NULL;
	 child = child->next)
      if (child->type == XML_ELEMENT_NODE
	  && child->name != NULL
	  && xmlStrEqual (BAD_CAST ("entry"), child->name))
        add (child);

  }
  else {

    doc = boost::shared_ptr<xmlDoc> (xmlNewDoc (BAD_CAST "1.0"), xmlFreeDoc);
    root = xmlNewDocNode (doc.get (), NULL, BAD_CAST "list", NULL);
    xmlDocSetRootElement (doc.get (), root);
  }

  enforce_size_limit ();
}

void
History::Book::save () const
{
  xmlChar *buffer = NULL;
  int size = 0;

  xmlDocDumpMemory (doc.get (), &buffer, &size);

  contacts_settings->set_string (CALL_HISTORY_KEY, (const char *)buffer);

  xmlFree (buffer);
}

void
History::Book::clear ()
{
  xmlNodePtr root = NULL;

  std::list<ContactPtr> old_contacts = ordered_contacts;
  ordered_contacts.clear ();

  cleared ();
  updated (this->shared_from_this ());

  for (std::list<ContactPtr>::iterator iter = old_contacts.begin ();
       iter != old_contacts.end();
       ++iter)
    contact_removed (*iter);

  doc = boost::shared_ptr<xmlDoc> (xmlNewDoc (BAD_CAST "1.0"), xmlFreeDoc);
  root = xmlNewDocNode (doc.get (), NULL, BAD_CAST "list", NULL);
  xmlDocSetRootElement (doc.get (), root);

  save ();
}

void
History::Book::on_missed_call (boost::shared_ptr<Ekiga::Call> call)
{
  add (call->get_remote_party_name (),
       call->get_remote_uri (),
       call->get_start_time (),
       call->get_duration (),
       MISSED);
}

void
History::Book::on_cleared_call (boost::shared_ptr<Ekiga::Call> call,
				std::string /*message*/)
{
  add (call->get_remote_party_name (),
       call->get_remote_uri (),
       call->get_start_time (),
       call->get_duration (),
       (call->is_outgoing ()?PLACED:RECEIVED));
}

void
History::Book::common_add (ContactPtr contact)
{
  contact->questions.connect (boost::ref (questions));
  /* nothing to do when the contact is updated or removed:
   * they don't get updated and only get removed all at the same time
   */

  ordered_contacts.push_back (contact);
  contact_added (contact);
  updated (this->shared_from_this ());
}

void
History::Book::enforce_size_limit()
{
  bool flag = false;

  while (ordered_contacts.size() > 100) {

    ContactPtr contact = ordered_contacts.front ();
    ordered_contacts.pop_front();
    xmlNodePtr node = contact->get_node ();
    contact->removed (contact);
    xmlUnlinkNode(node);
    xmlFreeNode(node);
    flag = true;
  }

  if (flag) {

    save ();
    updated (this->shared_from_this ());
  }
}
