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
 *                         gactor-menu.h  -  description
 *                         ----------------------------------
 *   begin                : written in 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : An actor object menu definition
 *
 */

#ifndef __GACTOR_MENU_H__
#define __GACTOR_MENU_H__

#include "actor.h"
#include "null-deleter.h"

#include <gtk/gtk.h>

namespace Ekiga {

  class GActorMenu;
  typedef boost::shared_ptr<GActorMenu> GActorMenuPtr;
  typedef std::list< GActorMenuPtr > GActorMenuStore;

  /* GActorMenu
   *
   * We will have one GActorMenu per UI object displaying an Actor.
   *
   * Each Actor object exposes Actions. Those actions are actions
   * specific to the Actor. They can be triggered, for example, by the
   * various GActorMenu associated with the different views of the
   * Actor.
   *
   * The GActorMenu is the binding between GIO Actions and Ekiga::Actions.
   *
   * When a GActorMenu is created from an Actor, all available Actions are
   * automatically registered through the GIO subsystem. Actions are
   * unregistered from the GIO subsystem when the GActorMenu is destroyed.
   *
   * You should not have several GActorMenu with conflicting Actions alive
   * at the same time (e.g. several GActorMenu associated to different contacts
   * each exposing a call action).
   */
  class GActorMenu
  {
  public:

    GActorMenu (Actor & obj);
    GActorMenu (Actor & obj,
                const std::string & name,
                const std::string & context = "win");
    virtual ~GActorMenu ();


    /** Activate the action of the given name. The first enabled action
     *  (if any) is activated when no action name is given.
     * @param name is the action name, or can be left empty.
     */
    virtual void activate (const std::string & name = "");


    /** Return a pointer to the GMenuModel corresponding to the current GActorMenu
     *  and the list of GActorMenus given as argument.
     *
     * @return a pointer to the GMenuModel, NULL if none (no action, or all actions
     *         are disabled).
     */
    GMenuModel *get_model (const GActorMenuStore & store = GActorMenuStore (),
                           bool display_section_label = true);

    /** Return a pointer to the GtkMenu corresponding to the current GActorMenu
     *  and the list of GActorMenus given as argument.
     *
     * @return a pointer to the GtkMenu, NULL if none (no action, or all actions
     *         are disabled).
     */
    GtkWidget *get_menu (const Ekiga::GActorMenuStore & store = GActorMenuStore ());


    /** Return the number of actions of the GActorMenu. The counter is only
     *  updated after get () has been called.
     */
    unsigned size ();

  protected:
    /** Sync GIO actions with Actions */
    virtual void sync_gio_actions ();

    /** Add GIO action with given name */
    virtual void add_gio_action (const std::string & name);

    /** Add GIO action corresponding to the given Action */
    virtual void add_gio_action (ActionPtr a);

    /** Remove GIO action with the given name */
    virtual void remove_gio_action (const std::string & name);

    /** Return the XML representation of the enabled Actions */
    virtual const std::string as_xml (const std::string & id = "",
                                      bool display_section_label = true);

    /** Return the XML representation of the full menu with
     * enabled Actions.
     */
    virtual const std::string build ();

    Actor & obj;

  private:
    void ctor_init ();

    Ekiga::scoped_connections conns;
    unsigned n;
    GtkBuilder *builder;
    std::string name;
    std::string context;
  };
}
#endif
