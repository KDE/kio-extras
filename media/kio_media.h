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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KIO_MEDIA_H_
#define _KIO_MEDIA_H_

#include <kio/slavebase.h>
#include <qobject.h>

#include "mediaimpl.h"

class MediaProtocol : public QObject, public KIO::SlaveBase
{
Q_OBJECT
public:
	MediaProtocol(const QCString &protocol, const QCString &pool,
	              const QCString &app);
	virtual ~MediaProtocol();

	virtual void get(const KURL &url);
	virtual void put(const KURL &url, int mode,
	                 bool overwrite, bool resume);
	virtual void copy(const KURL &src, const KURL &dest,
	                  int mode, bool overwrite );
	virtual void rename(const KURL &src, const KURL &dest, bool overwrite);
	virtual void symlink(const QString &target, const KURL &dest,
	                     bool overwrite);

	virtual void stat(const KURL &url);
	virtual void listDir(const KURL &url);
	virtual void mkdir(const KURL &url, int permissions);
	virtual void chmod(const KURL &url, int permissions);
	virtual void del(const KURL &url, bool isFile);

	// TODO?
	/**
	 * Special commands supported by this slave:
	 * 1 - mount
	 * 2 - unmount
	 */
	//virtual void special( const QByteArray &data);
	//void unmount(const QString &mediumName);
	//void mount(const QString &mediumName, bool readOnly);

private slots:
	void slotResult(KIO::Job *job);
	void slotStatResult(KIO::Job *job);
	void slotEntries(KIO::Job *job, const KIO::UDSEntryList &entries);

private:
	void listRoot();

	MediaImpl m_impl;
};

#endif
