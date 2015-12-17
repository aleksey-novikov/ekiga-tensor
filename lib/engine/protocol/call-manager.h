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
 *                         call-manager.h  -  description
 *                         ------------------------------------------
 *   begin                : written in 2007 by Damien Sandras 
 *   copyright            : (c) 2007 by Damien Sandras
 *   description          : Declaration of the interface of a call manager
 *                          implementation backend. A call manager handles calls,
 *                          sometimes simultaneously.
 *
 */


#ifndef __CALL_MANAGER_H__
#define __CALL_MANAGER_H__

#include <set>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

#include <boost/smart_ptr.hpp>

#include "codec-description.h"
#include "dynamic-object.h"
#include "actor.h"

namespace Ekiga
{

/**
 * @addtogroup calls
 * @{
 */

  class CallManager
    : public Actor,
      public DynamicObject<CallManager>
  {
    public:
    class Interface
    {
        public:
        std::string voip_protocol;
        std::string protocol;
        std::string id;
        bool publish;
        unsigned port;
    };
    typedef std::list<Interface> InterfaceList;


    /* The constructor
     */
    CallManager () {};

    /* The destructor
     */
    virtual ~CallManager () {}


    /*
     * CALL MANAGEMENT
     */

    /** Create a call based on the remote uri given as parameter
     * @param: An uri
     * @return: true if a Ekiga::Call could be created
     */
    virtual bool dial (const std::string & uri) = 0;

    /** Hang up all active calls (if any).
     */
    virtual void hang_up () = 0;

    /** Set delay before dropping an incoming call
     * @param delay is the delay after which the call should be rejected
     * (or forwarded if supported by the CallManager and if forward
     *  on no answer is enabled).
     */
    virtual void set_reject_delay (unsigned delay) = 0;

    /** Get delay before dropping an incoming call
     * @return the delay in seconds after which a call should be rejected
     * (or forwarded if supported by the CallManager and if forward
     *  on no answer is enabled).
     */
    virtual unsigned get_reject_delay () const = 0;

    /** Set auto answer
     * @param true if incoming calls should be auto answered.
     */
    virtual void set_auto_answer (bool enabled) = 0;

    /** Get auto answer
     * @return true if incoming calls should be auto answered.
     */
    virtual bool get_auto_answer () const = 0;

    /* Return true if URI can be handled by the CallCore,
     * false otherwise.
     * @param the URI to test
     * @return true of the URI can be handled, false otherwise
     */
    virtual bool is_supported_uri (const std::string & uri) = 0;


    /*
     * PROTOCOL DETAILS
     */

    /** Return the protocol name
     * @return the protocol name
     */
    virtual const std::string & get_protocol_name () const = 0;

    /**
     * @return the interfaces on which we are accepting calls. Generally,
     * under the form protocol:IP:port.
     */
    virtual const InterfaceList get_interfaces () const = 0;

    /** Set the port to listen to for incoming calls
     * @param port is the port on which we should bind
     */
    virtual bool set_listen_port (unsigned port) = 0;

    /** Set the DTMF mode to use to send DTMFs
     * @param mode is the desired DTMF mode
     */
    virtual void set_dtmf_mode (unsigned mode) = 0;

    /** Return the current DTMF mode
     * @return the desired DTMF mode
     */
    virtual unsigned get_dtmf_mode () const = 0;


    /*
     * MISC
     */

    /** Set the display name used on outgoing calls
     * @param name is the display name to use.
     */
    virtual void set_display_name (const std::string & name) = 0;

    /** Return the display name used on outgoing calls
     */
    virtual const std::string & get_display_name () const = 0;


    /*
     * AUDIO AND CODECS DETAILS
     */

    /** Enable the given codecs
     * @param codecs is a set of the codecs and their descriptions
     *        when the function returns, the list also contains disabled
     *        codecs supported by the CallManager. Unsupported codecs
     *        have been removed.
     */
    virtual void set_codecs (CodecList & codecs) = 0;

    /** Return the list of available codecs
     * @return a set of the codecs and their descriptions
     */
    virtual const Ekiga::CodecList & get_codecs () const = 0;

    /** Enable echo cancellation
     * @param enabled is true if echo cancellation should be enabled, false
     * otherwise.
     */
    virtual void set_echo_cancellation (bool enabled) = 0;

    /** Get echo cancellation setting
     * @return true if echo cancellation is enabled.
     */
    virtual bool get_echo_cancellation () const = 0;

    /** Enable silence detection
     * @param enabled is true if silence detection should be enabled, false
     * otherwise.
     */
    virtual void set_silence_detection (bool enabled) = 0;

    /** Get silence detection setting
     * @return true if silence detection is enabled.
     */
    virtual bool get_silence_detection () const = 0;
  };

  /**
   * @}
   */
};
#endif
