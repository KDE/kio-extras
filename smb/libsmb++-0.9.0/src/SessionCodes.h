/*
    This file is part of the smb++ library
    Copyright (C) 1999  Nicolas Brodu
    nicolas.brodu@free.fr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/

#ifndef __SESSION_CODES_H__
#define __SESSION_CODES_H__
#include "defines.h"
#ifndef USE_SAMBA

/*
RFC 1002 :

Packet structure :

                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      TYPE     |     FLAGS     |            LENGTH             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   /               TRAILER (Packet Type Dependent)                 /
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

// Packet types :

#define SESSION_MESSAGE 0x00
#define SESSION_REQUEST 0x81
#define POSITIVE_SESSION_RESPONSE 0x82
#define NEGATIVE_SESSION_RESPONSE 0x83
#define RETARGET_SESSION_RESPONSE 0x84
#define SESSION_KEEP_ALIVE 0x85

/*
Flags :

     0   1   2   3   4   5   6   7
   +---+---+---+---+---+---+---+---+
   | 0 | 0 | 0 | 0 | 0 | 0 | 0 | E |
   +---+---+---+---+---+---+---+---+

   Symbol     Bit(s)   Description

   E               7   Length extension, used as an additional,
                       high-order bit on the LENGTH field.

   RESERVED      0-6   Reserved, must be zero (0)

[..]

NEGATIVE SESSION RESPONSE packet error code values (in
hexidecimal):

          80 -  Not listening on called name
          81 -  Not listening for calling name
          82 -  Called name not present
          83 -  Called name present, but insufficient resources
          8F -  Unspecified error

*/

#define SESSION_ERROR_CALLED_NOT_LISTENING 0x80
#define SESSION_ERROR_CALLING_NOT_LISTENING 0x81
#define SESSION_ERROR_CALLED_NOT_PRESENT 0x82
#define SESSION_ERROR_NOT_ENOUGH_RESOURCES 0x83
#define SESSION_ERROR 0x8F

#endif
#endif //__SESSION_CODES_H__
