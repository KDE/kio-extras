/*
    This file is part of the smb++ library
    Copyright (C) 1999  Nicolas Brodu
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

#ifndef __DES_H__
#define __DES_H__
#include "defines.h"
#ifndef USE_SAMBA

class DES
{
protected:
	unsigned char key[64]; // one bit by byte, simpler algorithm...
	unsigned char K[16][48]; // one bit by byte, simpler algorithm...
	// Encrypt (encdec=0) or decrypt (encdec=1)
	void doIt(unsigned char* data, const unsigned long length, int encdec);
public:
	// key must be 56 bits
	// Anyway, the parity bits introduced in the des-how-to are discarded by the
	// first permutation function
	DES(const unsigned char* key=0);
	~DES();
	void setKey(const unsigned char* key=0);
	// Data should always be aligned to a 64 bit multiple
	// If not, discard the junk (hence force the caller to paddle...)
	void encrypt(unsigned char* data, const unsigned long length);
	void decrypt(unsigned char* data, const unsigned long length);
};

#endif
#endif // __DES_H__
