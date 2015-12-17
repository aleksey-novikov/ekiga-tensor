/*
 * Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>

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
 *                         foe-list.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2014 by Julien Puydt
 *   copyright            : (c) 2014 by Julien Puydt
 *   description          : interface of a delegate
 *
 */

#include <glib/gi18n.h>

#include "ekiga-settings.h"

#include "foe-list.h"
#include "form-request-simple.h"

Ekiga::FoeList::FoeList(boost::shared_ptr<FriendOrFoe> fof)
{
  /* This Action can be added to the FriendOrFoe */
  Ekiga::URIActionProvider::add_action (*fof, Ekiga::ActionPtr (new Ekiga::Action ("blacklist-edit", _("_Edit Blacklist"),
                                                                                   boost::bind (&Ekiga::FoeList::edit_foes, this))));
}


Ekiga::FoeList::~FoeList()
{
}


Ekiga::FriendOrFoe::Identification
Ekiga::FoeList::decide (const std::string /*domain*/,
			const std::string uri)
{
  boost::scoped_ptr<Ekiga::Settings> settings(new Ekiga::Settings (CONTACTS_SCHEMA));
  std::list<std::string> foes = settings->get_string_list ("foe-list");
  Ekiga::FriendOrFoe::Identification result = Ekiga::FriendOrFoe::Unknown;

  if (std::find (foes.begin (), foes.end (), uri) != foes.end ())
    result = Ekiga::FriendOrFoe::Foe;

  return result;
}


void
Ekiga::FoeList::add_foe (const std::string token)
{
  boost::scoped_ptr<Ekiga::Settings> settings(new Ekiga::Settings (CONTACTS_SCHEMA));
  std::list<std::string> foes = settings->get_string_list ("foe-list");
  foes.push_back (token);
  settings->set_string_list ("foe-list", foes);
}

void
Ekiga::FoeList::edit_foes ()
{
  boost::shared_ptr<Ekiga::FormRequestSimple> request =
    boost::shared_ptr<Ekiga::FormRequestSimple> (new Ekiga::FormRequestSimple (boost::bind (&Ekiga::FoeList::on_edit_foes_form_submitted, this, _1, _2, _3)));

  request->title (_("Edit the Blacklist"));

  boost::scoped_ptr<Ekiga::Settings> settings(new Ekiga::Settings (CONTACTS_SCHEMA));
  std::list<std::string> foes(settings->get_string_list ("foe-list"));

  request->editable_list ("foes",
                          _("Blacklist"),
			 std::list<std::string> (foes.begin (), foes.end ()),
			 std::list<std::string>());

  questions (request);
}

bool
Ekiga::FoeList::on_edit_foes_form_submitted (bool submitted,
                                             Ekiga::Form& result,
                                             G_GNUC_UNUSED std::string& error)
{
  if (!submitted)
    return false;

  std::list<std::string> foes = result.editable_list ("foes");
  boost::scoped_ptr<Ekiga::Settings> settings(new Ekiga::Settings (CONTACTS_SCHEMA));
  settings->set_string_list ("foe-list", foes);

  return true;
}

void
Ekiga::FoeList::pull_actions (Actor & actor,
                              G_GNUC_UNUSED const std::string & display_name,
                              const std::string & uri)
{
  Ekiga::URIActionProvider::remove_action (actor, "blacklist-add");
  if (decide (std::string (), uri) != Ekiga::FriendOrFoe::Foe)
    Ekiga::URIActionProvider::add_action (actor, Ekiga::ActionPtr (new Ekiga::Action ("blacklist-add", _("_Blacklist"),
                                                                                      boost::bind (&Ekiga::FoeList::add_foe, this, uri))));
}
