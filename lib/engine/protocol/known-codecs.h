/*
 * Ekiga -- A VoIP application
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
 *                         know-codecs.h  -  description
 *                         ------------------------------
 *   begin                : written in December 2014 by Damien Sandras
 *   copyright            : (c) 2014 by Damien Sandras
 *   description          : Codec descriptions.
 *
 */

#ifndef __KNOWN_CODECS_H
#define __KNOWN_CODECS_H

static const char* KnownCodecs[][3] = {
    { "G.722.1C-24K",   N_("G.722.1C"),        N_("Annex C, 14 kHz mode at 24 kbit/s")        },
    { "G.722.1C-32K",   N_("G.722.1C"),        N_("Annex C, 14 kHz mode at 32 kbit/s")        },
    { "G.722.1C-48K",   N_("G.722.1C"),        N_("Annex C, 14 kHz mode at 48 kbit/s")        },
    { "Opus-48",        N_("Opus"),            N_("Opus codec, 48 kHz mode")                  },
    { "iSAC-16kHz",     N_("iSAC"),            N_("internet Speech Audio Codec, 16 kHz mode") },
    { "iSAC-32kHz",     N_("iSAC"),            N_("internet Speech Audio Codec, 32 kHz mode") },
    { "G.722-64k",      N_("G.722"),           N_("Basic G.722")                              },
    { "G.722.1-24K",    N_("G.722.1"),         N_("7 kHz mode at 24 kbits/s")                 },
    { "G.722.1-32K",    N_("G.722.1"),         N_("7 kHz mode at 32 kbits/s")                 },
    { "G.722.2",        N_("G.722.2"),         N_("AMR Wideband")                             },
    { "GSM-AMR",        N_("AMR-NB"),          N_("AMR Narrowband")                           },
    { "SILK-8",         N_("SILK"),            N_("Skype SILK 8 kbits/s")                     },
    { "SILK-16",        N_("SILK"),            N_("Skype SILK 16 kbits/s")                    },
    { "G.711-ALaw-64k", N_("G.711 A-Law"),     N_("Standard G.711")                           },
    { "G.711-uLaw-64k", N_("G.711 µ-Law"),     N_("Standard G.711")                           },
    { "G.726-16k",      N_("G.726"),           N_("G.726 at 16 kbits/s")                      },
    { "G.726-24k",      N_("G.726"),           N_("G.726 at 24 kbits/s")                      },
    { "G.726-32k",      N_("G.726"),           N_("G.726 at 32 kbits/s")                      },
    { "G.726-40k",      N_("G.726"),           N_("G.726 at 40 kbits/s")                      },
    { "MS-IMA-ADPCM",   N_("IMA ADPCM"),       N_("Microsoft IMA ADPCM")                      },
    { "H.261",          N_("H.261"),           N_("Basic H.261")                              },
    { "H.263",          N_("H.263"),           N_("H.263")                                    },
    { "H.263plus",      N_("H.263+"),          N_("H.263 Version 1998")                       },
    { "H.264-0",        N_("H.264"),           N_("H.264 Single NAL mode")                    },
    { "H.264-1",        N_("H.264"),           N_("H.264 Interleaved mode")                   },
    { "H.264-High",     N_("H.264"),           N_("H.264 Interleaved mode, High-Profile")     },
    { "MPEG4",          N_("MPEG-4"),          N_("MPEG-4")                                   },
    { "VP8-WebM",       N_("VP8"),             N_("VP8")                                      },
    { NULL,             NULL,                  NULL                                           }
};

#endif
