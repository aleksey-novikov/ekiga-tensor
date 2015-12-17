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
 *                         audioinput-core.cpp  -  description
 *                         ------------------------------------------
 *   begin                : written in 2008 by Matthias Schneider
 *   copyright            : (c) 2008 by Matthias Schneider
 *   description          : declaration of the interface of a audioinput core.
 *                          An audioinput core manages AudioInputManagers.
 *
 */

#if DEBUG
#include <typeinfo>
#include <iostream>
#endif

#include <math.h>

#include <glib/gi18n.h>

#include "config.h"

#include "ekiga-settings.h"

#include "audioinput-core.h"

using namespace Ekiga;

static void
audio_device_changed (G_GNUC_UNUSED GSettings* settings,
		      G_GNUC_UNUSED const gchar* key,
		      gpointer data)
{
  g_return_if_fail (data != NULL);

  AudioInputCore* core = (AudioInputCore*) (data);
  core->setup ();
}


AudioInputCore::AudioInputCore (Ekiga::ServiceCore& _core):
  core(_core)
{
  PWaitAndSignal m_var(core_mutex);
  PWaitAndSignal m_vol(volume_mutex);

  preview_config.active = false;
  preview_config.channels = 0;
  preview_config.samplerate = 0;
  preview_config.bits_per_sample = 0;
  preview_config.buffer_size = 0;
  preview_config.num_buffers = 0;

  stream_config.active = false;
  stream_config.channels = 0;
  stream_config.samplerate = 0;
  stream_config.bits_per_sample = 0;
  stream_config.buffer_size = 0;
  stream_config.num_buffers = 0;

  desired_volume = 0;
  current_volume = 0;

  current_manager = NULL;
  average_level = 0;
  calculate_average = false;
  yield = false;

  notification_core = core.get<Ekiga::NotificationCore> ("notification-core");
  audio_device_settings = g_settings_new (AUDIO_DEVICES_SCHEMA);
  audio_device_settings_signal = 0;
}

AudioInputCore::~AudioInputCore ()
{
  PWaitAndSignal m(core_mutex);

  for (std::set<AudioInputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter)
    delete (*iter);

  g_clear_object (&audio_device_settings);
  managers.clear();

#if DEBUG
  std::cout << "Destroyed object of type " << typeid(*this).name () << std::endl;
#endif
}

void
AudioInputCore::setup ()
{
  PWaitAndSignal m(core_mutex);
  gchar* audio_device = NULL;

  audio_device = g_settings_get_string (audio_device_settings, "input-device");

  set_device (audio_device);

  if (audio_device_settings_signal == 0) {

    audio_device_settings_signal =
      g_signal_connect (audio_device_settings, "changed::input-device",
                        G_CALLBACK (audio_device_changed), this);
  }

  g_free (audio_device);
}

void
AudioInputCore::add_manager (AudioInputManager& manager)
{
  managers.insert (&manager);
  manager_added (manager);

  manager.device_error.connect   (boost::bind (boost::ref(device_error), boost::ref(manager), _1, _2));
  manager.device_opened.connect  (boost::bind (boost::ref(device_opened), boost::ref(manager), _1, _2));
  manager.device_closed.connect  (boost::bind (boost::ref(device_closed), boost::ref(manager), _1));
}


void
AudioInputCore::visit_managers (boost::function1<bool, AudioInputManager&> visitor) const
{
  PWaitAndSignal m(core_mutex);
  bool go_on = true;

  for (std::set<AudioInputManager*>::const_iterator iter = managers.begin ();
       iter != managers.end () && go_on;
       ++iter)
      go_on = visitor (*(*iter));
}

void
AudioInputCore::get_devices (std::vector<std::string>& devices)
{
  std::vector<AudioInputDevice> d;
  get_devices (d);

  devices.clear ();

  for (std::vector<AudioInputDevice>::const_iterator iter = d.begin ();
       iter != d.end ();
       ++iter)
    devices.push_back (iter->GetString ());
}

void
AudioInputCore::get_devices (std::vector<AudioInputDevice>& devices)
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  devices.clear();

  for (std::set<AudioInputManager*>::const_iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter)
    (*iter)->get_devices (devices);

#if PTRACING
  for (std::vector<AudioInputDevice>::const_iterator iter = devices.begin ();
       iter != devices.end ();
       ++iter) {

    PTRACE(4, "AudioInputCore\tDetected Device: " << *iter);
  }
#endif

}

