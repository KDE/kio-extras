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

#ifndef TYPES_H
#define TYPES_H
#include <sys/stat.h>

// SMBdirent extents the standard dirent response structure for readdir()
// because the smb servers also include some of the stat information
class SMBdirent : public stat
{
public:
 	char *d_name;
	SMBdirent();
	~SMBdirent();
	SMBdirent& operator=(const SMBdirent& dir);
};

// Thanks to Pål-Kristian Engstad <engstad@sqla.com> for introducing
// the idea of a callback class!
class SmbAnswerCallback
{
protected:
	char *lastAnswer;
public:
	SmbAnswerCallback();
	virtual ~SmbAnswerCallback();
	// Possible types are:
	// ANSWER_USER_NAME: a user name is requested for the server in optmessage
	// ANSWER_USER_PASSWORD: a password for the user name in optmessage
	// ANSWER_SERVICE_PASSWORD: a password for the service in optmessage
	virtual char *getAnswer(int type, const char *optmessage);
};
#define ANSWER_USER_NAME 0
#define ANSWER_USER_PASSWORD 1
#define ANSWER_SERVICE_PASSWORD 2

#endif // TYPES_H
