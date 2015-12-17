/*
 * Ekiga -- A VoIP and Video-Conferencing application
 * Copyright (C) 2000-2015 Damien Sandras <dsandras@seconix.com>

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
 *                         rtp-statistics.h  -  description
 *                         ------------------------------------------
 *   begin                : Written in 2015 by Damien Sandras
 *   copyright            : (c) 2015 by Damien Sandras
 *   description          : Declaration of minimal statistics all components
 *                          must support.
 *
 */


#ifndef __RTCP_STATISTICS_H__
#define __RTCP_STATISTICS_H__

class RTCPStatistics {

public:
    RTCPStatistics () :
        transmitted_audio_bandwidth (0),
        received_audio_bandwidth (0),
        jitter (-1),
        remote_jitter (-1),
        lost_packets (0),
        remote_lost_packets (0) {};

    /* Audio */
    std::string transmitted_audio_codec;
    unsigned transmitted_audio_bandwidth; // in kbits/s
    std::string received_audio_codec;
    unsigned received_audio_bandwidth; // in kbits/s
    int jitter; // in ms (-1 is N/A, as given by opal)
    int remote_jitter; // in ms (-1 is N/A, as given by opal)

    /* Total */
    unsigned lost_packets;        // as a percentage
    unsigned remote_lost_packets; // as a percentage
};

#endif
