/*
 * Ekiga -- A VoIP and Video-Conferencing application
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
 *                         codec-description.h  -  description
 *                         ------------------------------------------
 *   begin                : written in January 2008 by Damien Sandras
 *   copyright            : (c) 2008 by Damien Sandras
 *   description          : declaration of the interface of a codec description.
 *
 */

#ifndef __CODEC_DESCRIPTION_H__
#define __CODEC_DESCRIPTION_H__

#include <iostream>
#include <list>

#include <glib.h>

namespace Ekiga
{

/**
 * @addtogroup calls
 * @{
 */

  /** This class holds the representation of a codec.
   * That representation is different from the codec itself, but can be used
   * at several places in the engine.
   */
  class CodecDescription
    {
  public:

      /** Create an empty codec description
      */
      CodecDescription ();

      /** Create a codec description based on the parameters
       * @param name is the codec name uniquely representing it
       * @param rate is the clock rate
       * @param audio is true if it reprensents an audio codec
       * @param protocols is a comma separated list of protocols supported
       *        by the codec
       * @param active is true if the codec is active
       */
      CodecDescription (const std::string & name,
                        unsigned rate,
                        bool audio,
                        std::string protocols,
                        bool active);

      virtual ~CodecDescription ()
      {}

      /** Return the codec description under the form of a string.
       * @return the std::string representing the string description.
       *         (ie name:active, e.g. G.711:1)
       */
      std::string str ();


      /** name is the codec name
      */
      std::string name;

      /* nice display name
       * and information for "known" codecs
       */
      std::string display_name;
      std::string display_info;

      /** rate is the clock rate
      */
      unsigned rate;

      /** active is true if the codec is active
      */
      bool active;

      /** audio is true if it reprensents an audio codec
      */
      bool audio;

      /** protocols is a list of protocols supported by the codec
      */
      std::list<std::string> protocols;


      /** Return true if both CodecDescription are identical, false otherwise
       * @return true if both CodecDescription are identical, false otherwise
       */
      bool operator== (const CodecDescription & c) const;

      /** Return true if both CodecDescription are different, false otherwise
       * @return true if both CodecDescription are different, false otherwise
       */
      bool operator!= (const CodecDescription & c) const;
    };


  class CodecList
      : public std::list<CodecDescription>
    {
  public :

      /** Constructor that creates an empty CodecList
       */
      CodecList () {};

      virtual ~CodecList ()
      {}

      /** Load a CodecList from a list of format names
       *  The CodecList will contain all codecs from the config list
       *  and all supported codecs if they were not present in the
       *  configuration.
       * @param list of codec names under the form : format_name|active
       */
      virtual void load (G_GNUC_UNUSED const std::list<std::string> & codecs_config) {};

      /** Append the given CodecList at the end of the current CodecList.
       * @param other is the CodecList to append to the current one
       */
      void append (const CodecList& other);

      /** Append the given codec description to the current CodecList.
       * @param descr is the CodecDescription to append to the current list
       */
      void append (const CodecDescription& descr);

      /** Remove the codec description pointed to by the iterator
       * @param iter is the iterator
       */
      void remove (iterator it);

      /* Return true if there is a codec with the given display name
       * @param display_name is the nice name like in known-codecs.h
       * @return true or false
       */
      bool find (const std::string & display_name);

      /** Return the list of audio codecs descriptions in the current CodecList
       * @return the list of audio CodecDescription
       */
      CodecList get_audio_list ();


      /** Return the list of codecs descriptions under their str form
       * @return the list of CodecDescription
       */
      std::list<std::string> slist ();


      /** Return true if both CodecList are identical, false otherwise
       * @return true if both CodecList are identical, false otherwise
       */
      bool operator== (const CodecList & c) const;


      /** Return true if both CodecList are different, false otherwise
       * @return true if both CodecList are different, false otherwise
       */
      bool operator!= (const CodecList & c) const;
    };

/**
 * @}
 */

}

/** Output the CodecList
 */
std::ostream& operator<< (std::ostream & os, const Ekiga::CodecList & c);
#endif
