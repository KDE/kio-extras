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
    Utility class. Mainly for SMB Urls.
*/
#ifndef UTIL_H
#define UTIL_H

class Util
{
protected:
	// Stores the returned pointers for automatic deallocation
	// The caller doesn't need to manage our memory anymore!
	char *workgroupValue, *hostValue, *shareValue, *dirValue;
	char *userValue, *passValue, *ipValue;
	char *buildURLValue, *getNameValue, *getURLValue, *appendValue;
public:
	// Constructor and destructor ensure no memory leak
	Util();
	~Util();
	// Simple parser for smb URLs. get the results the the named functions
	// Warning: the URL should be decoded first, because it will be treated
	// exactly (i.e. '%20' will mean this text and not a space).
	// It will also interpret the "." and ".." by default
	void parse(const char *name, bool interpretDirs = true);
	char* user();
	char* password();
	char* ip();
	char* workgroup();
	char* host();
	char* share();
	char* path();
	// Can override the current values if necessary
	char* user(const char* newValue);
	char* password(const char* newValue);
	char* ip(const char* newValue);
	char* workgroup(const char* newValue);
	char* host(const char* newValue);
	char* share(const char* newValue);
	char* dir(const char* newValue);
	// Does the opposite of the parser
	char *buildURL(const char* user=0, const char *password=0,
		const char* workgroup=0, const char* host=0,
		const char* share=0, const char* file=0,
		const char* ip=0);
	// appends a string to an URL, making it another URL
	// subdirs ".." and "." can be recognized and interpreted
	// returns 0 on error, or the new URL
	char *append(const char* URL, const char *string, bool interpretDirs = true);
};

#endif
