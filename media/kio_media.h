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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KIO_MEDIA_H_
#define _KIO_MEDIA_H_

#include <kio/forwardingslavebase.h>

#include "mediaimpl.h"

class MediaProtocol : public KIO::ForwardingSlaveBase
{
Q_OBJECT
public:
	MediaProtocol(const QCString &protocol, const QCString &pool,
	              const QCString &app);
	virtual ~MediaProtocol();

	virtual bool rewriteURL(const KURL &url, KURL &newUrl);

	virtual void put(const KURL &url, int permissions,
	                 bool overwrite, bool resume);
	virtual void rename(const KURL &src, const KURL &dest, bool overwrite);
	virtual void mkdir(const KURL &url, int permissions);
	virtual void del(const KURL &url, bool isFile);
	virtual void stat(const KURL &url);
	virtual void listDir(const KURL &url);

private slots:
	void slotWarning( const QString &msg );
	
private:
	void listRoot();

	MediaImpl m_impl;
};

#endif
