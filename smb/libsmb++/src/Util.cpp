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
#include <iostream.h>
#include <stdio.h> // for sprintf
#include "Resolve.h" // how we can know it's a workgroup or a host in a URL
#include "Util.h"
#include "strtool.h"

Util::Util()
{
	workgroupValue = hostValue = shareValue = dirValue = 0;
	userValue = passValue = ipValue = 0;
	buildURLValue = getNameValue = getURLValue = appendValue = 0;
}

Util::~Util()
{
	if (workgroupValue) delete workgroupValue;
	if (hostValue) delete hostValue;
	if (shareValue) delete shareValue;
	if (dirValue) delete dirValue;
	if (userValue) delete userValue;
	if (passValue) delete passValue;
	if (ipValue) delete ipValue;
	if (buildURLValue) delete buildURLValue;
	if (getNameValue) delete getNameValue;
	if (getURLValue) delete getURLValue;
	if (appendValue) delete appendValue;
	workgroupValue = hostValue = shareValue = dirValue = 0;
	userValue = passValue = ipValue = 0;
	buildURLValue = getNameValue = getURLValue = appendValue = 0;
}

// parse the name(url), and possibly interpret the "." and ".."
void Util::parse(const char *name, bool interpretDirs)
{
	// first destroy all previous values
	if (workgroupValue) delete workgroupValue;
	if (hostValue) delete hostValue;
	if (shareValue) delete shareValue;
	if (dirValue) delete dirValue;
	if (userValue) delete userValue;
	if (passValue) delete passValue;
	if (ipValue) delete passValue;
	workgroupValue = hostValue = shareValue = dirValue = userValue = passValue = 0;
	
	// handle empty case
	if (!name) return;
	
	// Now split the URL into components.
	const int nsep = 8;
	char separators[nsep+1] = "\\/:@?;=&";
	int ntoken=0;
	char *lastpos = (char*)name;
	char *p = (char*)name;
	// process the whole string. don't use strtok...
	for (p = (char*)name; (*p); p++) {
		if (!strchr(separators,*p)) continue;
		if ((lastpos!=p) && (lastpos!=p-1)) ntoken++;
		lastpos=p;
	}
	if ((lastpos!=p) && (lastpos!=p-1)) ntoken++;
//	cerr<<ntoken<<endl;
	
	// create the token and separator tables
	char **token = new (char*)[ntoken];
	char *separator = new char[ntoken];
	// fill the table
	lastpos = (char*)name;
	int count=0; // counter
	for (p = (char*)name; (*p); p++) {
		if (!strchr(separators,*p)) continue;
		int len = p - lastpos;
		if (len>0) {
			token[count] = new char[len+1];
			memcpy(token[count], lastpos, len);
			token[count][len] = 0;
			if (lastpos==name) separator[count] = 0;
			else separator[count] = *(lastpos-1);
			count++;
		}
		lastpos=p+1;
	}
	// and the last token (ends with the terminal \0)
	int len = p - lastpos;
	if (len>0) {
		token[count] = new char[len+1];
		memcpy(token[count], lastpos, len);
		token[count][len] = 0;
		if (lastpos==name) separator[count] = 0;
		else separator[count] = *(lastpos-1);
	}

	// if there is a user field in the url
	bool userfield = false;
	for (int i=0; i<ntoken; i++) if (separator[i]=='@') userfield = true;
	
	// Token analysis
	for (int i=0; i<ntoken; i++) {

		// skip protocol part		
		if ((i==0) && ((!strcmp(token[0],"smb") || (!strcmp(token[0],"SMB")))))
			continue;
	
		// Can handle samba's syntax \\host\share
		if ((i==0) && (separator[0]=='\\')) {
			newstrcpy(hostValue, token[0]);
			continue;
		}
		if ((interpretDirs) && (!strcmp(token[i],".."))) {
			// Find the last value in the url and strip it if it was a path
			// user ".." is strange but allowed here...
			// First dirValue: it can be the concatenation of multiple tokens...
			if (dirValue) {
				// Hmmm, multiple tokens... Cut the string at the last '/'
				if ((p=strrchr(dirValue,'/'))) (*p)=0; // Did I hear a complaint?
				else {
					delete dirValue;
					dirValue=0;
				}
				continue;
			}
			// maybe a share?
			if (shareValue) {
				delete shareValue;
				shareValue=0;
				continue;
			}
			// Well, an host then...
			if (hostValue) {
				delete hostValue;
				hostValue=0;
				continue;
			}
			// So, last chance with workgroup
			if (workgroupValue) {
				delete workgroupValue;
				workgroupValue=0;
				continue;
			}
			// Nop, so the ".." is a perfectly valid token!
			continue;
		}
		// Same with "."
		if ((interpretDirs) && (!strcmp(token[i],"."))) {
			if ((workgroupValue) || (hostValue)) continue;
			// After host, share and dir don't care.
			// Before workgroup, it's a strange but valid token.
		}
		
		// Now the trick is to look at the separator and consider all cases
		switch (separator[i]) {
			case ':':
				// first ':' goes into the password if there is a user field
				if ((userfield) && (!passValue)) {
					newstrcpy(passValue, token[i]);
					break;
				}
				// Otherwise, slight twist of RFC 1738 will make an IP of this
				// field instead of a port (smb://user:pass@host:ip/share/...)
				newstrcpy(ipValue, token[i]);
				break;
			case '@':
				// It might be a workgroup or a host => call Cache
				// note smb://user:pass@workgroup/... could do NT logons!
				if (Resolve::isWorkgroup(token[i])) {
					newstrcpy(workgroupValue, token[i]);
				} else {
					newstrcpy(hostValue, token[i]);
				}
				userfield = false; // cannot be in userfield anymore
				break;
			case '\\':
			case '/':
				// if we have already filled a field, continue in order
				// ah... dir can be composed of multiple tokens => append
				if (dirValue) {
					newstrappend(dirValue,"/",token[i]);
					break;
				}
				if (shareValue) {newstrcpy(dirValue, token[i]); break;}
				if (hostValue) {newstrcpy(shareValue, token[i]); break;}
				if (workgroupValue) {newstrcpy(hostValue, token[i]); break;}
				// first '/' goes into the user if there is a user field
				if ((userfield) && (!userValue)) {
					newstrcpy(userValue, token[i]);
					break;
				}
				// Otherwise, it might be a workgroup or a host => call NMB
				if (Resolve::isWorkgroup(token[i])) {
					newstrcpy(workgroupValue, token[i]);
					break;
				} else {
					newstrcpy(hostValue, token[i]);
					break;
				}
		} // end switch
	} // end token analysis

	// cleanup	
	for (int i=0; i<ntoken; i++) {
//		cerr<<separator[i]<<" "<<token[i]<<endl;
		delete token[i]; // all tokens are non 0
	}
	delete token; // token is non zero
	delete separator;
}


