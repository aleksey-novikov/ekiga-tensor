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
 *                         audiooutput-core.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Matthias Schneider
 *   copyright            : (c) 2008 by Matthias Schneider
 *   description          : declaration of the interface of a audiooutput core.
 *                          An audiooutput core manages AudioOutputManagers.
 *
 */

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include <algorithm>
#include <math.h>

#include <glib/gi18n.h>
#include <boost/algorithm/string.hpp>

#include "audiooutput-core.h"
#include "audiooutput-manager.h"

#include "ekiga-settings.h"

using namespace Ekiga;

static void
sound_event_changed (G_GNUC_UNUSED GSettings* settings,
                     const gchar* key,
                     gpointer data)
{
  g_return_if_fail (data != NULL);

  AudioOutputCore* core = (AudioOutputCore*) (data);

  core->setup_sound_events (key);
}

static void
audio_device_changed (GSettings* settings,
                      G_GNUC_UNUSED const gchar* key,
                      gpointer data)
{
  g_return_if_fail (data != NULL);

  AudioOutputCore *core = (AudioOutputCore*) (data);
  gchar *schema_id = NULL;

  g_object_get (settings, "schema-id", &schema_id, NULL);

  g_return_if_fail (schema_id != NULL);

  if (!g_strcmp0 (schema_id, AUDIO_DEVICES_SCHEMA))
    core->setup_audio_device ();
  else if (!g_strcmp0 (schema_id, SOUND_EVENTS_SCHEMA))
    core->setup_audio_device (secondary);
}


AudioOutputCore::AudioOutputCore (Ekiga::ServiceCore& core)
{
  PWaitAndSignal m_pri(core_mutex[primary]);
  PWaitAndSignal m_sec(core_mutex[secondary]);
  PWaitAndSignal m_vol(volume_mutex);

  audio_event_scheduler = new AudioEventScheduler (*this);

  current_primary_config.active = false;
  current_primary_config.channels = 0;
  current_primary_config.samplerate = 0;
  current_primary_config.bits_per_sample = 0;
  current_primary_config.buffer_size = 0;
  current_primary_config.num_buffers = 0;

  current_primary_volume = 0;
  desired_primary_volume = 0;

  current_manager[primary] = NULL;
  current_manager[secondary] = NULL;
  average_level = 0;
  calculate_average = false;
  yield = false;

  notification_core = core.get<Ekiga::NotificationCore> ("notification-core");
  sound_events_settings = g_settings_new (SOUND_EVENTS_SCHEMA);
  audio_device_settings = g_settings_new (AUDIO_DEVICES_SCHEMA);
  audio_device_settings_signals[primary] = 0;
  audio_device_settings_signals[secondary] = 0;
}

AudioOutputCore::~AudioOutputCore ()
{
  PWaitAndSignal m_pri(core_mutex[primary]);
  PWaitAndSignal m_sec(core_mutex[secondary]);

  delete audio_event_scheduler;

  for (std::set<AudioOutputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter)
    delete (*iter);

  managers.clear();

  g_clear_object (&sound_events_settings);
  g_clear_object (&audio_device_settings);

#if DEBUG
  std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
#endif
}


void
AudioOutputCore::setup ()
{
  setup_audio_device (primary);
  setup_audio_device (secondary);
  setup_sound_events ();
}

