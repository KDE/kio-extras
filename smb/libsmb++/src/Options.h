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
    This class enables the user to set up some options
*/
#ifndef OPTIONS_H
#define OPTIONS_H

class SmbAnswerCallback;

class Options
{
protected:
	SmbAnswerCallback *theCallback;
	char *userName;
	char *browseServer;
	char *charSet;
	char *broadcast;
	char *wins;
public:
	Options();
	virtual ~Options();
	// Set up a callback for the library to get passwords
	virtual void setPasswordCallback(SmbAnswerCallback *getObject);
	// Default user is used instead of anonymous access
	virtual void setDefaultUser(const char *aUserName);
	// Make browsing work without samba installed locally
	virtual void setDefaultBrowseServer(const char *serverName);
	
	// The following has an effect only for native code.
	// Set the charset to use. If not set, no conversion.
	virtual void setCharSet(const char *aCharSet);
	// Set the network broadcast address
	virtual void setBroadcastAddress(const char *addr);
	// Set the WINS address
	virtual void setWINSAddress(const char *addr);
};

#endif // OPTIONS_H