char* Util::workgroup()
{
	return workgroupValue;
}

char* Util::host()
{
	return hostValue;
}

char* Util::share()
{
	return shareValue;
}

char* Util::path()
{
	return dirValue;
}

char* Util::user()
{
	return userValue;
}

char* Util::password()
{
	return passValue;
}

char* Util::ip()
{
	return ipValue;
}

char* Util::workgroup(const char* newValue)
{
	newstrcpy(workgroupValue, newValue);
	return workgroupValue;
}

char* Util::host(const char* newValue)
{
	newstrcpy(hostValue, newValue);
	return hostValue;
}

char* Util::share(const char* newValue)
{
	newstrcpy(shareValue, newValue);
	return shareValue;
}

char* Util::dir(const char* newValue)
{
	newstrcpy(dirValue, newValue);
	return dirValue;
}

char* Util::user(const char* newValue)
{
	newstrcpy(userValue, newValue);
	return userValue;
}

char* Util::password(const char* newValue)
{
	newstrcpy(passValue, newValue);
	return passValue;
}

char* Util::ip(const char* newValue)
{
	newstrcpy(ipValue, newValue);
	return ipValue;
}

// Does the opposite of the parser
char *Util::buildURL(const char* user, const char *password,
	const char* workgroup, const char* host,
	const char* share, const char* file,
	const char* ip)
{
	newstrcpy(buildURLValue,"smb://");
	
	char *userpass=0;
	if (user && password) {
		newstrcpy(userpass,user);
		newstrappend(userpass,":",password);
	} else if (user) {
		newstrcpy(userpass,user);
	} else if (password) {
		newstrappend(userpass,":",password);
	}
	if (userpass) newstrappend(buildURLValue,userpass,"@");

	if (workgroup && host) {
		newstrappend(buildURLValue,workgroup);
		newstrappend(buildURLValue,"/",host);
	} else if (workgroup) newstrappend(buildURLValue,workgroup);
	else if (host) newstrappend(buildURLValue,host);
	
	if (ip) newstrappend(buildURLValue,":",ip);
	if (share) newstrappend(buildURLValue,"/",share);
	if (file) newstrappend(buildURLValue,"/",file);

	return buildURLValue;
}

// appends a string to an URL, making it another URL
// subdirs '.' and ".." can be recognized and interpreted
// This function parses the URL at the same time
// returns 0 on error, or the new URL
char *Util::append(const char* URL, const char *string, bool interpretDirs)
{
	// first have a correct URL
	parse(URL, interpretDirs);
	// Correct URL reconstruction in the return variable
	newstrcpy(appendValue,	buildURL(user(),password(),workgroup(),host(),share(),path(),ip()));
	// append the string
	newstrappend(appendValue, "/", string);
	// reparse for a correct URL and interpreting the directories
	parse(URL, interpretDirs);
	// Final reconstruction
	newstrcpy(appendValue,	buildURL(user(),password(),workgroup(),host(),share(),path(),ip()));
	return appendValue;
}