void
AudioOutputCore::setup_audio_device (AudioOutputPS device_idx)
{
  PWaitAndSignal m_pri(core_mutex[primary]);
  PWaitAndSignal m_sec(core_mutex[secondary]);
  AudioOutputDevice device;

  std::vector<AudioOutputDevice> devices;
  AudioOutputDevice device_fallback (AUDIO_OUTPUT_FALLBACK_DEVICE_TYPE,
                                     AUDIO_OUTPUT_FALLBACK_DEVICE_SOURCE,
                                     AUDIO_OUTPUT_FALLBACK_DEVICE_NAME);
  AudioOutputDevice device_preferred1 (AUDIO_OUTPUT_PREFERRED_DEVICE_TYPE1,
                                       AUDIO_OUTPUT_PREFERRED_DEVICE_SOURCE1,
                                       AUDIO_OUTPUT_PREFERRED_DEVICE_NAME1);
  AudioOutputDevice device_preferred2 (AUDIO_OUTPUT_PREFERRED_DEVICE_TYPE2,
                                       AUDIO_OUTPUT_PREFERRED_DEVICE_SOURCE2,
                                       AUDIO_OUTPUT_PREFERRED_DEVICE_NAME2);
  bool found = false;
  bool found_preferred1 = false;
  bool found_preferred2 = false;

  gchar* audio_device = NULL;

 
  if (device_idx == primary)
    audio_device = g_settings_get_string (audio_device_settings, "output-device");
  else
    audio_device = g_settings_get_string (sound_events_settings, "output-device");

  get_devices (devices);

  if (audio_device != NULL) {

    for (std::vector<AudioOutputDevice>::iterator it = devices.begin ();
         it < devices.end ();
         ++it) {

      if ((*it).GetString () == audio_device) {

        found = true;
        break;
      }
      else if (*it == device_preferred1) {

        found_preferred1 = true;
      }
      else if (*it == device_preferred2) {

        found_preferred2 = true;
      }
    }
  }

  if (found)
    device.SetFromString (audio_device);
  else if (found_preferred1)
    device = device_preferred1;
  else if (found_preferred2)
    device = device_preferred2;
  else if (!devices.empty ())
    device = *devices.begin ();
  else
    device = device_fallback;

  if (!found)
    g_settings_set_string ((device_idx == primary)?audio_device_settings:sound_events_settings,
                           "output-device", device.GetString ().c_str ());
  else
    set_device (device_idx, device);

  if (audio_device_settings_signals[device_idx] == 0 && device_idx == primary) {

    audio_device_settings_signals[device_idx] =
      g_signal_connect (audio_device_settings, "changed::output-device",
                        G_CALLBACK (audio_device_changed), this);
  }
  else if (audio_device_settings_signals[device_idx] == 0 && device_idx == secondary) {

    audio_device_settings_signals[device_idx] =
      g_signal_connect (sound_events_settings, "changed::output-device",
                        G_CALLBACK (audio_device_changed), this);
  }

  g_free (audio_device);
  PTRACE(1, "AudioOutputCore\tSet " << (device_idx == primary ? "primary" : "secondary") << " audio device to " << device.name);
}


void
AudioOutputCore::setup_sound_events (std::string e)
{
  static const char* events [] =
    {
      "busy-tone-sound",
      "incoming-call-sound",
      "new-message-sound",
      "new-voicemail-sound",
      "ring-tone-sound"
    };

  gulong signal = 0;

  boost::replace_all (e, "enable-", "");

  for (int i = 0 ; i < 5 ; i++) {

    std::string event = events[i];

    if (e.empty () || e == event) {

      gchar *file_name = NULL;
      bool enabled;

      file_name = g_settings_get_string (sound_events_settings, event.c_str ());
      enabled = g_settings_get_boolean (sound_events_settings, ("enable-" + event).c_str ());
      if (file_name == NULL) {

        PTRACE(1, "AudioOutputCoreConfBridge\t" << event << " is NULL");
        return;
      }
      else
        PTRACE(1, "AudioOutputCoreConfBridge\t" << event << " set to " << file_name);

      map_event (event, file_name, primary, enabled);
      g_free (file_name);
    }
  }

  signal = g_signal_handler_find (G_OBJECT (sound_events_settings),
                                  G_SIGNAL_MATCH_FUNC,
                                  0, 0, NULL,
                                  (gpointer) sound_event_changed,
                                  NULL);
  /* Connect all signals at once if no handler is found */
  if (signal == 0) {

    for (int i = 0 ; i < 5 ; i++) {

      std::string event = events[i];
      g_signal_connect (sound_events_settings, ("changed::" + event).c_str (),
                        G_CALLBACK (sound_event_changed), this);
      g_signal_connect (sound_events_settings, ("changed::enable-" + event).c_str (),
                        G_CALLBACK (sound_event_changed), this);
    }
  }
}

void
AudioOutputCore::add_manager (AudioOutputManager& manager)
{
  managers.insert (&manager);
  manager_added (manager);

  manager.device_error.connect (boost::bind (boost::ref(device_error), boost::ref(manager), _1, _2, _3));
  manager.device_opened.connect (boost::bind (boost::ref(device_opened), boost::ref(manager), _1, _2, _3));
  manager.device_closed.connect (boost::bind (boost::ref(device_closed), boost::ref(manager), _1, _2));
}

