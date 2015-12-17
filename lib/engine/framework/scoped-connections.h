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
 *                         scoped-connections.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2013 by Julien Puydt
 *   copyright            : (c) 2013 by Julien Puydt
 *   description          : an helper class to store list of boost connections
 *                          which will get auto-released
 *
 */

#ifndef __SCOPED_CONNECTIONS_H__
#define __SCOPED_CONNECTIONS_H__

#include <list>
#include <boost/signals2.hpp>

/* The boost signals2 library has several tricks to disconnect connections on signals
 * automatically, namely :
 * - inherit from boost::signals2::trackable, which is good to get rid of
 * connnections to a dying object (but that basically means you are lazy
 * so it's probably better to avoid that method, as that means we might
 * end up with memory problems later on) ;
 * - use a boost::signals2::scoped_connection which makes it possible to do things
 * more manually, but is annoying when you need many of them because they're not
 * easy to put in a container ;
 *
 * This file provides a scoped_connections class, in which you can put your
 * connections, and which will disconnect them either when it dies or if you
 * asked it to clear. That makes it possible to listen to signals on an object,
 * then clear, then listen to signals on another object. Or it just makes it easy
 * to manage a big number of connections.
 */

namespace Ekiga {

  class scoped_connections:
    public boost::noncopyable
  {
  public:

    ~scoped_connections ()
      { clear (); }

    void add (boost::signals2::connection conn)
    { conns.push_front (conn); }

    void clear ()
    {
      for (std::list<boost::signals2::connection>::iterator iter = conns.begin ();
	   iter != conns.end ();
	   ++iter)
	iter->disconnect ();
      conns.clear ();
    }

  private:

    std::list<boost::signals2::connection> conns;
  };
};

#endif
