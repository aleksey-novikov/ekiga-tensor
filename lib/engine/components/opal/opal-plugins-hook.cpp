/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2010 Damien Sandras <dsandras@seconix.com>
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
 *                         opal-plugins-hook.cpp  -  description
 *                         --------------------------------
 *   begin                : Sun Sept 26 2010
 *   copyright            : (C) 2010 by Julien Puydt
 *   description          : Code to connect the various ekiga plugins to opal
 *
 */

#include "opal-audio.h"

#include "opal-plugins-hook.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// we first declare the three plugin service descriptor classes
///////////////////////////////////////////////////////////////////////////////////////////////////////

class PSoundChannel_EKIGA_PluginDeviceDescriptor : public PPluginDeviceDescriptor
{
public:

  PSoundChannel_EKIGA_PluginDeviceDescriptor ()
  {
    // FIXME: this is due to opal's PFactory design
    PAssertAlways("This constructor is not to be called");
  }

  PSoundChannel_EKIGA_PluginDeviceDescriptor (Ekiga::ServiceCore& core):
    audioinput_core(core.get<Ekiga::AudioInputCore> ("audioinput-core")),
    audiooutput_core(core.get<Ekiga::AudioOutputCore> ("audiooutput-core"))
  {}

  const char* GetServiceType () const
  { return "PSoundChannel"; }

  const char* GetServiceName () const
  { return "EKIGA"; }

  virtual PObject* CreateInstance (P_INT_PTR) const
  {
    // FIXME: if that happens in a thread, that's bad...
    boost::shared_ptr<Ekiga::AudioInputCore> input = audioinput_core.lock ();
    boost::shared_ptr<Ekiga::AudioOutputCore> output = audiooutput_core.lock ();
    if (input && output)
      return new PSoundChannel_EKIGA (input, output);
    else
      return NULL;
  }

  virtual PStringArray GetDeviceNames (P_INT_PTR) const
  { return PStringList ("EKIGA"); }

  virtual bool ValidateDeviceName (const PString & deviceName,
				   P_INT_PTR) const
  { return deviceName.Find ("EKIGA") == 0; }

private:
  boost::weak_ptr<Ekiga::AudioInputCore> audioinput_core;
  boost::weak_ptr<Ekiga::AudioOutputCore> audiooutput_core;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////
// then we will need workers:
///////////////////////////////////////////////////////////////////////////////////////////////////////


struct opal_audio_worker:
  public PPluginFactory::Worker<PSoundChannel_EKIGA_PluginDeviceDescriptor>
{
  opal_audio_worker (Ekiga::ServiceCore& core_):
    PPluginFactory::Worker<PSoundChannel_EKIGA_PluginDeviceDescriptor> ("PSoundChannelEKIGA"),
    core(core_)
  {}

  PPluginDeviceDescriptor* Create (PPluginFactory::Param_T) const
  { return new PSoundChannel_EKIGA_PluginDeviceDescriptor (core); }

  Ekiga::ServiceCore& core;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////
// now, let's rock :
///////////////////////////////////////////////////////////////////////////////////////////////////////

struct opal_plugins_hook_workers
{
  opal_plugins_hook_workers (Ekiga::ServiceCore& core):
    audio(core)
  {}

  opal_audio_worker audio;
};

static boost::shared_ptr<opal_plugins_hook_workers> workers;

void
hook_ekiga_plugins_to_opal (Ekiga::ServiceCore& core)
{
  workers = boost::shared_ptr<opal_plugins_hook_workers> (new opal_plugins_hook_workers (core));

  PPluginManager::GetPluginManager().RegisterService ("PSoundChannelEKIGA");
}