void
AudioOutputCore::visit_managers (boost::function1<bool, AudioOutputManager&> visitor) const
{
  PWaitAndSignal m_pri(core_mutex[primary]);
  PWaitAndSignal m_sec(core_mutex[secondary]);
  bool go_on = true;

  for (std::set<AudioOutputManager *>::const_iterator iter = managers.begin ();
       iter != managers.end () && go_on;
       ++iter)
      go_on = visitor (*(*iter));
}

void
AudioOutputCore::map_event (const std::string& event_name,
                            const std::string& file_name,
                            AudioOutputPS ps,
                            bool enabled)
{
  audio_event_scheduler->set_file_name(event_name, file_name, ps, enabled);
}

void
AudioOutputCore::play_file (const std::string& file_name)
{
  audio_event_scheduler->add_event_to_queue(file_name, true, 0, 0);
}

void
AudioOutputCore::play_event (const std::string& event_name)
{
  audio_event_scheduler->add_event_to_queue(event_name, false, 0, 0);
}

void
AudioOutputCore::start_play_event (const std::string& event_name,
                                   unsigned interval,
                                   unsigned repetitions)
{
  audio_event_scheduler->add_event_to_queue(event_name, false, interval, repetitions);
}

void
AudioOutputCore::stop_play_event (const std::string& event_name)
{
  audio_event_scheduler->remove_event_from_queue(event_name);
}

void
AudioOutputCore::get_devices (std::vector<std::string>& devices)
{
  std::vector<AudioOutputDevice> d;

  get_devices (d);

  devices.clear ();

  for (std::vector<AudioOutputDevice>::iterator iter = d.begin ();
       iter != d.end ();
       ++iter)
    devices.push_back (iter->GetString ());
}

void
AudioOutputCore::get_devices (std::vector <AudioOutputDevice>& devices)
{
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);
  PWaitAndSignal m_sec(core_mutex[secondary]);

  devices.clear();

  for (std::set<AudioOutputManager*>::const_iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter)
    (*iter)->get_devices (devices);

#if PTRACING
  for (std::vector<AudioOutputDevice>::const_iterator iter = devices.begin ();
       iter != devices.end ();
       ++iter) {
    PTRACE(4, "AudioOutputCore\tDetected device: " << *iter);
  }
#endif

}

void
AudioOutputCore::set_device(AudioOutputPS ps,
                            const AudioOutputDevice& device)
{
  PTRACE(4, "AudioOutputCore\tSetting device[" << ps << "]: " << device);
  yield = true;
  PWaitAndSignal m_sec(core_mutex[secondary]);

  switch (ps) {

    case primary:
      yield = true;
      core_mutex[primary].Wait();
      internal_set_primary_device (device);
      core_mutex[primary].Signal();

      break;

    case secondary:
        if (device == current_device[primary]) {

          current_manager[secondary] = NULL;
          current_device[secondary].type = "";
          current_device[secondary].source = "";
          current_device[secondary].name = "";
        }
        else
          internal_set_manager (secondary, device);

        break;

    default:
      break;
  }
}

void
AudioOutputCore::add_device (const std::string& sink,
                             const std::string& device_name,
                             HalManager* /*manager*/)
{
  PTRACE(4, "AudioOutputCore\tAdding device " << device_name);
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);

  AudioOutputDevice device;
  for (std::set<AudioOutputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

     if ((*iter)->has_device (sink, device_name, device)) {

       device_added(device);

       boost::shared_ptr<Ekiga::Notification> notif (new Ekiga::Notification (Ekiga::Notification::Info,
									      _("New Audio Output Device"),
									      device.GetString (),
									      _("Use It"),
									      boost::bind (&AudioOutputCore::on_set_device, (AudioOutputCore*) this, device)));
       notification_core->push_notification (notif);
     }
  }
}

void
AudioOutputCore::remove_device (const std::string& sink,
                                const std::string& device_name,
                                HalManager* /*manager*/)
{
  PTRACE(4, "AudioOutputCore\tRemoving device " << device_name);
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);

  AudioOutputDevice device;

  for (std::set<AudioOutputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

     if ((*iter)->has_device (sink, device_name, device)) {

       if ( (device == current_device[primary]) && (current_primary_config.active) ) {

         AudioOutputDevice new_device;
         new_device.type   = AUDIO_OUTPUT_FALLBACK_DEVICE_TYPE;
         new_device.source = AUDIO_OUTPUT_FALLBACK_DEVICE_SOURCE;
         new_device.name   = AUDIO_OUTPUT_FALLBACK_DEVICE_NAME;
         internal_set_primary_device(new_device);
       }

       device_removed(device, device == current_device[primary]);
     }
  }
}

