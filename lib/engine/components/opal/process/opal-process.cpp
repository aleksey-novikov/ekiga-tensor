/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2009 Damien Sandras <dsandras@seconix.com>
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
 *                         gnomemeeting.cpp  -  description
 *                         --------------------------------
 *   begin                : Sat Dec 23 2000
 *   copyright            : (C) 2000-2006 by Damien Sandras
 *   description          : This file contains the main class
 *
 */

#include "config.h"

#include "opal-process.h"

#include <gtk/gtk.h>

#include "call-core.h"

#include "opal-plugins-hook.h"

GnomeMeeting *GnomeMeeting::GM = 0;

/* The main GnomeMeeting Class  */
GnomeMeeting::GnomeMeeting () : PProcess(PACKAGE_NAME, PACKAGE_NAME, MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
  GM = this;
}


GnomeMeeting::~GnomeMeeting ()
{
}


GnomeMeeting *
GnomeMeeting::Process ()
{
  return GM;
}


void GnomeMeeting::Main ()
{
}


void GnomeMeeting::Start (Ekiga::ServiceCore& core)
{
  hook_ekiga_plugins_to_opal (core);
  endpoint = new Opal::EndPoint (core);
}


void GnomeMeeting::Exit ()
{
  // Shutting down endpoints and cleaning up factories
  // are executed by Opal when the process is being
  // destroyed. However, it triggers segfaults when
  // using static PMutex instances in OpalGloballyUniqueID
  // for example.
  //
  // That is why we clean things up preventively.

  // Destroy the manager & shutdown all endpoints
  delete endpoint;

  // Clean up factories
  PProcessStartupFactory::KeyList_T list = PProcessStartupFactory::GetKeyList();
  for (PProcessStartupFactory::KeyList_T::const_iterator it = list.begin(); it != list.end(); ++it)
    PProcessStartupFactory::CreateInstance(*it)->OnShutdown();
}


Opal::EndPoint&
GnomeMeeting::GetEndPoint ()
{
  return *endpoint;
}
