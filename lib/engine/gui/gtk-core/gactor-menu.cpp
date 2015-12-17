/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
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
 *                         gactor-menu.cpp  -  description
 *                         -------------------------------
 *   begin                : written in 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An Actor object menu implementation
 *
 */

#include <gio/gio.h>
#include <string>

#include "action.h"
#include "gactor-menu.h"

#include <iostream>

static void
action_activated (GSimpleAction *a,
                  G_GNUC_UNUSED GVariant *p,
                  gpointer data)
{
  const char *action = (const char *) g_object_get_data (G_OBJECT (a), "action");
  Ekiga::GActorMenu *menu = (Ekiga::GActorMenu *) data;

  g_return_if_fail (action && menu);
  menu->activate (action);
}


Ekiga::GActorMenu::GActorMenu (Ekiga::Actor & _obj) : obj (_obj)
{
  ctor_init ();
  context = "win";
}


Ekiga::GActorMenu::GActorMenu (Ekiga::Actor & _obj,
                               const std::string & _name,
                               const std::string & _context) : obj (_obj), name (_name), context (_context)
{
  ctor_init ();
}


Ekiga::GActorMenu::~GActorMenu ()
{
  Actor::const_iterator it;

  for (it = obj.begin(); it != obj.end(); ++it)
    g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                                (*it)->get_name ().c_str ());

  g_object_unref (builder);
}


void
Ekiga::GActorMenu::activate (const std::string & _name)
{
  Actor::const_iterator it;

  for (it = obj.begin(); it != obj.end(); ++it) {
    if (_name.empty () || (*it)->get_name () == _name) {
      (*it)->activate ();
      return;
    }
  }
}


GMenuModel *
Ekiga::GActorMenu::get_model (const Ekiga::GActorMenuStore & store,
                              bool display_section_label)
{
  int c = 0;
  std::string content = as_xml (std::string (), display_section_label);

  for (Ekiga::GActorMenuStore::const_iterator it = store.begin ();
       it != store.end ();
       it++) {
    content = content + (*it)->as_xml (std::string (), display_section_label);
    c += (*it)->size ();
  }
  content = "<?xml_content version=\"1.0\"?>"
            "<interface>"
            "  <menu id=\"menu\">" + content + "</menu>"
            "</interface>";

  gtk_builder_add_from_string (builder, content.c_str (), -1, NULL);
  return (n > 0 || c > 0 ? G_MENU_MODEL (gtk_builder_get_object (builder, "menu")) : NULL);
}


GtkWidget *
Ekiga::GActorMenu::get_menu (const Ekiga::GActorMenuStore & store)
{
  GMenuModel *model = get_model (store, false);

  if (!model)
    return NULL;

  GtkWidget *menu = gtk_menu_new_from_model (model);
  gtk_widget_insert_action_group (menu, context.c_str (), G_ACTION_GROUP (g_application_get_default ()));
  g_object_ref (menu);

  return menu;
}


unsigned
Ekiga::GActorMenu::size ()
{
  return n;
}


void
Ekiga::GActorMenu::sync_gio_actions ()
{
  Actor::const_iterator it;

  for (it = obj.begin(); it != obj.end(); ++it) {
    if ((*it)->is_enabled ())
      add_gio_action (*it);
    else
      remove_gio_action ((*it)->get_name ());
  }
}


void
Ekiga::GActorMenu::add_gio_action (const std::string & _name)
{
  Actor::const_iterator it;

  for (it = obj.begin(); it != obj.end(); ++it) {
    if ((*it)->get_name () == _name) {
      add_gio_action (*it);
      return;
    }
  }
}


void
Ekiga::GActorMenu::add_gio_action (Ekiga::ActionPtr a)
{
  GSimpleAction *action = NULL;

  /* Action is disabled or already present */
  if (!a->is_enabled ())
    return;

  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                              a->get_name ().c_str ());

  action = g_simple_action_new (a->get_name ().c_str (), NULL);
  g_object_set_data_full (G_OBJECT (action), "action",
                          g_strdup (a->get_name ().c_str ()),
                          (GDestroyNotify) g_free);
  g_action_map_add_action (G_ACTION_MAP (g_application_get_default ()),
                           G_ACTION (action));
  g_signal_connect (action, "activate",
                    G_CALLBACK (action_activated),
                    (gpointer) this);
  g_object_unref (action);
}


void
Ekiga::GActorMenu::remove_gio_action (const std::string & _name)
{
  g_action_map_remove_action (G_ACTION_MAP (g_application_get_default ()),
                              _name.c_str ());
}


const std::string
Ekiga::GActorMenu::as_xml (const std::string & id,
                           bool display_section_label)
{
  Actor::const_iterator it;
  std::string xml_content;
  n = 0;

  if (!id.empty ())
   xml_content += "    <section id=\"" + id + "\">";
  else
   xml_content += "    <section>";

  if (display_section_label && !name.empty ())
   xml_content +=  "        <attribute name=\"label\" translatable=\"yes\">"+name+"</attribute>";

  for (it = obj.begin(); it != obj.end(); ++it) {

    xml_content +=
      "      <item>"
      "        <attribute name=\"label\" translatable=\"yes\">"+(*it)->get_description ()+"</attribute>"
      "        <attribute name=\"action\">" + context + "."+(*it)->get_name ()+"</attribute>"
      "      </item>";
    n++;
  }

  xml_content +=
    "    </section>";

  return (n > 0 ? xml_content : "");
}


const std::string
Ekiga::GActorMenu::build ()
{
  std::string xml_content = "<menu id=\"menu\">" + as_xml () + "</menu>";
  return "<?xml_content version=\"1.0\"?><interface>" + xml_content + "</interface>";
}


void
Ekiga::GActorMenu::ctor_init ()
{
  n = 0;
  builder = gtk_builder_new ();

  sync_gio_actions ();

  conns.add (obj.action_enabled.connect (boost::bind (static_cast<void (Ekiga::GActorMenu::*)(const std::string&)>(&Ekiga::GActorMenu::add_gio_action), this, _1)));
  conns.add (obj.action_disabled.connect (boost::bind (&Ekiga::GActorMenu::remove_gio_action, this, _1)));

  conns.add (obj.action_added.connect (boost::bind (static_cast<void (Ekiga::GActorMenu::*)(const std::string&)>(&Ekiga::GActorMenu::add_gio_action), this, _1)));
  conns.add (obj.action_removed.connect (boost::bind (&Ekiga::GActorMenu::remove_gio_action, this, _1)));
}
