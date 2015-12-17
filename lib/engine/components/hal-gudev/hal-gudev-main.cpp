/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2014 Damien Sandras <dsandras@seconix.com>
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
 *                         hal-gudev-main.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2014 by Julien Puydt
 *   copyright            : (c) 2014 Julien Puydt
 *   description          : implementation of the hook for the GUDev device monitor
 *
 */

#include "hal-gudev-main.h"
#include "hal-core.h"

#include "hal-gudev-monitor.h"

struct GUDevSpark: public Ekiga::Spark
{
  GUDevSpark (): result(false)
  {}

  bool try_initialize_more (Ekiga::ServiceCore& core,
			    int* /*argc*/,
			    char** /*argv*/[])
  {
    boost::shared_ptr<Ekiga::HalCore> hal_core = core.get<Ekiga::HalCore> ("hal-core");
    boost::shared_ptr<GUDevMonitor> monitor = core.get<GUDevMonitor> ("gudev");
    boost::shared_ptr<Ekiga::AudioInputCore> audioinput_core = core.get<Ekiga::AudioInputCore> ("audioinput-core");
    boost::shared_ptr<Ekiga::AudioOutputCore> audiooutput_core = core.get<Ekiga::AudioOutputCore> ("audiooutput-core");

    if (hal_core && !monitor) {

      monitor = boost::shared_ptr<GUDevMonitor> (new GUDevMonitor (audioinput_core, audiooutput_core));

      core.add (Ekiga::ServicePtr (monitor));
      hal_core->add_manager (*monitor);
      result = true;
    }

    return result;
  }

  Ekiga::Spark::state get_state () const
  { return result?FULL:BLANK; }

  const std::string get_name () const
  { return "GUDEV"; }

  bool result;
};

void
hal_gudev_init (Ekiga::KickStart& kickstart)
{
  boost::shared_ptr<Ekiga::Spark> spark(new GUDevSpark);
  kickstart.add_spark (spark);
}
