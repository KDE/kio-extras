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
    This class provides a common interface for the underlying SMB core functions
    It's an abstract one. Use the SMB class to instanciate (it will select
    between the different possible SMB core systems)
*/

#ifndef FILE_MANAGEMENT_INTERFACE_H
#define FILE_MANAGEMENT_INTERFACE_H

#include <fcntl.h> // for the O_RDWR flag
#include "defines.h"
#include "types.h"

class FileManagementInterface
{
public:
	FileManagementInterface();
	virtual ~FileManagementInterface();
	// Those functions work like their standard equivalent,
	// but file descriptors are for internal use only.
	// They accept smbURLs as parameters
	virtual int open(const char* file="", int flags=O_RDWR, int mode=0644) = 0;
	virtual int creat(const char* file="", int mode=0644) = 0;
	virtual int stat(const char *filename, struct stat *buf) = 0;
	virtual int fstat(int fd, struct stat *buf) = 0;
	// read and write do caching !
	virtual int read(int fd, void *buf, uint32 count) = 0;
	virtual int write(int fd, void *buf, uint32 count) = 0;
	// flush forces a write of all buffered data for the given fd
	virtual int flush(int fd) = 0;
	virtual int32 lseek(int fd, int32 offset, int from) = 0;
	virtual int close(int fd) = 0;
	// unlink sends a unlink SMB. Will delete or unlink depending on target OS
	virtual int unlink(const char *file) = 0;
	// Renames a file, but doesn't move it between directories
	virtual int rename(const char *fileURL, const char *newname) = 0;
	// deletes a directory
	virtual int rmdir(const char *pathname) = 0;
	// creates a directory
	virtual int mkdir(const char *pathname) = 0;
	// opendir returns a descriptor as well.
	virtual int opendir(const char *name) = 0;
	// see types.h for SMBdirent description
	virtual SMBdirent *readdir(int dirdesc) = 0;
	virtual int closedir(int dirdesc) = 0;
	// OK, the standard version returns nothing. Here -1 = error (0 otherwise)
	virtual int rewinddir(int dirdesc) = 0;
	// return the last error encountered when a function returns -1	
	virtual int error() = 0;
};

#endif // FILE_MANAGEMENT_INTERFACE_H
