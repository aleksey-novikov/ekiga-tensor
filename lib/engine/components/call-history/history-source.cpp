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
 *                         history-source.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Julien Puydt
 *   copyright            : (c) 2007 by Julien Puydt
 *   description          : implementation of the source for the call history
 *
 */

#include "config.h"

#include "history-source.h"

boost::shared_ptr<History::Source>
History::Source::create (Ekiga::ServiceCore &_core)
{
  boost::shared_ptr<History::Source> source = boost::shared_ptr<History::Source> (new History::Source (_core));
  source->load ();

  return source;
}


History::Source::Source (Ekiga::ServiceCore &_core): core(_core)
{
}

History::Source::~Source ()
{
}

const std::list<std::string>
History::Source::existing_groups () const
{
  return book->existing_groups ();
}

History::BookPtr
History::Source::get_book () const
{
  return book;
}

void
History::Source::load ()
{
  book = History::Book::create (core);

  add_book (book);
}

