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
 *                         opal-codec-description.h  -  description
 *                         ------------------------------------------
 *   begin                : written in January 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of the interface of an Opal codec
 *                          description.
 *
 */

#ifndef __OPAL_CODEC_DESCRIPTION_H__
#define __OPAL_CODEC_DESCRIPTION_H__

#include <ptlib.h>
#include <opal/manager.h>

#include "codec-description.h"

namespace Opal {

  /*** Codec description ***/
  class CodecDescription
    : public Ekiga::CodecDescription
    {
  public:
      CodecDescription (const OpalMediaFormat & format,
                        bool active);

  private:
      OpalMediaFormat format;
    };


  class CodecList
    : public Ekiga::CodecList
    {
  public :

      /** Constructor that creates an empty CodecList
       */
      CodecList () {};


      /** Load a CodecList from a list of format names
       * @param list of codec names under the form : format_name|active
       */
      void load (const std::list<std::string> & codecs_config);


  private:
      void GetAllowedFormats (OpalMediaFormatList & list);
    };
}
#endif
