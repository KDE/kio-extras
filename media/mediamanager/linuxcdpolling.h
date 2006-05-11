/* This file is part of the KDE Project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _LINUXCDPOLLING_H_
#define _LINUXCDPOLLING_H_

#include "backendbase.h"

#include <QObject>
#include <qbytearray.h>
#include <QMap>

class DiscType
{
public:
	enum Type { None, Unknown, Audio, Data, DVD, Mixed,
	            Blank, VCD, SVCD, UnknownType, Broken };

	DiscType(Type type = Unknown);

	bool isKnownDisc() const;
	bool isDisc() const;
	bool isNotDisc() const;
	bool isData() const;

	operator int() const;

private:
	Type m_type;
};

class PollingThread;

class LinuxCDPolling : public QObject, public BackendBase
{
Q_OBJECT

public:

	LinuxCDPolling(MediaList &list);
	virtual ~LinuxCDPolling();

	/**
	 * Find the disc type of the medium inserted in a drive
	 * (considered to be a cdrom or dvdrom)
	 *
	 * @param devNode the path to the device to test
	 * @param current the current known state of the drive
	 * @return the disc type
	 */
	static DiscType identifyDiscType(const QByteArray &devNode,
		 const DiscType &current = DiscType::Unknown);

private Q_SLOTS:
	void slotMediumAdded(const QString &id);
	void slotMediumRemoved(const QString &id);
	void slotMediumStateChanged(const QString &id);
	void slotTimeout();

private:
	void applyType(DiscType type, const Medium *medium);

	static bool hasDirectory(const QByteArray &devNode, const QByteArray &dir);

	QMap<QString, PollingThread*> m_threads;
	QStringList m_excludeNotification;
};

#endif
