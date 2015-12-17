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
 *                         engine.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : Vroom.
 *
 */

#include "config.h"

#include "engine.h"

#include "services.h"
#include "kickstart.h"

#include "notification-core.h"
#include "plugin-core.h"
#include "presence-core.h"
#include "account-core.h"
#include "contact-core.h"
#include "call-core.h"
#include "friend-or-foe.h"
#include "foe-list.h"
#include "audioinput-core.h"
#include "audiooutput-core.h"
#include "hal-core.h"
#include "history-main.h"
#include "glib-notify-main.h"
#include "gtk-core-main.h"
#include "gmconf-personal-details.h"


#include "audioinput-main-null.h"
#include "audiooutput-main-null.h"

#include "audioinput-main-ptlib.h"
#include "audiooutput-main-ptlib.h"

#ifdef HAVE_GUDEV
#include "hal-gudev-main.h"
#endif

#include "opal-process.h"
#include "opal-main.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#include <iostream>
#endif

void
engine_init (Ekiga::ServiceCore& core,
             int argc,
             char *argv [])
{
  // AT THE VERY FIRST, create the PProcess
  GnomeMeeting & instance = opal_init_pprocess (argc, argv);

  // FIRST we add a few things by hand
  // (for speed and because that's less code)

  boost::shared_ptr<Ekiga::NotificationCore> notification_core(new Ekiga::NotificationCore);
  core.add (notification_core);

  boost::shared_ptr<Ekiga::FriendOrFoe> friend_or_foe (new Ekiga::FriendOrFoe);
  boost::shared_ptr<Ekiga::FoeList> foe_list (new Ekiga::FoeList (friend_or_foe));
  boost::shared_ptr<Ekiga::AccountCore> account_core (new Ekiga::AccountCore);
  boost::shared_ptr<Ekiga::ContactCore> contact_core (new Ekiga::ContactCore);
  boost::shared_ptr<Ekiga::CallCore> call_core (new Ekiga::CallCore (friend_or_foe, notification_core));
  boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core (new Ekiga::AudioOutputCore (core));
  boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core (new Ekiga::AudioInputCore(core));
  boost::shared_ptr<Ekiga::HalCore> hal_core (new Ekiga::HalCore);
  boost::shared_ptr<Gmconf::PersonalDetails> details(new Gmconf::PersonalDetails);
  boost::shared_ptr<Ekiga::PresenceCore> presence_core(new Ekiga::PresenceCore (details));

  core.add (contact_core);
  core.add (friend_or_foe);
  core.add (foe_list);
  core.add (audioinput_core);
  core.add (audiooutput_core);
  core.add (hal_core);
  core.add (call_core);
  core.add (account_core);
  core.add (details);
  core.add (presence_core);

  //
  instance.Start (core);

  // THEN we use the kickstart scheme

  Ekiga::KickStart kickstart;

  audioinput_null_init (kickstart);
  audiooutput_null_init (kickstart);

  audioinput_ptlib_init (kickstart);
  audiooutput_ptlib_init (kickstart);

  gnotify_init (kickstart);

#ifdef HAVE_GUDEV
  hal_gudev_init (kickstart);
#endif

  opal_init (kickstart);

  history_init (kickstart);

  plugin_init (kickstart);

  // FIXME: Some parts in the kickstart need the gui.  The gui needs
  //  some parts in the kickstart.  So we will kick a first time to
  //  get things not needing the gui up and running, then start the
  //  gui (which will hence find what it needs) and kick a second time
  //  to really make the engine go vroom. It would be nicer to either
  //  push the parts needed by the gui in the hand-crafted part of
  //  this initialization, or put the gui in the kickstart too.

  kickstart.kick (core, &argc, &argv);

  gtk_core_init (core, &argc, &argv);

  kickstart.kick (core, &argc, &argv);

  /* FIXME: everything that follows except the debug output shouldn't
     be there, as that means we're doing the work of initializing
     those in the correct order here instead of having the specific
     code in question to do it itself
   */

  audioinput_core->setup ();
  audiooutput_core->setup ();


  hal_core->audiooutput_device_added.connect (boost::bind (&Ekiga::AudioOutputCore::add_device, boost::ref (*audiooutput_core), _1, _2, _3));
  hal_core->audiooutput_device_removed.connect (boost::bind (&Ekiga::AudioOutputCore::remove_device, boost::ref (*audiooutput_core), _1, _2, _3));
  hal_core->audioinput_device_added.connect (boost::bind (&Ekiga::AudioInputCore::add_device, boost::ref (*audioinput_core), _1, _2, _3));
  hal_core->audioinput_device_removed.connect (boost::bind (&Ekiga::AudioInputCore::remove_device, boost::ref (*audioinput_core), _1, _2, _3));

  /* FIXME: does it really belong here? */
  friend_or_foe->add_helper (foe_list);

#if DEBUG_STARTUP
  std::cout << "Here is what ekiga is made of for this run :" << std::endl;
  core.dump (std::cout);
#endif
}


void engine_close (Ekiga::ServiceCore& core)
{
  opal_close (core);
}
