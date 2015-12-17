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
 *                         presence-core.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *                          (c) 2008 by Damien Sandras
 *   description          : implementation of the main
 *                          presentity managing object
 *
 */

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include "presence-core.h"
#include "personal-details.h"


Ekiga::PresenceCore::PresenceCore (boost::shared_ptr<Ekiga::PersonalDetails> _details): details(_details)
{
  conns.add (details->updated.connect(boost::bind (&Ekiga::PresenceCore::publish, this)));
}

void
Ekiga::PresenceCore::add_cluster (ClusterPtr cluster)
{
  clusters.insert (cluster);
  cluster_added (cluster);
  cluster->questions.connect (boost::ref (questions));
}

void
Ekiga::PresenceCore::remove_cluster (ClusterPtr cluster)
{
  cluster_removed (cluster);
  clusters.erase (cluster);
}

void
Ekiga::PresenceCore::visit_clusters (boost::function1<bool, ClusterPtr > visitor) const
{
  bool go_on = true;
  for (std::set<ClusterPtr >::const_iterator iter = clusters.begin ();
       iter != clusters.end () && go_on;
       iter++)
    go_on = visitor (*iter);
}

void
Ekiga::PresenceCore::add_presence_fetcher (boost::shared_ptr<PresenceFetcher> fetcher)
{
  presence_fetchers.push_back (fetcher);
  conns.add (fetcher->presence_received.connect (boost::bind (&Ekiga::PresenceCore::on_presence_received, this, _1, _2)));
  conns.add (fetcher->status_received.connect (boost::bind (&Ekiga::PresenceCore::on_status_received, this, _1, _2)));
  for (std::map<std::string, uri_info>::const_iterator iter
         = uri_infos.begin ();
       iter != uri_infos.end ();
       ++iter)
    fetcher->fetch (iter->first);
}

void
Ekiga::PresenceCore::remove_presence_fetcher (boost::shared_ptr<PresenceFetcher> fetcher)
{
  presence_fetchers.remove (fetcher);
}

void
Ekiga::PresenceCore::fetch_presence (const std::string uri)
{
  uri_infos[uri].count++;

  if (uri_infos[uri].count == 1) {

    for (std::list<boost::shared_ptr<PresenceFetcher> >::iterator iter
           = presence_fetchers.begin ();
         iter != presence_fetchers.end ();
         ++iter)
      (*iter)->fetch (uri);
  }

  presence_received (uri, uri_infos[uri].presence);
  status_received (uri, uri_infos[uri].status);
}

void Ekiga::PresenceCore::unfetch_presence (const std::string uri)
{
  uri_infos[uri].count--;

  if (uri_infos[uri].count <= 0) {

    uri_infos.erase (uri_infos.find (uri));

    for (std::list<boost::shared_ptr<PresenceFetcher> >::iterator iter
           = presence_fetchers.begin ();
         iter != presence_fetchers.end ();
         ++iter)
      (*iter)->unfetch (uri);
  }
}

bool Ekiga::PresenceCore::is_supported_uri (const std::string & uri)
{
  for (std::list<boost::shared_ptr<PresenceFetcher> >::iterator iter
       = presence_fetchers.begin ();
       iter != presence_fetchers.end ();
       ++iter)
    if ((*iter)->is_supported_uri (uri))
      return true;

  return false;
}

void
Ekiga::PresenceCore::on_presence_received (const std::string uri,
                                           const std::string presence)
{
  uri_infos[uri].presence = presence;
  presence_received (uri, presence);
}

void
Ekiga::PresenceCore::on_status_received (const std::string uri,
                                         const std::string status)
{
  uri_infos[uri].status = status;
  status_received (uri, status);
}

void
Ekiga::PresenceCore::add_presence_publisher (boost::shared_ptr<PresencePublisher> publisher)
{
  presence_publishers.push_back (publisher);
}

void
Ekiga::PresenceCore::remove_presence_publisher (boost::shared_ptr<PresencePublisher> publisher)
{
  presence_publishers.remove (publisher);
}

void
Ekiga::PresenceCore::publish ()
{
  for (std::list<boost::shared_ptr<PresencePublisher> >::iterator iter
         = presence_publishers.begin ();
       iter != presence_publishers.end ();
       ++iter)
    (*iter)->publish (*details);
}