void
AudioOutputCore::start (unsigned channels,
                        unsigned samplerate,
                        unsigned bits_per_sample)
{
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);

  if (current_primary_config.active) {

    PTRACE(1, "AudioOutputCore\tTrying to start output device although already started");
    return;
  }


  average_level = 0;
  internal_open(primary, channels, samplerate, bits_per_sample);
  current_primary_config.active = true;
  current_primary_config.channels = channels;
  current_primary_config.samplerate = samplerate;
  current_primary_config.bits_per_sample = bits_per_sample;
  current_primary_config.buffer_size = 0;
  current_primary_config.num_buffers = 0;
}

void
AudioOutputCore::stop()
{
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);

  average_level = 0;
  internal_close(primary);

  current_primary_config.active = false;
}

void
AudioOutputCore::set_buffer_size (unsigned buffer_size,
                                  unsigned num_buffers)
{
  yield = true;
  PWaitAndSignal m_pri(core_mutex[primary]);

  if (current_manager[primary])
    current_manager[primary]->set_buffer_size (primary, buffer_size, num_buffers);

  current_primary_config.buffer_size = buffer_size;
  current_primary_config.num_buffers = num_buffers;
}

void
AudioOutputCore::set_frame_data (const char* data,
                                 unsigned size,
                                 unsigned& bytes_written)
{
  if (yield) {

    yield = false;
    g_usleep (5 * G_TIME_SPAN_MILLISECOND);
  }
  PWaitAndSignal m_pri(core_mutex[primary]);

  if (current_manager[primary]) {

    if (!current_manager[primary]->set_frame_data(primary, data, size, bytes_written)) {

      internal_close(primary);
      internal_set_primary_fallback();
      internal_open(primary, current_primary_config.channels, current_primary_config.samplerate, current_primary_config.bits_per_sample);
      if (current_manager[primary])
        current_manager[primary]->set_frame_data(primary, data, size, bytes_written); // the default device must always return true
    }

    PWaitAndSignal m_vol(volume_mutex);
    if (desired_primary_volume != current_primary_volume) {

      current_manager[primary]->set_volume(primary, desired_primary_volume);
      current_primary_volume = desired_primary_volume;
    }
  }

  if (calculate_average)
    calculate_average_level((const short*) data, bytes_written);
}

void
AudioOutputCore::set_volume (AudioOutputPS ps,
                             unsigned volume)
{
  PWaitAndSignal m_vol(volume_mutex);

  if (ps == primary)
    desired_primary_volume = volume;
}

void
AudioOutputCore::play_buffer(AudioOutputPS ps,
                             const char* buffer,
                             unsigned long len,
                             unsigned channels,
                             unsigned sample_rate,
                             unsigned bps)
{
  switch (ps) {

    case primary:
      core_mutex[primary].Wait();

      if (!current_manager[primary]) {

        PTRACE(1, "AudioOutputCore\tDropping sound event, primary manager not set");
        core_mutex[primary].Signal();
        return;
      }

      if (current_primary_config.active) {

        PTRACE(1, "AudioOutputCore\tDropping sound event, primary device not set");
        core_mutex[primary].Signal();
        return;
      }
      internal_play(primary, buffer, len, channels, sample_rate, bps);
      core_mutex[primary].Signal();

      break;

    case secondary:
        core_mutex[secondary].Wait();

        if (current_manager[secondary]) {

          internal_play(secondary, buffer, len, channels, sample_rate, bps);
          core_mutex[secondary].Signal();
        } else {
          core_mutex[secondary].Signal();
          PTRACE(1, "AudioOutputCore\tNo secondary audiooutput device defined, trying primary");
          play_buffer(primary, buffer, len, channels, sample_rate, bps);
        }

      break;

    default:
      break;
  }
}

void
AudioOutputCore::on_set_device (const AudioOutputDevice& device)
{
  g_settings_set_string (audio_device_settings, "output-device", device.GetString ().c_str ());
}

