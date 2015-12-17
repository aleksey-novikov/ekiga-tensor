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
 *                         opal-presentity.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2013 by Julien Puydt
 *   copyright            : (c) 2013 by Julien Puydt
 *   description          : declaration of a presentity for an opal account roster
 *
 */



#ifndef __OPAL_PRESENTITY_H__
#define __OPAL_PRESENTITY_H__

#include <libxml/tree.h>
#include <boost/smart_ptr.hpp>

#include "form.h"

#include "presence-core.h"

#include "presentity.h"
#include "dynamic-object.h"


namespace Opal
{
  class Account;
  class Cluster;

  /* This class implements and Ekiga::Presentity, stored as a node
   * in an XML document -- the code is relative to that node, so could
   * probably be abstracted, should the need arise!
   */

  class Presentity:
      public Ekiga::Presentity,
      public Ekiga::DynamicObject<Presentity>
  {
  public:

    /* build a node describing a valid presentity, which the caller
     * will then use to create a valid instance using the ctor */
    static xmlNodePtr build_node (const std::string name_,
                                  const std::string uri_,
                                  const std::list<std::string> groups_);

    static boost::shared_ptr<Presentity> create (Account & _account,
                                                 boost::weak_ptr<Ekiga::PresenceCore> _presence_core,
                                                 boost::function0<std::list<std::string> > _existing_groups,
                                                 xmlNodePtr _node);

    ~Presentity ();

    /* methods of the general Ekiga::Presentity class: */

    const std::string get_name () const;

    const std::string get_presence () const;

    const std::string get_status () const;

    const std::list<std::string> get_groups () const;

    const std::string get_uri () const;

    bool has_uri (const std::string uri) const;

    /* setter methods specific for this class of presentity, where we
     * expect to get presence from elsewhere:
     */

    void set_presence (const std::string presence_);

    void set_status (const std::string status_);

    // method to rename a group for this presentity
    void rename_group (const std::string old_name,
                       const std::string new_name);

    /* Those allow the parent Opal::Heap to manage this presentity
     * effectively.
     *
     * First, the 'trigger_saving' signal is to let the heap know that
     * the live XML document was modified and hence should be saved.
     *
     * The 'remove' method asks the presentity to remove all its data
     * from the live XML document, emit 'trigger_saving' so those
     * changes get written in the XML source document, and finally
     * emit 'removed' so the views (and the heap) forget about it as a
     * presentity.
     *
     */
    boost::signals2::signal<void(void)> trigger_saving;
    void remove ();

  private:
    Presentity (Account & account,
                boost::weak_ptr<Ekiga::PresenceCore> presence_core_,
                boost::function0<std::list<std::string> > existing_groups_,
                xmlNodePtr node_);

    void add_actions ();

    /* this pair of method is to let the user edit the presentity with
     * a nice form
     */
    void edit_presentity ();
    bool edit_presentity_form_submitted (bool submitted,
                                         Ekiga::Form& result,
                                         std::string& error);

    Account & account;
    boost::weak_ptr<Ekiga::PresenceCore> presence_core;
    boost::function0<std::list<std::string> > existing_groups;
    xmlNodePtr node;

    std::string presence;
    std::string status;
  };

  typedef boost::shared_ptr<Presentity> PresentityPtr;

};

#endif
