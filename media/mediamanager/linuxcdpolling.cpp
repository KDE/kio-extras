/* This file is part of the KDE Project
   Copyright (c) 2003 Gav Wood <gav kde org>
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

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

/* Some code of this file comes from kdeautorun */

#include "linuxcdpolling.h"

#include <qthread.h>
#include <qmutex.h>
#include <qtimer.h>
#include <qfile.h>
//Added by qt3to4:
#include <Q3CString>

#include <kdebug.h>

#include "fstabbackend.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// Never ever include directly a kernel header!
// #include <linux/cdrom.h>
// Instead we redefine the necessary (copied from the header)

/* This struct is used by the CDROMREADTOCHDR ioctl */
struct cdrom_tochdr
{
	unsigned char cdth_trk0;      /* start track */
	unsigned char cdth_trk1;      /* end track */
};

#define CDROMREADTOCHDR         0x5305 /* Read TOC header
                                           (struct cdrom_tochdr) */
#define CDROM_DRIVE_STATUS      0x5326  /* Get tray position, etc. */
#define CDROM_DISC_STATUS       0x5327  /* Get disc type, etc. */

/* drive status possibilities returned by CDROM_DRIVE_STATUS ioctl */
#define CDS_NO_INFO             0       /* if not implemented */
#define CDS_NO_DISC             1
#define CDS_TRAY_OPEN           2
#define CDS_DRIVE_NOT_READY     3
#define CDS_DISC_OK             4

/* return values for the CDROM_DISC_STATUS ioctl */
/* can also return CDS_NO_[INFO|DISC], from above */
#define CDS_AUDIO               100
#define CDS_DATA_1              101
#define CDS_DATA_2              102
#define CDS_XA_2_1              103
#define CDS_XA_2_2              104
#define CDS_MIXED               105

#define CDSL_CURRENT            ((int) (~0U>>1))

// -------



DiscType::DiscType(Type type)
	: m_type(type)
{
}

bool DiscType::isKnownDisc() const
{
	return m_type != None
	    && m_type != Unknown
	    && m_type != UnknownType
	    && m_type != Broken;
}

bool DiscType::isDisc() const
{
	return m_type != None
	    && m_type != Unknown
	    && m_type != Broken;
}

bool DiscType::isNotDisc() const
{
	return m_type == None;
}

bool DiscType::isData() const
{
	return m_type == Data;
}

DiscType::operator int() const
{
	return (int)m_type;
}


class PollingThread : public QThread
{
public:
	PollingThread(const Q3CString &devNode) : m_dev(devNode)
	{
		kdDebug(1219) << "PollingThread::PollingThread("
		          << devNode << ")" << endl;
		m_stop = false;
		m_currentType = DiscType::None;
		m_lastPollType = DiscType::None;
	}


	void stop()
	{
		QMutexLocker locker(&m_mutex);
		m_stop = true;
	}

	bool hasChanged()
	{
		QMutexLocker locker(&m_mutex);

		return m_currentType!=m_lastPollType;
	}

	DiscType type()
	{
		QMutexLocker locker(&m_mutex);
		m_currentType = m_lastPollType;
		return m_currentType;
	}

protected:
	virtual void run()
	{
		kdDebug(1219) << "PollingThread(" << m_dev << ") start" << endl;
		while (!m_stop && m_lastPollType!=DiscType::Broken)
		{
			m_mutex.lock();
			DiscType type = m_lastPollType;
			m_mutex.unlock();

			type = LinuxCDPolling::identifyDiscType(m_dev, type);

			m_mutex.lock();
			m_lastPollType = type;
			m_mutex.unlock();

			msleep(500);
		}
		kdDebug(1219) << "PollingThread(" << m_dev << ") stop" << endl;
	}

private:
	QMutex m_mutex;
	bool m_stop;
	const Q3CString m_dev;
	DiscType m_currentType;
	DiscType m_lastPollType;
};


LinuxCDPolling::LinuxCDPolling(MediaList &list)
	: QObject(), BackendBase(list)
{
	connect(&m_mediaList, SIGNAL(mediumAdded(const QString &,
	                                        const QString &, bool)),
	        this, SLOT(slotMediumAdded(const QString &)) );

	connect(&m_mediaList, SIGNAL(mediumRemoved(const QString &,
	                                          const QString &, bool)),
	        this, SLOT(slotMediumRemoved(const QString &)) );

	connect(&m_mediaList, SIGNAL(mediumStateChanged(const QString &,
	                                               const QString &, bool, bool)),
	        this, SLOT(slotMediumStateChanged(const QString &)) );

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
	timer->start(500);
}

LinuxCDPolling::~LinuxCDPolling()
{
	QMap<QString, PollingThread*>::iterator it = m_threads.begin();
	QMap<QString, PollingThread*>::iterator end = m_threads.end();

	for(; it!=end; ++it)
	{
		PollingThread *thread = it.data();
		thread->stop();
		thread->wait();
		delete thread;
	}
}

