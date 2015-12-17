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
 *                         contact-core.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : interface of the main contact managing object
 *
 */

#ifndef __CONTACT_CORE_H__
#define __CONTACT_CORE_H__

#include "services.h"
#include "source.h"
#include "scoped-connections.h"
#include "action-provider.h"
#include "chain-of-responsibility.h"
#include "form-request.h"

/* declaration of a few helper classes */
namespace Ekiga
{

/**
 * @defgroup contacts Address Book
 * @{
 */


  /** Core object for address book support.
   *
   * Notice that you give sources to this object as references, so they won't
   * be freed here : it's up to you to free them somehow.
   */
  class ContactCore:
    public URIActionProviderStore,
    public Service
  {
  public:

    /** The constructor.
     */
    ContactCore ()
    {}

    ~ContactCore ();

    /*** Service Implementation ***/

    /** Returns the name of the service.
     * @return The service name.
     */
    const std::string get_name () const
    { return "contact-core"; }


    /** Returns the description of the service.
     * @return The service description.
     */
    const std::string get_description () const
    { return "\tContact managing object"; }


    /*** Public API ***/

    /** Adds a source to the ContactCore service.
     * @param The source to be added.
     */
    void add_source (SourcePtr source);


    /** Triggers a callback for all Ekiga::Source sources of the
     * ContactCore service.
     * @param The callback (the return value means "go on" and allows
     *  stopping the visit)
     */
    void visit_sources (boost::function1<bool, SourcePtr > visitor) const;


    /** This signal is emitted when a Ekiga::Source has been
     * added to the ContactCore Service.
     */
    boost::signals2::signal<void(SourcePtr)> source_added;


    /** This chain allows the core to present forms to the user
     */
    ChainOfResponsibility<FormRequestPtr> questions;

  private:

    std::list<SourcePtr > sources;
    Ekiga::scoped_connections conns;
  };

/**
 * @}
 */

};
#endif