void
AudioInputCore::set_device (const std::string& device_string)
{
  PWaitAndSignal m(core_mutex);

  std::vector<AudioInputDevice> devices;
  AudioInputDevice device;
  AudioInputDevice device_fallback (AUDIO_INPUT_FALLBACK_DEVICE_TYPE,
                                    AUDIO_INPUT_FALLBACK_DEVICE_SOURCE,
                                    AUDIO_INPUT_FALLBACK_DEVICE_NAME);
  AudioInputDevice device_preferred1 (AUDIO_INPUT_PREFERRED_DEVICE_TYPE1,
                                      AUDIO_INPUT_PREFERRED_DEVICE_SOURCE1,
                                      AUDIO_INPUT_PREFERRED_DEVICE_NAME1);
  AudioInputDevice device_preferred2 (AUDIO_INPUT_PREFERRED_DEVICE_TYPE2,
                                      AUDIO_INPUT_PREFERRED_DEVICE_SOURCE2,
                                      AUDIO_INPUT_PREFERRED_DEVICE_NAME2);
  bool found = false;
  bool found_preferred1 = false;
  bool found_preferred2 = false;

  get_devices (devices);
  for (std::vector<AudioInputDevice>::const_iterator it = devices.begin ();
       it < devices.end ();
       ++it) {

    if ((*it).GetString () == device_string) {

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

  if (found)
    device.SetFromString (device_string);
  else if (found_preferred1)
    device = device_preferred1;
  else if (found_preferred2)
    device = device_preferred2;
  else if (!devices.empty ())
    device = *devices.begin ();
  else
    device = device_fallback;

  if (!found)
    g_settings_set_string (audio_device_settings, "input-device", device.GetString ().c_str ());
  else
    internal_set_device (device);

  PTRACE(4, "AudioInputCore\tSet device to " << device.source << "/" << device.name);
}

void
AudioInputCore::add_device (const std::string& source,
			    const std::string& device_name,
			    HalManager* /*manager*/)
{
  PTRACE(4, "AudioInputCore\tAdding device " << device_name);
  yield = true;
  PWaitAndSignal m(core_mutex);

  AudioInputDevice device;
  for (std::set<AudioInputManager*>::const_iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

    if ((*iter)->has_device (source, device_name, device)) {

      device_added (device);

      boost::shared_ptr<Ekiga::Notification> notif (new Ekiga::Notification (Ekiga::Notification::Info,
									     _("New Audio Input Device"),
									     device.GetString (), _("Use It"),
									     boost::bind (&AudioInputCore::on_set_device, (AudioInputCore*) this, device)));
      notification_core->push_notification (notif);
    }
  }
}

void
AudioInputCore::remove_device (const std::string& source,
			       const std::string& device_name,
			       HalManager* /*manager*/)
{
  PTRACE(4, "AudioInputCore\tRemoving device " << device_name);
  yield = true;
  PWaitAndSignal m(core_mutex);

  AudioInputDevice device;
  for (std::set<AudioInputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

     if ((*iter)->has_device (source, device_name, device)) {

       if ( ( current_device == device) && (preview_config.active || stream_config.active) ) {

            AudioInputDevice new_device;
            new_device.type = AUDIO_INPUT_FALLBACK_DEVICE_TYPE;
            new_device.source = AUDIO_INPUT_FALLBACK_DEVICE_SOURCE;
            new_device.name = AUDIO_INPUT_FALLBACK_DEVICE_NAME;
            internal_set_device( new_device);
       }

       device_removed (device,  current_device == device);
     }
  }
}

void
AudioInputCore::start_preview (unsigned channels,
			       unsigned samplerate,
			       unsigned bits_per_sample)
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  PTRACE(4, "AudioInputCore\tStarting preview " << channels << "x" << samplerate << "/" << bits_per_sample);

  if (preview_config.active || stream_config.active) {

    PTRACE(1, "AudioInputCore\tTrying to start preview in wrong state");
  }

  internal_open(channels, samplerate, bits_per_sample);

  preview_config.active = true;
  preview_config.channels = channels;
  preview_config.samplerate = samplerate;
  preview_config.bits_per_sample = bits_per_sample;
  preview_config.buffer_size = 320; //FIXME: verify
  preview_config.num_buffers = 5;

  if (current_manager)
    current_manager->set_buffer_size(preview_config.buffer_size, preview_config.num_buffers);

  average_level = 0;
}

void
AudioInputCore::stop_preview ()
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  PTRACE(4, "AudioInputCore\tStopping Preview");

  if (!preview_config.active || stream_config.active) {

    PTRACE(1, "AudioInputCore\tTrying to stop preview in wrong state");
  }

  internal_close();
  preview_config.active = false;
}


void
AudioInputCore::set_stream_buffer_size (unsigned buffer_size,
					unsigned num_buffers)
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  PTRACE(4, "AudioInputCore\tSetting stream buffer size " << num_buffers << "/" << buffer_size);

  if (current_manager)
    current_manager->set_buffer_size(buffer_size, num_buffers);

  stream_config.buffer_size = buffer_size;
  stream_config.num_buffers = num_buffers;
}

