/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KIO_REMOTE_H
#define KIO_REMOTE_H

#include <kio/slavebase.h>
#include "remoteimpl.h"
//Added by qt3to4:
#include <Q3CString>

class RemoteProtocol : public KIO::SlaveBase
{
public:
	RemoteProtocol(const Q3CString &protocol, const Q3CString &pool,
	               const Q3CString &app);
	virtual ~RemoteProtocol();

	virtual void listDir(const KURL &url);
	virtual void stat(const KURL &url);
	virtual void del(const KURL &url, bool isFile);
	virtual void get(const KURL &url);
	virtual void rename(const KURL &src, const KURL &dest, bool overwrite);

private:
	void listRoot();
	
	RemoteImpl m_impl;
};

#endif
