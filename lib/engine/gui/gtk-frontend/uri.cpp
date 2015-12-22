
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

#include "uri.h"
#include "ekiga-settings.h"

using namespace std;

string
gm_transform_number (const string number)
{
  string retval;

  Ekiga::Settings* settings = new Ekiga::Settings (CALL_OPTIONS_SCHEMA);

  if (number.empty() || number[0] == '*' || number[0] == '#' ||
      number.length() == (unsigned)settings->get_int ("local-number-length")) {
    retval = number;
  } else if (number.length() == (unsigned)settings->get_int ("outer-number-length")) {
    retval = settings->get_string ("nonlocal-number-prefix") +
             settings->get_string ("outer-number-prefix") +
             number;
  } else {
    string prefix = settings->get_string ("nonlocal-number-prefix");
    if (!number.compare(0, prefix.length(), prefix))
      retval = number;
    else
      retval = settings->get_string ("nonlocal-number-prefix") + number;
  }

  delete settings;

  return retval;
}

string
gm_make_uri (const string number, const string proxy)
{
  if (number.length() == 0)
    return number;

  return "sip:" + gm_transform_number(number) + "@" + proxy;
}