void LinuxCDPolling::slotMediumAdded(const QString &id)
{
	kdDebug(1219) << "LinuxCDPolling::slotMediumAdded(" << id << ")" << endl;

	if (m_threads.contains(id)) return;

	const Medium *medium = m_mediaList.findById(id);

	QString mime = medium->mimeType();
	kdDebug(1219) << "mime == " << mime << endl;

	if (mime.find("dvd")==-1 && mime.find("cd")==-1) return;

	if (!medium->isMounted())
	{
		m_excludeNotification.append( id );
		
		Q3CString dev = QFile::encodeName( medium->deviceNode() ).data();
		PollingThread *thread = new PollingThread(dev);
		m_threads[id] = thread;
		thread->start();
	}
}

void LinuxCDPolling::slotMediumRemoved(const QString &id)
{
	kdDebug(1219) << "LinuxCDPolling::slotMediumRemoved(" << id << ")" << endl;

	if (!m_threads.contains(id)) return;

	PollingThread *thread = m_threads[id];
	m_threads.remove(id);
	thread->stop();
	thread->wait();
	delete thread;

	m_excludeNotification.remove(id);
}

void LinuxCDPolling::slotMediumStateChanged(const QString &id)
{
	kdDebug(1219) << "LinuxCDPolling::slotMediumStateChanged("
	          << id << ")" << endl;

	const Medium *medium = m_mediaList.findById(id);

	QString mime = medium->mimeType();
	kdDebug(1219) << "mime == " << mime << endl;

	if (mime.find("dvd")==-1 && mime.find("cd")==-1) return;

	if (!m_threads.contains(id) && !medium->isMounted())
	{
		// It is just a mount state change, no need to notify
		m_excludeNotification.append( id );
		
		Q3CString dev = QFile::encodeName( medium->deviceNode() ).data();
		PollingThread *thread = new PollingThread(dev);
		m_threads[id] = thread;
		thread->start();
	}
	else if (m_threads.contains(id) && medium->isMounted())
	{
		PollingThread *thread = m_threads[id];
		m_threads.remove(id);
		thread->stop();
		thread->wait();
		delete thread;
	}
}

void LinuxCDPolling::slotTimeout()
{
	//kdDebug(1219) << "LinuxCDPolling::slotTimeout()" << endl;

	QMap<QString, PollingThread*>::iterator it = m_threads.begin();
	QMap<QString, PollingThread*>::iterator end = m_threads.end();

	for(; it!=end; ++it)
	{
		QString id = it.key();
		PollingThread *thread = it.data();

		if (thread->hasChanged())
		{
			DiscType type = thread->type();
			const Medium *medium = m_mediaList.findById(id);
			applyType(type, medium);
		}
	}
}

static QString baseType(const Medium *medium)
{
	kdDebug(1219) << "baseType(" << medium->id() << ")" << endl;

	QString devNode = medium->deviceNode();
	QString mountPoint = medium->mountPoint();
	QString fsType = medium->fsType();
	bool mounted = medium->isMounted();

	QString mimeType, iconName, label;

	FstabBackend::guess(devNode, mountPoint, fsType, mounted,
	                    mimeType, iconName, label);

	if (devNode.find("dvd")!=-1)
	{
		kdDebug(1219) << "=> dvd" << endl;
		return "dvd";
	}
	else
	{
		kdDebug(1219) << "=> cd" << endl;
		return "cd";
	}
}

static void restoreEmptyState(MediaList &list, const Medium *medium,
                              bool allowNotification)
{
	kdDebug(1219) << "restoreEmptyState(" << medium->id() << ")" << endl;

	QString id = medium->id();
	QString devNode = medium->deviceNode();
	QString mountPoint = medium->mountPoint();
	QString fsType = medium->fsType();
	bool mounted = medium->isMounted();

	QString mimeType, iconName, label;

	FstabBackend::guess(devNode, mountPoint, fsType, mounted,
	                    mimeType, iconName, label);

	list.changeMediumState(id, devNode, mountPoint, fsType, mounted,
	                       allowNotification, mimeType, iconName, label);
}


void LinuxCDPolling::applyType(DiscType type, const Medium *medium)
{
	kdDebug(1219) << "LinuxCDPolling::applyType(" << type << ", "
	          << medium->id() << ")" << endl;

	QString id = medium->id();
	QString dev = medium->deviceNode();
	
	bool notify = !m_excludeNotification.contains(id);
	m_excludeNotification.remove(id);
	
	switch (type)
	{
	case DiscType::Data:
		restoreEmptyState(m_mediaList, medium, notify);
		break;
	case DiscType::Audio:
	case DiscType::Mixed:
		m_mediaList.changeMediumState(id, "audiocd:/?device="+dev,
		                              notify, "media/audiocd");
		break;
	case DiscType::VCD:
		m_mediaList.changeMediumState(id, false, notify, "media/vcd");
		break;
	case DiscType::SVCD:
		m_mediaList.changeMediumState(id, false, notify, "media/svcd");
		break;
	case DiscType::DVD:
		m_mediaList.changeMediumState(id, false, notify, "media/dvdvideo");
		break;
	case DiscType::Blank:
		if (baseType(medium)=="dvd")
		{
			m_mediaList.changeMediumState(id, false,
			                              notify, "media/blankdvd");
		}
		else
		{
			m_mediaList.changeMediumState(id, false,
			                              notify, "media/blankcd");
		}
		break;
	case DiscType::None:
	case DiscType::Unknown:
	case DiscType::UnknownType:
		restoreEmptyState(m_mediaList, medium, false);
		break;
	}
}

