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
 *                         call.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : declaration of the interface of a call handled by
 *                          the Ekiga::CallManager.
 *
 */


#ifndef __CALL_H__
#define __CALL_H__

#include <boost/smart_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <string>

#include "actor.h"
#include "rtcp-statistics.h"
#include "dynamic-object.h"

namespace Ekiga
{

  /**
   * @addtogroup calls
   * @{
   */

  /*
   * Everything is handled asynchronously and signaled through the
   * Ekiga::CallManager
   */
  class Call
    : public Actor,
      public DynamicObject<Call>
  {

  public:

      enum StreamType { Audio };

      /*
       * Call Management
       */

      /** Hang up the call
      */
      virtual void hang_up () = 0;

      /** Answer an incoming call
      */
      virtual void answer () = 0;

      /** Transfer the call to the specified uri
       * @param uri is the uri where to transfer the call
       * @return false if the transfer could not be tried
       */
      virtual bool transfer (std::string uri) = 0;

      /** Put the call on hold or retrieve it
      */
      virtual void toggle_hold () = 0;

      /** Toggle the stream transmission (if any)
       * @param the stream type
       */
      virtual void toggle_stream_pause (StreamType type) = 0;

      /** Send the given DTMF
       * @param dtmf is the dtmf to send (one char)
       */
      virtual void send_dtmf (const char dtmf) = 0;


      /*
       * Call Information
       */

      /** Return the call id
       * @return: the call id
       */
      virtual const std::string get_id () const = 0;

      /** Return the local party name
       * @return: the local party name
       */
      virtual const std::string get_local_party_name () const = 0;

      /** Return the remote party name
       * @return: the remote party name
       */
      virtual const std::string get_remote_party_name () const = 0;

      /** Return the remote application
       * @return: the remote application
       */
      virtual const std::string get_remote_application () const = 0;

      /** Return the remote callback uri
       * @return: the remote uri
       */
      virtual const std::string get_remote_uri () const = 0;

      /** Return the call duration
       * @return the current call duration
       */
      virtual const std::string get_duration () const = 0;

      /** Return the call start date and time
       * @return the current call start date and time
       */
      virtual time_t get_start_time () const = 0;

      /** Return information about call type
       * @return true if it is an outgoing call
       */
      virtual bool is_outgoing () const = 0;

      /** Return call statistics
       * @return RTCPStatistcs
       */
      virtual const RTCPStatistics & get_statistics () = 0;

      /*
       * Signals
       */

      /* Signal emitted when the call is established
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> established;

      /* Signal emitted when an established call is cleared
       * @param: a string describing why the call was cleared
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>, std::string)> cleared;

      /* Signal emitted when the call is missed, ie cleared
       * without having been established
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> missed;

      /* Signal emitted when the call is forwarded
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> forwarded;

      /* Signal emitted when the call is held
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> held;

      /* Signal emitted when the call is being setup
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> setup;

      /* Signal emitted when the call is retrieved
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> retrieved;

      /* Signal emitted when the remote party is ringing
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>)> ringing;

      /* Signal emitted when a stream is opened
       * @param the stream name
       * @param the stream type
       * @param transmission or reception
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>, std::string, StreamType, bool)> stream_opened;

      /* Signal emitted when a stream is closed
       * @param the stream name
       * @param the stream type
       * @param transmission or reception
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>, std::string, StreamType, bool)> stream_closed;

      /* Signal emitted when a transmitted stream is paused
       * @param the stream name
       * @param the stream type
       * @param transmission or reception
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>, std::string, StreamType)> stream_paused;

      /* Signal emitted when a transmitted stream is resumed
       * @param the stream name
       * @param the stream type
       * @param transmission or reception
       */
      boost::signals2::signal<void(boost::shared_ptr<Ekiga::Call>, std::string, StreamType)> stream_resumed;
    };

/**
 * @}
 */

};

#endif