void
AudioOutputCore::internal_set_primary_device(const AudioOutputDevice& device)
{
  if (current_primary_config.active)
     internal_close(primary);

  if (device == current_device[secondary]) {

    current_manager[secondary] = NULL;
    current_device[secondary].type = "";
    current_device[secondary].source = "";
    current_device[secondary].name = "";
  }

  internal_set_manager(primary, device);

  if (current_primary_config.active)
    internal_open(primary, current_primary_config.channels, current_primary_config.samplerate, current_primary_config.bits_per_sample);

  if ((current_primary_config.buffer_size > 0) && (current_primary_config.num_buffers > 0 ) ) {

    if (current_manager[primary])
      current_manager[primary]->set_buffer_size (primary, current_primary_config.buffer_size, current_primary_config.num_buffers);
  }
}

void
AudioOutputCore::internal_set_manager (AudioOutputPS ps,
                                       const AudioOutputDevice& device)
{
  current_manager[ps] = NULL;
  for (std::set<AudioOutputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

     if ((*iter)->set_device (ps, device))
       current_manager[ps] = (*iter);
  }

  if (current_manager[ps]) {

    current_device[ps]  = device;
  } else {

    if (ps == primary) {

      PTRACE(1, "AudioOutputCore\tTried to set unexisting primary device " << device);
      internal_set_primary_fallback();
    } else {

      PTRACE(1, "AudioOutputCore\tTried to set unexisting secondary device " << device);
      current_device[secondary].type = "";
      current_device[secondary].source = "";
      current_device[secondary].name = "";
    }
  }

}

void
AudioOutputCore::internal_set_primary_fallback ()
{
  current_device[primary].type   = AUDIO_OUTPUT_FALLBACK_DEVICE_TYPE;
  current_device[primary].source = AUDIO_OUTPUT_FALLBACK_DEVICE_SOURCE;
  current_device[primary].name   = AUDIO_OUTPUT_FALLBACK_DEVICE_NAME;
  PTRACE(1, "AudioOutputCore\tFalling back to " << current_device[primary]);
  internal_set_manager(primary, current_device[primary]);
}

bool
AudioOutputCore::internal_open (AudioOutputPS ps,
                                unsigned channels,
                                unsigned samplerate,
                                unsigned bits_per_sample)
{
  PTRACE(4, "AudioOutputCore\tOpening device["<<ps<<"] with " << channels<< "-" << samplerate << "/" << bits_per_sample);

  if (!current_manager[ps]) {

    PTRACE(1, "AudioOutputCore\tUnable to obtain current manager for device["<<ps<<"]");
    return false;
  }

  if (!current_manager[ps]->open(ps, channels, samplerate, bits_per_sample)) {

    PTRACE(1, "AudioOutputCore\tUnable to open device["<<ps<<"]");
    if (ps == primary) {

      internal_set_primary_fallback();
      if (current_manager[primary])
        current_manager[primary]->open(ps, channels, samplerate, bits_per_sample);

      return true;
    } else {

      return false;
    }
  }

  return true;
}

void
AudioOutputCore::internal_close (AudioOutputPS ps)
{
  PTRACE(4, "AudioOutputCore\tClosing current device");
  if (current_manager[ps])
    current_manager[ps]->close(ps);
}

void
AudioOutputCore::internal_play(AudioOutputPS ps,
                               const char* buffer,
                               unsigned long len,
                               unsigned channels,
                               unsigned sample_rate,
                               unsigned bps)
{
  unsigned long pos = 0;
  unsigned bytes_written = 0;
  unsigned buffer_size = (unsigned)((float)sample_rate/25);

  if (!internal_open ( ps, channels, sample_rate, bps))
    return;

  if (current_manager[ps]) {

    current_manager[ps]->set_buffer_size (ps, buffer_size, 4);
    do {

      if (!current_manager[ps]->set_frame_data(ps, buffer+pos, std::min(buffer_size, (unsigned) (len - pos)), bytes_written))
        break;
      pos += buffer_size;
    } while (pos < len);
  }

  internal_close( ps);
}

void
AudioOutputCore::calculate_average_level (const short*buffer,
                                          unsigned size)
{
  int sum = 0;
  unsigned csize = 0;

  while (csize < (size>>1) ) {

    if (*buffer < 0)
      sum -= *buffer++;
    else
      sum += *buffer++;

    csize++;
  }

  average_level = log10 (9.0*sum/size/32767+1)*1.0;
}
