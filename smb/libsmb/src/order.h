/*
	This file is part of the smb library
    Copyright (C) 1999  Nicolas Brodu
    brodu@iie.cnam.fr

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

#ifndef __ORDER_H__
#define __ORDER_H__

// this header won't be installed -- can include config.h
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Default -- install a system-wide version to change this
#define NBSCOPE ""
#undef DEBUG
#undef DNSQUERY
#define OVERKILL 1

/*********************
 INTEGER SIZE SECTION
*********************/

#if SIZEOF_CHAR != 1
#error No one-byte sized type found !
#endif

/* Modify those two lines if you have the error above */
#define int8 char
#define uint8 unsigned char

/* #undef int16 */
#if SIZEOF_SHORT == 2
#define int16 short
#define uint16 unsigned short
#elif SIZEOF_INT == 2
#define int16 int
#define uint16 unsigned int
/* 
   Some machines do not have a 16 bit integer type
   See below for what happens then
   or uncomment this if your program cannot handle it */
/* #else */
/* #error No two-byte sized type found ! */
#endif

#if SIZEOF_INT == 4
#define int32 int
#define uint32 unsigned int
#elif SIZEOF_LONG == 4
#define int32 long
#define uint32 unsigned long
#else
#error No four-byte sized type found !
#endif

#ifndef int16
/*
   int16 will be redefined in this case to int32
   Beware ! Nowhere in the program int16 can then be assumed to
   have a size of 2 bytes. If it is realy critical, just
   implements a workaround (ex : struct with 2 int8), or
   use #ifdef INT16_IS_4_BYTE_LONG, or if it really matters
   uncomment the two lines above
*/
#define int16 int32
#define INT16_IS_4_BYTE_LONG
#endif

/*******************
 BIG ENDIAN SECTION
*******************/

/* The macros below are used to code big and little endian
   into or from local order.
*/

#if WORDS_BIGENDIAN == 0

#define LITEND16(i) (i)
#define LITEND32(i) (i)

#define BIGEND16(i) (((i>>8)&0xFF)|((i<<8)&0xFF00))
#define BIGEND32(i) (((i>>24)&(0xFF))|((i>>8)&(0xFF00))|((i<<24)&(0xFF000000))|((i<<8)&(0xFF0000)));

#else /* CPU is big endian */

#define BIGEND16(i) (i)
#define BIGEND32(i) (i)

#define LITEND16(i) (((i>>8)&0xFF)|((i<<8)&0xFF00))
#define LITEND32(i) (((i>>24)&(0xFF))|((i>>8)&(0xFF00))|((i<<24)&(0xFF000000))|((i<<8)&(0xFF0000)));

#endif /* big endian */

#endif /* __ORDER_H__ */
