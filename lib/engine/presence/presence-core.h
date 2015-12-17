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
 *                         presence-core.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *                          (c) 2008 by Damien Sandras
 *   description          : declaration of the main
 *                          presentity managing object
 *
 */

#ifndef __PRESENCE_CORE_H__
#define __PRESENCE_CORE_H__

#include "services.h"
#include "scoped-connections.h"
#include "cluster.h"
#include "personal-details.h"
#include "action-provider.h"

namespace Ekiga
{

/**
 * @defgroup presence Presence
 * @{
 */
  class PresenceFetcher
  {
  public:

    /** Triggers presence fetching for the given uri
     * (notice: the PresenceFetcher should count how many times it was
     * requested presence for an uri, in case several presentities share it)
     * @param The uri for which to fetch presence information.
     */
    virtual void fetch (const std::string /*uri*/) = 0;

    /** Stops presence fetching for the given uri
     * (notice that if some other presentity asked for presence information
     * on the same uri, the fetching should go on until the last of them is
     * gone)
     * @param The uri for which to stop fetching presence information.
     */
    virtual void unfetch (const std::string /*uri*/) = 0;

    /* Return true if URI can be handled by the PresenceFetcher,
     * false otherwise.
     * @param the URI to test
     * @return true of the URI can be handled, false otherwise
     */
    virtual bool is_supported_uri (const std::string & /*uri*/) = 0;

    /** Those signals are emitted whenever this presence fetcher gets
     * presence information about an uri it was required to handle.
     * The information is given as a pair of strings (uri, data).
     */
    boost::signals2::signal<void(std::string, std::string)> presence_received;
    boost::signals2::signal<void(std::string, std::string)> status_received;
  };

  class PresencePublisher
  {
  public:

    virtual ~PresencePublisher () {}

    virtual void publish (const PersonalDetails& details) = 0;
  };

  /** Core object for the presence support.
   *
   * The presence core has several goals:
   *  - one of them is of course to list presentities, and know what happens to
   *    them;
   *  - another one is that we may want to store presentities somewhere as dead
   *    data, but still be able to gain presence information and actions on
   *    them.
   *
   * This is achieved by using two types of helpers:
   * - the abstract class PresenceFetcher, through which it is possible to gain
   *   presence information: they allow the PresenceCore to declare some
   *   presence information is needed about an uri, or now unneeded;
   * - finally, a simple callback-based api allows to add detecters for
   *   supported uris: this allows for example a Presentity to know if it
   *   should declare an uri as "foo@bar" or as "prtcl:foo@bar".
   */

  /*
   * FIXME : couldn't a chain of responsibility be used there instead of a
   *         special registering magic?
   */
  class PresenceCore:
    public URIActionProviderStore,
    public Service
  {
  public:

    /** The constructor.
     */
    PresenceCore (boost::shared_ptr<PersonalDetails> details);

    /*** Service Implementation ***/
  public:
    /** Returns the name of the service.
     * @return The service name.
     */
    const std::string get_name () const
    { return "presence-core"; }

    /** Returns the description of the service.
     * @return The service description.
     */
    const std::string get_description () const
    { return "\tPresence managing object"; }

    /*** API to list presentities ***/
  public:

    /** Adds a cluster to the PresenceCore service.
     * @param The cluster to be added.
     */
    void add_cluster (ClusterPtr cluster);

    /** Removes a cluster from the PresenceCore service.
     * @param The cluster to be removed.
     */
    void remove_cluster (ClusterPtr cluster);

    /** Triggers a callback for all Ekiga::Cluster clusters of the
     * PresenceCore service.
     * @param The callback (the return value means "go on" and allows
     *  stopping the visit)
     */
    void visit_clusters (boost::function1<bool, ClusterPtr > visitor) const;

    /** This signal is emitted when an Ekiga::Cluster has been added
     * to the PresenceCore Service.
     */
    boost::signals2::signal<void(ClusterPtr)> cluster_added;

    /** This signal is emitted when an Ekiga::Cluster has been removed
     * to the PresenceCore Service.
     */
    boost::signals2::signal<void(ClusterPtr)> cluster_removed;

  private:
    std::set<ClusterPtr > clusters;

    /*** API to help presentities get presence ***/
  public:

    /** Adds a fetcher to the pool of presentce fetchers.
     * @param The presence fetcher.
     */
    void add_presence_fetcher (boost::shared_ptr<PresenceFetcher> fetcher);

    /** Removes a fetcher from the pool of presentce fetchers.
     * @param The presence fetcher.
     */
    void remove_presence_fetcher (boost::shared_ptr<PresenceFetcher> fetcher);

    /** Tells the PresenceCore that someone is interested in presence
     * information for the given uri.
     * @param: The uri for which presence is requested.
     */
    void fetch_presence (const std::string uri);

    /** Tells the PresenceCore that someone becomes uninterested in presence
     * information for the given uri.
     * @param: The uri for which presence isn't requested anymore.
     */
    void unfetch_presence (const std::string uri);

    /* Return true if URI can be handled by the PresenceCore,
     * false otherwise.
     * @param the URI to test
     * @return true of the URI can be handled, false otherwise
     */
    bool is_supported_uri (const std::string & uri);

    /** Those signals are emitted whenever information has been received
     * about an uri ; the information is a pair of strings (uri, information).
     */
    boost::signals2::signal<void(std::string, std::string)> presence_received;
    boost::signals2::signal<void(std::string, std::string)> status_received;

    /** This chain allows the core to present forms to the user
     */
    ChainOfResponsibility<FormRequestPtr> questions;

  private:

    std::list<boost::shared_ptr<PresenceFetcher> > presence_fetchers;
    void on_presence_received (const std::string uri,
                               const std::string presence);
    void on_status_received (const std::string uri,
                             const std::string status);
    struct uri_info
    {
      uri_info (): count(0), presence("unknown"), status("")
      { }

      int count;
      std::string presence;
      std::string status;
    };

    std::map<std::string, uri_info> uri_infos;

    /* help publishing presence */
  public:

    void add_presence_publisher (boost::shared_ptr<PresencePublisher> publisher);
    void remove_presence_publisher (boost::shared_ptr<PresencePublisher> publisher);

  private:

    std::list<boost::shared_ptr<PresencePublisher> > presence_publishers;
    void publish ();

  public:

    /*** Misc ***/
  private:

    boost::shared_ptr<PersonalDetails> details;
    Ekiga::scoped_connections conns;
  };

/**
 * @}
 */

};

#endif
