/* Ekiga -- A VoIP application
 * Copyright (C) 2000-2013 Damien Sandras <dsandras@seconix.com>
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
 *                         ekiga-ettings.h  -  description
 *                         -------------------------------
 *   begin                : Sat Dec 23 2000
 *   copyright            : (C) 2000-2013 by Damien Sandras
 *   description          : This file defines gsettings keys
 *
 */


#ifndef EKIGA_SETTINGS_H_
#define EKIGA_SETTINGS_H_

#include <boost/smart_ptr.hpp>
#include <boost/signals2.hpp>

#include <gio/gio.h>

#include "config.h"

#define USER_INTERFACE "org.gnome." PACKAGE_NAME ".general.user-interface"

#define PERSONAL_DATA_SCHEMA "org.gnome." PACKAGE_NAME ".general.personal-data"
#define SOUND_EVENTS_SCHEMA "org.gnome." PACKAGE_NAME ".general.sound-events"
#define AUDIO_DEVICES_SCHEMA "org.gnome." PACKAGE_NAME ".devices.audio"

#define PROTOCOLS_SCHEMA "org.gnome." PACKAGE_NAME ".protocols"
#define PORTS_SCHEMA PROTOCOLS_SCHEMA ".ports"
#define SIP_SCHEMA PROTOCOLS_SCHEMA ".sip"
#define CALL_FORWARDING_SCHEMA PROTOCOLS_SCHEMA ".call-forwarding"

#define GENERAL_SCHEMA "org.gnome." PACKAGE_NAME ".general"
#define NAT_SCHEMA GENERAL_SCHEMA ".nat"
#define CALL_OPTIONS_SCHEMA GENERAL_SCHEMA ".call-options"

#define CODECS_SCHEMA "org.gnome." PACKAGE_NAME ".codecs"
#define AUDIO_CODECS_SCHEMA CODECS_SCHEMA ".audio"

#define CONTACTS_SCHEMA "org.gnome." PACKAGE_NAME ".contacts"

#define QUEUE_SCHEMA "org.gnome." PACKAGE_NAME ".queue"


namespace Ekiga {

  /*
   * This is a C++ wrapper class around GSettings.
   *
   * It is recommended to use it in C++ code, but also
   * when defining GObjects.
   *
   * When defining GObjects, you can use the standard g_signal_connect
   * instead of the "changed" C++ signal.
   */
  class Settings : boost::noncopyable {

    static void f_callback (G_GNUC_UNUSED GSettings *settings,
                            gchar *key,
                            const Settings* self)
    {
      self->changed (std::string (key));
    }


public:

    Settings (const std::string & schema)
    {
      init (schema);
    }

    Settings (const std::string & schema,
              boost::function1<void, const std::string &> & f)
    {
      init (schema);
      changed.connect (f);
    }

    ~Settings ()
    {
      g_signal_handler_disconnect (gsettings, handler);
      g_clear_object (&gsettings);
    }

    GSettings* get_g_settings ()
    {
      return gsettings;
    }

    const std::string get_string (const std::string & key)
    {
      gchar *value = g_settings_get_string (gsettings, key.c_str ());
      std::string result;

      if (value)
        result = value;

      g_free (value);
      return result;
    }

    void set_string (const std::string & key, const std::string & value)
    {
      g_settings_set_string (gsettings, key.c_str (), value.c_str ());
    }

    int get_int (const std::string & key)
    {
      return g_settings_get_int (gsettings, key.c_str ());
    }

    void set_int (const std::string & key, int i)
    {
      g_settings_set_int (gsettings, key.c_str (), i);
    }

    int get_enum (const std::string & key)
    {
      return g_settings_get_enum (gsettings, key.c_str ());
    }

    void set_enum (const std::string & key, int i)
    {
      g_settings_set_enum (gsettings, key.c_str (), i);
    }

    bool get_bool (const std::string & key)
    {
      return g_settings_get_boolean (gsettings, key.c_str ());
    }

    void set_bool (const std::string & key, bool i)
    {
      g_settings_set_boolean (gsettings, key.c_str (), i);
    }

    std::list<std::string> get_string_list (const std::string & key)
    {
      gchar **values = g_settings_get_strv (gsettings, key.c_str ());
      std::list<std::string> result;

      for (int i = 0 ; values && values[i] != NULL ; i++)
        result.push_back (values[i]);

      return result;
    }

    void set_string_list (const std::string & key, const std::list<std::string> & list)
    {
      gchar **values = (gchar**) g_malloc (sizeof (gchar*) * (list.size() + 1));
      int i = 0;
      for (std::list<std::string>::const_iterator it = list.begin ();
           it != list.end ();
           it++)
        values[i++] = g_strdup (it->c_str ());
      values[i++] = NULL;

      g_settings_set_strv (gsettings, key.c_str (), values);
      g_strfreev (values);
    }

    GSList* get_slist (const std::string & key)
    {
      GSList* list = NULL;
      gchar **values = g_settings_get_strv (gsettings, key.c_str ());
      if (values) {
        for (int i = 0 ; values[i] ; i++)
          list = g_slist_append (list, g_strdup (values[i]));
      }
      g_strfreev (values);

      return list;
    }

    void set_slist (const std::string & key, const GSList *list)
    {
      GArray* array = g_array_new (TRUE, TRUE, sizeof (gchar *));
      for (const GSList *l = list ; l ; l = g_slist_next (l))
        array = g_array_append_val (array, l->data);
      g_settings_set_strv (gsettings, key.c_str (), (const gchar **) array->data);
      g_array_free (array, TRUE);
    }

    void get_int_tuple (const std::string & key, int & a, int & b)
    {
      std::string  s = get_string (key);
      gchar **couple = NULL;
      if (!s.empty ())
        couple = g_strsplit (s.c_str (), ":", 2);
      if (couple && couple [0])
        a = atoi (couple [0]);
      if (couple && couple [1])
        b = atoi (couple [1]);

      g_strfreev (couple);
    }

    boost::signals2::signal<void(std::string)> changed;

private:
    void init (const std::string & schema)
    {
      gsettings = g_settings_new (schema.c_str ());
      handler = g_signal_connect (gsettings, "changed", G_CALLBACK (&f_callback), this);
    }

    gulong handler;
    GSettings *gsettings;
  };

  typedef boost::shared_ptr<Settings> SettingsPtr;
  typedef boost::function1<void, const std::string &> SettingsCallback;
}

#endif /* EKIGA_SETTINGS_H */
