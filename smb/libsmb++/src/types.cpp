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

#include "types.h"
#include "strtool.h"

SMBdirent::SMBdirent()
{
	d_name = 0;
}

SMBdirent::~SMBdirent()
{
	if (d_name) delete d_name;
	d_name = 0;
}

SMBdirent& SMBdirent::operator=(const SMBdirent& dir)
{
	st_mode = dir.st_mode;
	st_uid = dir.st_uid;
	st_gid = dir.st_gid;
	st_rdev = dir.st_rdev;
	st_size = dir.st_size;
	st_atime = dir.st_atime;
	st_ctime = dir.st_ctime;
	st_mtime = dir.st_mtime;
	newstrcpy(d_name, dir.d_name);
	return *this;
}

SmbAnswerCallback::SmbAnswerCallback()
{
	lastAnswer = 0;
}

SmbAnswerCallback::~SmbAnswerCallback()
{
	if (lastAnswer) delete lastAnswer;
	lastAnswer = 0;
}

char *SmbAnswerCallback::getAnswer(int type, const char *optmessage)
{
	// print your custom/internationalized message here
	// ask user an answer
	// reallocate (newstrcpy) lastAnswer with it, or set it to 0
	return lastAnswer;
}

