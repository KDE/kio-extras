/*
    This file is part of the smb++ library
    Copyright (C) 2000  Nicolas Brodu
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

/*
	This file contains utility functions. These could have been macros,
	but static functions will avoid any library clash (they are private),
	and generate less code than macros (only one intance per #included file).
	The real solution would be to use symbols like SMBPP_newstrcpy or any
	other garbage before to avoid clash (but then the code is less lisible)
	or define a class with those functions static (same thing) or use
	a namespace (same thing again, and less portable maybe).
*/

#ifndef STRTOOL_H
#define STRTOOL_H

#include <string.h>
#include <stdio.h> // for sprintf

#ifndef strcasecmp
#include <ctype.h> // for toupper
#endif


static int mystrcasecmp(const char *c1, const char *c2)
{
	if ((!c1) && (!c2)) return 0;
	if (!c1) return -1;
	if (!c2) return 1;
#ifndef strcasecmp
	int l1=strlen(c1), l2=strlen(c2);
	int i=0;
	while ((i<l1) && (i<l2))
	{
		char a1=toupper(c1[i]), a2=toupper(c2[i]);
		if (a1<a2) return -1;
		if (a1>a2) return 1;
		i++;
	}
	if (l1<l2) return -1;
	if (l1>l2) return 1;
	return 0;
#else
	return strcasecmp(c1, c2);
#endif
}

static int mystrcmp(const char *c1, const char *c2)
{
	if ((!c1) && (!c2)) return 0;
	if (!c1) return -1;
	if (!c2) return 1;
	return strcmp(c1, c2);
}

// copy b into a and manage the new/delete operations for a
static void newstrcpy(char* &a, const char* b)
{
	if (a) delete(a);
	if (b) {
		a = new char[strlen(b)+1];
		strcpy(a,b);
	} else a = 0;
}

// append b to a and manage the new/delete operations for a
static void newstrappend(char* &a, const char* b)
{
	if (!a) newstrcpy(a,b);
	else if (b) {
		char *strappendtmpstring = new char[strlen(a)+strlen(b)+1];
		sprintf(strappendtmpstring,"%s%s",a,b);
		delete a;
		a = strappendtmpstring;
	}
}

// append b and c to a and manage the new/delete operations for a
static void newstrappend(char* &a, const char* b, const char* c)
{
	if (!c) newstrappend(a,b);
	else if (!a) {
		if (!b)	newstrcpy(a,c);
		else {
			char *strappendtmpstring = new char[strlen(b)+strlen(c)+1];
			sprintf(strappendtmpstring,"%s%s",b,c);
			a = strappendtmpstring;
		}
	} else if (b) {
		char *strappendtmpstring = new char[strlen(a)+strlen(b)+strlen(c)+1];
		sprintf(strappendtmpstring,"%s%s%s",a,b,c);
		delete a;
		a = strappendtmpstring;
	} else newstrappend(a,c);
}

#endif

