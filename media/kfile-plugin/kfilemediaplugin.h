/* This file is part of the KDE project
   Copyright (C) 2004 Kevin Ottens <ervin ipsquad net>

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

#ifndef _KFILE_MEDIA_H_
#define _KFILE_MEDIA_H_

#include <kfilemetainfo.h>
#include <kurl.h>

#include "medium.h"

class KFileMediaPlugin : public KFilePlugin
{
Q_OBJECT
public:
	KFileMediaPlugin(QObject *parent, const QStringList &args);

	bool readInfo(KFileMetaInfo &info, uint what = KFileMetaInfo::Fastest);

private:
	void addMimeType(const char *mimeType);
	const Medium askMedium(KFileMetaInfo &info);

	quint64 m_total;
	quint64 m_used;
	quint64 m_free;

Q_SIGNALS:
    void leaveModality();

private Q_SLOTS:
	void slotFoundMountPoint(const QString &mountPoint,
	                         quint64 total, quint64 used,
	                         quint64 available);
	void slotDfDone();
	void enterLoop();
};

#endif
