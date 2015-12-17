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
 *                         dynamic-object.h  -  description
 *                         --------------------------------
 *   copyright            : (c) 2009 by Julien Puydt
 *                          (c) 2015 by Damien SANDRAS
 *   description          : common class for objects which are somehow 'dynamic'
 *
 */

#ifndef __DYNAMIC_OBJECT_H__
#define __DYNAMIC_OBJECT_H__

#include <boost/signals2.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif


namespace Ekiga
{
  template<typename ObjectType>
  class DynamicObject : public virtual boost::enable_shared_from_this<ObjectType>
    {
  public:

      /*
       * Classical constructors should not be used as we need to have
       * a shared_ptr to the object as soon as possible to be able to
       * use shared_from_this from the beginning.
       *
       * The following method should be implemeted in the child class
       * in order to create the object:
       *
       * static boost::shared_ptr<ChildObject> create ();
       *
       * The engine presents an explicit separation between:
       *  - The high-level API (e.g. Ekiga::Book)
       *  - The low-level implementation API (e.g. Ekiga::BookImpl<ContactType>)
       *
       */
#if DEBUG
      virtual ~DynamicObject ()
      {
        std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
      }
#endif

      /**
       * Signals on that object
       */

      /** This signal is emitted when the object has been updated.
       */
      boost::signals2::signal<void(boost::shared_ptr<ObjectType>)> updated;


      /** This signal is emitted when the object has been removed.
       */
      boost::signals2::signal<void(boost::shared_ptr<ObjectType>)> removed;
    };
};
#endif
