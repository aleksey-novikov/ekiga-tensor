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
 *                         cluster.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : declaration of the interface of a heap
 *                          implementation backend
 *
 */

#ifndef __CLUSTER_H__
#define __CLUSTER_H__

#include "heap.h"
#include "actor.h"

namespace Ekiga
{

/**
 * @addtogroup presence
 * @{
 */

  class Cluster: public virtual Actor
  {

  public:

    /** The destructor.
     */
    virtual ~Cluster () {}

    /** Get the list of Heaps by visiting them with a callback.
     * @param The callback used to know about heaps.
     */
    virtual void visit_heaps (boost::function1<bool, HeapPtr >) const = 0;


    /** This signal is emitted when a Heap has been added to the Source.
     */
    boost::signals2::signal<void(HeapPtr)> heap_added;


    /** This signal is emitted when a Heap has been updated in the Source.
     */
    boost::signals2::signal<void(HeapPtr)> heap_updated;


    /** This signal is emitted when a Heap has been removed in the Source.
     */
    boost::signals2::signal<void(HeapPtr)> heap_removed;
  };

  typedef boost::shared_ptr<Cluster> ClusterPtr;

/**
 * @}
 */

};

#endif
