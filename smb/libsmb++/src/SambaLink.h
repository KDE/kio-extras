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
#ifndef SAMBALINK_H
#define SAMBALINK_H
#include "defines.h"
#ifdef USE_SAMBA

#include "types.h"
#include "SambaDefs.h"
#include "FileManagementInterface.h"
#include "Util.h"
#include "Options.h"

class SambaLink : public FileManagementInterface, public Options
{
private:
	// static to ensure multiple instance won't reset Samba libs
	static int instances;
	
	// Connect utility (from smbclient source code)
	// Connect to host, share,... given in argument.
	// Suppose the argument is already parsed in util if it's 0
	struct cli_state *connectUtil(const char *toparse = 0);
	
	// The error is written here and send back in the error() call
	int lastError;
protected:
	// Used in any of the functions when an error occurs
	// find the error for this client, always returns -1
	int findError(struct cli_state *cli = 0);
	// set the lastError to an error code, always returns -1
	int findError(int errcode);
	// set the lastError to 0, always returns 0
	// Used in any of the functions when all goes well
	int noError(int ret=0);
	// Get the share list of the host in the url.
	// Use the full url because a user/pass might be in it
	int getShareList(const char* toparse = 0);
	// Get the workgroup list according to the host in the url.
	// Use the full url because a user/pass might be in it
	int getWorkgroupList(const char* toparse = 0);
	// Get the member of workgroup list according to the host in the url.
	// Use the full url because a user/pass might be in it
	int getMemberList(const char* toparse = 0);
	// Will try to find the correct master for util.workgroup()
	int findMaster();
	char *theMaster; // and put the result here

	Util util;

public:
	SambaLink();
	virtual ~SambaLink();
	
	// If not already done, load samba code
	static void loadSamba();
	
	// Those functions work like their standard equivalent,
	// but file descriptors are for internal use only.
	// They accept smbURLs as parameters
	int open(const char* file="", int flags=O_RDWR, int mode=0644);
	int creat(const char* file="", int mode=0644);
	int stat(const char *filename, struct stat *buf);
	int fstat(int fd, struct stat *buf);
	// read and write do caching !
	int read(int fd, void *buf, uint32 count);
	int write(int fd, void *buf, uint32 count);
	// flush forces a write of all buffered data for the given fd
	int flush(int fd);
	int32 lseek(int fd, int32 offset, int from);
	int close(int fd);
	// unlink sends a unlink SMB. Will delete or unlink depending on target OS
	int unlink(const char *file);
	// Renames a file, but doesn't move it between directories
	int rename(const char *fileURL, const char *newname);
	// deletes a directory
	int rmdir(const char *pathname);
	// creates a directory
	int mkdir(const char *pathname);
	// opendir returns a descriptor as well.
	int opendir(const char *name);
	// see types.h for SMBdirent description
	SMBdirent *readdir(int dirdesc);
	int closedir(int dirdesc);
	// OK, the standard version returns nothing. Here -1 = error (0 otherwise)
	int rewinddir(int dirdesc);
	// return the last error encountered when a function returns -1	
	int error();
};

#endif
#endif // SAMBALINK_H