DiscType LinuxCDPolling::identifyDiscType(const Q3CString &devNode,
                                          const DiscType &current)
{
	//kdDebug(1219) << "LinuxCDPolling::identifyDiscType("
	//          << devNode << ")" << endl;

	int fd;
	struct cdrom_tochdr th;

	// open the device
	fd = open(devNode, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return DiscType::Broken;

	switch (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT))
	{
	case CDS_DISC_OK:
	{
		if (current.isDisc())
		{
			close(fd);
			return current;
		}

		// see if we can read the disc's table of contents (TOC).
		if (ioctl(fd, CDROMREADTOCHDR, &th))
		{
			close(fd);
			return DiscType::Blank;
		}

		// read disc status info
		int status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);

		// release the device
		close(fd);

		switch (status)
		{
		case CDS_AUDIO:
			return DiscType::Audio;
		case CDS_DATA_1:
		case CDS_DATA_2:
			if (hasDirectory(devNode, "video_ts"))
			{
				return DiscType::DVD;
			}
			else if (hasDirectory(devNode, "vcd"))
			{
				return DiscType::VCD;
			}
			else if (hasDirectory(devNode, "svcd"))
			{
				return DiscType::SVCD;
			}
			else
			{
				return DiscType::Data;
			}
		case CDS_MIXED:
			return DiscType::Mixed;
		default:
			return DiscType::UnknownType;
		}
	}
	case CDS_NO_INFO:
		close(fd);
		return DiscType::Unknown;
	default:
		close(fd);
		return DiscType::None;
	}
}

bool LinuxCDPolling::hasDirectory(const Q3CString &devNode, const Q3CString &dir)
{
	bool ret = false; // return value
	int fd = 0; // file descriptor for drive
	unsigned short bs; // the discs block size
	unsigned short ts; // the path table size
	unsigned int tl; // the path table location (in blocks)
	unsigned char len_di = 0; // length of the directory name in current path table entry
	unsigned int parent = 0; // the number of the parent directory's path table entry
	char dirname[256]; // filename for the current path table entry
	int pos = 0; // our position into the path table
	int curr_record = 1; // the path table record we're on
	Q3CString fixed_directory = dir.upper(); // the uppercase version of the "directory" parameter

	// open the drive
	fd = open(devNode, O_RDONLY | O_NONBLOCK);
	if (fd == -1) return false;

	// read the block size
	lseek(fd, 0x8080, SEEK_CUR);
	if (read(fd, &bs, 2) != 2)
	{
		close(fd);
		return false;
	}
	if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
		bs = ((bs << 8) & 0xFF00) | ((bs >> 8) & 0xFF);

	// read in size of path table
	lseek(fd, 2, SEEK_CUR);
	if (read(fd, &ts, 2) != 2)
	{
		close(fd);
		return false;
	}
	if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
		ts = ((ts << 8) & 0xFF00) | ((ts >> 8) & 0xFF);

	// read in which block path table is in
	lseek(fd, 6, SEEK_CUR);
	if (read(fd, &tl, 4) != 4)
	{
		close(fd);
		return false;
	}
	if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
		tl = ((tl << 24) & 0xFF000000) | ((tl << 8) & 0xFF0000) |
		     ((tl >> 8) & 0xFF00) | ((tl >> 24) & 0xFF);

	// seek to the path table
	lseek(fd, bs * tl, SEEK_SET);

	// loop through the path table entries
	while (pos < ts)
	{
		// get the length of the filename of the current entry
		if (read(fd, &len_di, 1) != 1)
		{
			ret = false;
			break;
		}

		// get the record number of this entry's parent
		// i'm pretty sure that the 1st entry is always the top directory
		lseek(fd, 5, SEEK_CUR);
		if (read(fd, &parent, 2) != 2)
		{
			ret = false;
			break;
		}
		if (Q_BYTE_ORDER != Q_LITTLE_ENDIAN)
			parent = ((parent << 8) & 0xFF00) | ((parent >> 8) & 0xFF);

		// read the name
		if (read(fd, dirname, len_di) != len_di)
		{
			ret = false;
			break;
		}
		dirname[len_di] = 0;
		qstrcpy(dirname, Q3CString(dirname).upper());

		// if we found a folder that has the root as a parent, and the directory name matches
		// then return success
		if ((parent == 1) && (dirname == fixed_directory))
		{
			ret = true;
			break;
		}

		// all path table entries are padded to be even, so if this is an odd-length table, seek a byte to fix it
		if (len_di%2 == 1)
		{
			lseek(fd, 1, SEEK_CUR);
			pos++;
		}

		// update our position
		pos += 8 + len_di;
		curr_record++;
	}

	close(fd);
	return ret;
}


#include "linuxcdpolling.moc"
