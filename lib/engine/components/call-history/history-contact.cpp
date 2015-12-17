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
 *                         history-contact.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : implementation of a call history entry
 *
 */

#include "history-contact.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "call-core.h"

#include "robust-xml.h"

/* at one point we will return a smart pointer on this... and if we don't use
 * a false smart pointer, we will crash : the reference count isn't embedded!
 */
struct null_deleter
{
    void operator()(void const *) const
    {
    }
};


boost::shared_ptr<History::Contact>
History::Contact::create (boost::shared_ptr<Ekiga::ContactCore> _contact_core,
                          boost::shared_ptr<xmlDoc> _doc,
                          xmlNodePtr _node)
{
  return boost::shared_ptr<History::Contact> (new History::Contact (_contact_core, _doc, _node));
}


boost::shared_ptr<History::Contact>
History::Contact::create (boost::shared_ptr<Ekiga::ContactCore> _contact_core,
                          boost::shared_ptr<xmlDoc> _doc,
                          const std::string _name,
                          const std::string _uri,
                          time_t _call_start,
                          const std::string _call_duration,
                          call_type c_t)
{
  return boost::shared_ptr<History::Contact> (new History::Contact (_contact_core, _doc, _name, _uri, _call_start, _call_duration, c_t));
}


History::Contact::Contact (boost::shared_ptr<Ekiga::ContactCore> _contact_core,
			   boost::shared_ptr<xmlDoc> _doc,
			   xmlNodePtr _node):
  contact_core(_contact_core), doc(_doc), node(_node)
{
  xmlChar* xml_str = NULL;

  xml_str = xmlGetProp (node, (const xmlChar *)"type");
  if (xml_str != NULL) {

    m_type = (call_type)(xml_str[0] - '0'); // FIXME: I don't like it!
    xmlFree (xml_str);
  }

  xml_str = xmlGetProp (node, (const xmlChar *)"uri");
  if (xml_str != NULL) {

    uri = (const char *)xml_str;
    xmlFree (xml_str);
  }

  for (xmlNodePtr child = node->children ;
       child != NULL ;
       child = child->next) {

    if (child->type == XML_ELEMENT_NODE
        && child->name != NULL) {

      if (xmlStrEqual (BAD_CAST ("name"), child->name)) {

        xml_str = xmlNodeGetContent (child);
	if (xml_str != NULL)
	  name = (const char *)xml_str;
        xmlFree (xml_str);
      }

      if (xmlStrEqual (BAD_CAST ("call_start"), child->name)) {

        xml_str = xmlNodeGetContent (child);
	if (xml_str != NULL)
	  call_start = (time_t) strtoll((const char *) xml_str, NULL, 0);
        xmlFree (xml_str);
      }

      if (xmlStrEqual (BAD_CAST ("call_duration"), child->name)) {

        xml_str = xmlNodeGetContent (child);
	if (xml_str != NULL)
	  call_duration = (const char *) xml_str;
        xmlFree (xml_str);
      }
    }
  }

  /* Pull actions */
  boost::shared_ptr<Ekiga::ContactCore> ccore = contact_core.lock ();
  if (ccore)
    ccore->pull_actions (*this, name, uri);
}


History::Contact::Contact (boost::shared_ptr<Ekiga::ContactCore> _contact_core,
			   boost::shared_ptr<xmlDoc> _doc,
			   const std::string _name,
			   const std::string _uri,
                           time_t _call_start,
                           const std::string _call_duration,
			   call_type c_t):
  contact_core(_contact_core), doc(_doc),
  name(_name), uri(_uri), call_start(_call_start), call_duration(_call_duration), m_type(c_t)
{
  gchar* tmp = NULL;
  std::string callp;

  node = xmlNewNode (NULL, BAD_CAST "entry");

  xmlSetProp (node, BAD_CAST "uri", BAD_CAST uri.c_str ());
  xmlNewChild (node, NULL,
	       BAD_CAST "name",
	       BAD_CAST robust_xmlEscape (node->doc, name).c_str ());

  tmp = g_strdup_printf ("%" G_GINT64_FORMAT, (gint64)call_start);
  xmlNewChild (node, NULL,
	       BAD_CAST "call_start", BAD_CAST tmp);
  g_free (tmp);

  xmlNewChild (node, NULL,
	       BAD_CAST "call_duration", BAD_CAST call_duration.c_str ());

  /* FIXME: I don't like the way it's done */
  tmp = g_strdup_printf ("%d", m_type);
  xmlSetProp (node, BAD_CAST "type", BAD_CAST tmp);
  g_free (tmp);

  /* Pull actions */
  boost::shared_ptr<Ekiga::ContactCore> ccore = contact_core.lock ();
  if (ccore)
    ccore->pull_actions (*this, name, uri);
}

History::Contact::~Contact ()
{
}

const std::string
History::Contact::get_name () const
{
  return name;
}

bool
History::Contact::has_uri (const std::string uri_) const
{
  return uri == uri_;
}

const std::set<std::string>
History::Contact::get_groups () const
{
  std::set<std::string> groups;

  switch (m_type) {
  case RECEIVED:
    groups.insert (_("Received"));
    break;
  case PLACED:
    groups.insert (_("Placed"));
    break;
  case MISSED:
    groups.insert (_("Missed"));
    break;

  default:
    groups.insert ("AIE!!");
  }

  return groups;
}

xmlNodePtr
History::Contact::get_node ()
{
  return node;
}

History::call_type
History::Contact::get_type () const
{
  return m_type;
}

time_t
History::Contact::get_call_start () const
{
  return call_start;
}

const std::string
History::Contact::get_call_duration () const
{
  return call_duration;
}

const std::string
History::Contact::get_uri () const
{
  return uri;
}