void
AudioInputCore::start_stream (unsigned channels,
			      unsigned samplerate,
			      unsigned bits_per_sample)
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  PTRACE(4, "AudioInputCore\tStarting stream " << channels << "x" << samplerate << "/" << bits_per_sample);

  if (preview_config.active || stream_config.active) {

    PTRACE(1, "AudioInputCore\tTrying to start stream in wrong state");
  }

  internal_open(channels, samplerate, bits_per_sample);

  stream_config.active = true;
  stream_config.channels = channels;
  stream_config.samplerate = samplerate;
  stream_config.bits_per_sample = bits_per_sample;

  average_level = 0;
}

void
AudioInputCore::stop_stream ()
{
  yield = true;
  PWaitAndSignal m(core_mutex);

  PTRACE(4, "AudioInputCore\tStopping Stream");

  if (preview_config.active || !stream_config.active) {

    PTRACE(1, "AudioInputCore\tTrying to stop stream in wrong state");
    return;
  }

  internal_close();
  stream_config.active = false;
  average_level = 0;
}

void
AudioInputCore::get_frame_data (char* data,
				unsigned size,
				unsigned& bytes_read)
{
  if (yield) {
    yield = false;
    g_usleep (5 * G_TIME_SPAN_MILLISECOND);
  }
  PWaitAndSignal m_var(core_mutex);

  if (current_manager) {

    if (!current_manager->get_frame_data(data, size, bytes_read)) {

      internal_close();
      internal_set_fallback();
      internal_open(stream_config.channels, stream_config.samplerate, stream_config.bits_per_sample);
      if (current_manager)
        current_manager->get_frame_data(data, size, bytes_read); // the default device must always return true
    }

    PWaitAndSignal m_vol(volume_mutex);
    if (desired_volume != current_volume) {

      current_manager->set_volume (desired_volume);
      current_volume = desired_volume;
    }
  }

  if (calculate_average)
    calculate_average_level((const short*) data, bytes_read);
}

void
AudioInputCore::set_volume (unsigned volume)
{
  PWaitAndSignal m(volume_mutex);

  desired_volume = volume;
}

void
AudioInputCore::on_set_device (const AudioInputDevice& device)
{
  g_settings_set_string (audio_device_settings, "input-device", device.GetString ().c_str ());
}

void
AudioInputCore::internal_set_device(const AudioInputDevice& device)
{
  PTRACE(4, "AudioInputCore\tSetting device: " << device);

  if (preview_config.active || stream_config.active)
    internal_close();

  internal_set_manager (device);

  if (preview_config.active) {

    internal_open(preview_config.channels, preview_config.samplerate, preview_config.bits_per_sample);

    if ((preview_config.buffer_size > 0) && (preview_config.num_buffers > 0 ) ) {

      if (current_manager)
        current_manager->set_buffer_size (preview_config.buffer_size, preview_config.num_buffers);
    }
  }

  if (stream_config.active) {

    internal_open(stream_config.channels, stream_config.samplerate, stream_config.bits_per_sample);

    if ((stream_config.buffer_size > 0) && (stream_config.num_buffers > 0 ) ) {

      if (current_manager)
        current_manager->set_buffer_size (stream_config.buffer_size, stream_config.num_buffers);
    }
  }
}

void
AudioInputCore::internal_set_manager (const AudioInputDevice& device)
{
  current_manager = NULL;
  for (std::set<AudioInputManager*>::iterator iter = managers.begin ();
       iter != managers.end ();
       ++iter) {

     if ((*iter)->set_device (device))
       current_manager = (*iter);
  }

  // If the desired manager could not be found,
  // we se the default device. The default device
  // MUST ALWAYS be loaded and openable
  if (current_manager) {

    current_device  = device;
  }
  else {

    PTRACE(1, "AudioInputCore\tTried to set unexisting device " << device);
    internal_set_fallback();
  }
}

void
AudioInputCore::internal_set_fallback()
{
    current_device.type = AUDIO_INPUT_FALLBACK_DEVICE_TYPE;
    current_device.source = AUDIO_INPUT_FALLBACK_DEVICE_SOURCE;
    current_device.name = AUDIO_INPUT_FALLBACK_DEVICE_NAME;
    PTRACE(1, "AudioInputCore\tFalling back to " << current_device);
    internal_set_manager (current_device);
}

void
AudioInputCore::internal_open (unsigned channels,
			       unsigned samplerate,
			       unsigned bits_per_sample)
{
  PTRACE(4, "AudioInputCore\tOpening device with " << channels << "-" << samplerate << "/" << bits_per_sample );

  if (current_manager && !current_manager->open(channels, samplerate, bits_per_sample)) {

    internal_set_fallback();

    if (current_manager)
      current_manager->open(channels, samplerate, bits_per_sample);
  }
}

void
AudioInputCore::internal_close ()
{
  PTRACE(4, "AudioInputCore\tClosing current device");
  if (current_manager)
    current_manager->close();
}

void
AudioInputCore::calculate_average_level (const short* buffer,
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
