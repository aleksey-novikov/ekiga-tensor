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
 *                         platform.c  -  description
 *                         ------------------------------------------
 *   begin                : Nov 2006
 *   copyright            : (C) 2006-2007 by Julien Puydt
 *   description          : Implementation of platform-specific workarounds
 */

#include "platform.h"
#include <gtk/gtk.h> // for gtk_show_uri

#ifdef WIN32
#include "winpaths.h"

/* Yes, static variables should be avoided -- but we will need those paths
 * during all application lifetime!
 */

static gchar *basedir = NULL;
static gchar *sysconfdir = NULL;
static gchar *datadir = NULL;
#endif

#ifndef WIN32
#include <gtk/gtk.h>
#else
#include <windows.h>
#endif

void
gm_platform_init ()
{
#ifdef WIN32
  basedir = g_strdup (g_win32_get_package_installation_directory_of_module (NULL));
  sysconfdir = g_strdup (basedir);
  datadir = g_strdup (basedir);
#endif
}

void gm_platform_shutdown ()
{
#ifdef WIN32
  g_free (basedir);
  g_free (sysconfdir);
  g_free (datadir);
#endif
}

#ifdef WIN32
const gchar *
win32_sysconfdir ()
{
  return sysconfdir;
}

const gchar *
win32_datadir ()
{
  return datadir;
}
#endif

#ifndef WIN32
static void
gm_platform_open_uri_fallback (const gchar *uri)
{
  gchar *commandline = NULL;
  gboolean success = FALSE;

  if (!success && g_getenv("KDE_FULL_SESSION") != NULL) {

    commandline = g_strdup_printf ("kfmclient exec %s", uri);
    success = g_spawn_command_line_async (commandline, NULL);
    g_free (commandline);
  }

  if (!success) {

    commandline = g_strdup_printf ("sensible-browser %s", uri);
    success = g_spawn_command_line_async (commandline, NULL);
    g_free (commandline);
  }

  if (!success) {

    commandline = g_strdup_printf ("firefox %s", uri);
    success = g_spawn_command_line_async (commandline, NULL);
    g_free (commandline);
  }

  if (!success) {

    commandline = g_strdup_printf ("konqueror %s", uri);
    success = g_spawn_command_line_async (commandline, NULL);
    g_free (commandline);
  }

  g_return_if_fail (success);
}

void
gm_platform_open_uri (const gchar *uri)
{
  GError *error = NULL;

  g_return_if_fail (uri != NULL);

  if (!gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &error)) {
    g_error_free (error);
    gm_platform_open_uri_fallback (uri);
  }
}

#else

void
gm_platform_open_uri (const gchar *uri)
{
  SHELLEXECUTEINFO sinfo;

  g_return_if_fail (uri != NULL);

  memset (&sinfo, 0, sizeof (sinfo));
  sinfo.cbSize = sizeof (sinfo);
  sinfo.fMask = SEE_MASK_CLASSNAME;
  sinfo.lpVerb = "open";
  sinfo.lpFile = uri;
  sinfo.nShow = SW_SHOWNORMAL;
  sinfo.lpClass = "http";

  (void)ShellExecuteEx (&sinfo); /* leave out any error */
}

#endif
