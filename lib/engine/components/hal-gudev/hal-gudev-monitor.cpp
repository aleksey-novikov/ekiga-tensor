/* Ekiga -- A VoIP and -Conferencing application
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
 *                         hal-gudev-monitor.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2014 by Julien Puydt
 *   copyright            : (c) 2014 Julien Puydt
 *   description          : implementation of the GUDev device monitor
 *
 */

#include "hal-gudev-monitor.h"
#include <algorithm>

#if DEBUG
static void
print_gudev_device (GUdevDevice* device)
{
  g_print ("GUdevDevice:\n");
  g_print ("\tname: %s\n", g_udev_device_get_name (device));
  g_print ("\tfile: %s\n", g_udev_device_get_device_file (device));

  g_print ("\tproperties:\n");
  const gchar* const *keys = g_udev_device_get_property_keys (device);
  for (int num = 0;
       keys[num] != NULL;
       num++) {

    const gchar* key = keys[num];
    const gchar* val = g_udev_device_get_property (device, key);
    g_print ("\t\t%s -> %s\n", key, val);
  }
}
#endif


void
gudev_monitor_uevent_handler (G_GNUC_UNUSED GUdevClient* client,
                              const gchar* action,
                              GUdevDevice* device,
                              GUDevMonitor* monitor)
{
  if (g_str_equal (action, "add") || g_str_equal (action, "remove"))
    monitor->device_change (device, action);
}


GUDevMonitor::GUDevMonitor (boost::shared_ptr<Ekiga::AudioInputCore> _audioinput_core,
                            boost::shared_ptr<Ekiga::AudioOutputCore> _audiooutput_core)
        : audioinput_core(_audioinput_core), audiooutput_core(_audiooutput_core)
{
  const gchar* subsystems[] = { "sound", NULL};
  client = g_udev_client_new (subsystems);

  _audioinput_core->get_devices (audio_input_devices);
  _audiooutput_core->get_devices (audio_output_devices);

  g_signal_connect (G_OBJECT (client), "uevent",
		    G_CALLBACK (gudev_monitor_uevent_handler), this);
}


GUDevMonitor::~GUDevMonitor ()
{
  g_object_unref (client);
}


void
GUDevMonitor::device_change (GUdevDevice* device,
                             const gchar* action)
{
#if DEBUG
  g_print ("%s\n", __PRETTY_FUNCTION__);
  print_gudev_device (device);
#endif
  gboolean add = g_str_equal (action, "add");
  gboolean remove = g_str_equal (action, "remove");

  const char* subsystem = g_udev_device_get_subsystem (device);
  if (g_str_equal (subsystem, "sound")) {
    // Audio device detected
    std::vector<std::string> new_audio_input_devices;
    std::vector<std::string> new_audio_output_devices;
    boost::shared_ptr<Ekiga::AudioInputCore> aicore = audioinput_core.lock ();
    boost::shared_ptr<Ekiga::AudioOutputCore> aocore = audiooutput_core.lock ();
    Ekiga::Device dev;
    if (!aicore || !aocore)
      return;

    aicore->get_devices (new_audio_input_devices);
    aocore->get_devices (new_audio_output_devices);

    std::vector<std::string>::iterator iter;
    // This is awful with an exploding O complexity. I'm ashamed.
    for (iter = new_audio_input_devices.begin ();
         iter != new_audio_input_devices.end ();
         ++iter) {
      std::vector<std::string>::iterator it = std::find (audio_input_devices.begin (), audio_input_devices.end (), (*iter));
      if (it == audio_input_devices.end () && !(*iter).empty ()) {
        if (add) {
          dev.SetFromString (*iter);
          audioinput_device_added (dev.source, dev.name);
        }
        break;
      }
      else
        audio_input_devices.erase (it);
    }
    if (remove && audio_input_devices.size () > 0) {
      dev.SetFromString (audio_input_devices[0]);
      audioinput_device_removed (dev.source, dev.name);
    }
    audio_input_devices = new_audio_input_devices;

    for (iter = new_audio_output_devices.begin ();
         iter != new_audio_output_devices.end ();
         ++iter) {
      std::vector<std::string>::iterator it = std::find (audio_output_devices.begin (), audio_output_devices.end (), (*iter));
      if (it == audio_output_devices.end () && !(*iter).empty ()) {
        if (add) {
          dev.SetFromString (*iter);
          audiooutput_device_added (dev.source, dev.name);
        }
        break;
      }
      else
        audio_output_devices.erase (it);
    }
    if (remove && audio_output_devices.size () > 0) {
      dev.SetFromString (audio_output_devices[0]);
      audiooutput_device_removed (dev.source, dev.name);
    }
    audio_output_devices = new_audio_output_devices;
  }
}
