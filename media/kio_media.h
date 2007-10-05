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

#ifndef _KIO_MEDIA_H_
#define _KIO_MEDIA_H_

#include <kio/forwardingslavebase.h>

#include "mediaimpl.h"

class MediaProtocol : public KIO::ForwardingSlaveBase
{
Q_OBJECT
public:
	MediaProtocol(const QByteArray &protocol, const QByteArray &pool,
	              const QByteArray &app);
	virtual ~MediaProtocol();

	virtual bool rewriteUrl(const KUrl &url, KUrl &newUrl);

	virtual void put(const KUrl &url, int permissions,
	                 KIO::JobFlags flags);
	virtual void rename(const KUrl &src, const KUrl &dest, KIO::JobFlags flags);
	virtual void mkdir(const KUrl &url, int permissions);
	virtual void del(const KUrl &url, bool isFile);
	virtual void stat(const KUrl &url);
	virtual void listDir(const KUrl &url);

private Q_SLOTS:
	void slotWarning( const QString &msg );
	
private:
	void listRoot();

	MediaImpl m_impl;
};

#endif
