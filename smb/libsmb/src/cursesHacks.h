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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_BOOL
#define HAVE_BOOL
typedef int bool;
#ifdef __cplusplus
const bool false = 0;
const bool true = 1;
#else
#define false (bool)0;
#define true (bool)1;
#endif
#else /* HAVE_BOOL */
#define CXX_BUILTIN_BOOL 1 /* Hack for ncurses */
#endif

#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#ifdef CXX_BUILTIN_BOOL
#undef bool
#endif

// hack for alpha/OSF3
#ifndef getbegyx
void getbegyx(WINDOW *stdscr, int &miny, int &minx)
{
	miny=0; minx=0;
}
#endif
#ifndef getmaxyx
void getmaxyx(WINDOW *stdscr, int &maxy, int &maxx)
{
	maxy=24; maxx=80;  // suppose vt100 term
}
#endif
// hack for alpha/OSF1
#ifndef hline
void hline(int c, int len)
{
	for (int i=0; i<len; i++) printw("%c",c);
}
#endif
#ifndef mvhline
void mvhline(int y, int x, int c, int len)
{
	move(y,x);
    hline(c,len);
}
#endif
#ifndef getnstr
#include <string.h>
void getnstr(char *buf, int len)
{
	char *tmp=new char[1000>len?1000:len];
	getstr(tmp);
	if (len>0) tmp[len-1]=0;
	strcpy(buf,tmp);
	delete tmp;
}
#endif
#ifndef mvgetnstr
void mvgetnstr(int y, int x, char *buf, int len)
{
	move(y,x);
    getnstr(buf,len);
}
#endif

// FreeBSD seems not to have those defines
#ifndef KEY_ENTER
#define KEY_ENTER       0527            /* Enter or send (unreliable) */
#endif
#ifndef KEY_DOWN // assume others won't be defined as well
#define KEY_DOWN        0402            /* Down-arrow */
#define KEY_UP          0403		/* Up-arrow */
#define KEY_LEFT        0404		/* Left-arrow */
#define KEY_RIGHT       0405        /* Right-arrow */
#endif
#ifndef A_NORMAL // assume others won't be defined as well
#define NCURSES_BITS(mask,shift) ((mask) << ((shift) + 8))
#define A_NORMAL	0L
#define A_ATTRIBUTES	NCURSES_BITS(~(1UL - 1UL),0)
#define A_CHARTEXT	(NCURSES_BITS(1UL,0) - 1UL)
#define A_COLOR		NCURSES_BITS(((1UL) << 8) - 1UL,0)
#define A_STANDOUT	NCURSES_BITS(1UL,8)
#define A_UNDERLINE	NCURSES_BITS(1UL,9)
#define A_REVERSE	NCURSES_BITS(1UL,10)
#define A_BLINK		NCURSES_BITS(1UL,11)
#define A_DIM		NCURSES_BITS(1UL,12)
#define A_BOLD		NCURSES_BITS(1UL,13)
#define A_ALTCHARSET	NCURSES_BITS(1UL,14)
#define A_INVIS		NCURSES_BITS(1UL,15)
#ifndef attrset     // too bad :-(
#define attrset(a)
#endif
#ifndef intrflush
#define intrflush(a,b)
#endif
#ifndef keypad
#define keypad(a,b)
#endif
#endif
