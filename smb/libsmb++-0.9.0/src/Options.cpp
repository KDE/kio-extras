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

#include <string.h>
#include "Options.h"
#include "strtool.h"

Options::Options()
{
	theCallback = 0;
	userName = 0;
	browseServer = 0;
	charSet = 0;
}

Options::~Options()
{
	if (userName) delete userName;
	userName = 0;
	if (browseServer) delete browseServer;
	browseServer = 0;
	if (charSet) delete charSet;
	charSet = 0;
}


// Set up a callback for the library to get passwords
void Options::setPasswordCallback(SmbAnswerCallback *getObject)
{
	theCallback = getObject;
}

// Default user is used instead of anonymous access
void Options::setDefaultUser(const char *aUserName)
{
	if (userName) delete userName;
	newstrcpy(userName, aUserName);
}

// Make browsing work without samba installed locally
void Options::setDefaultBrowseServer(const char *serverName)
{
	if (browseServer) delete browseServer;
	newstrcpy(browseServer, serverName);
}

// Set the charset to use. If not set, no conversion.
void Options::setCharSet(const char *aCharSet)
{
	if (charSet) delete charSet;
	newstrcpy(charSet, aCharSet);
}









