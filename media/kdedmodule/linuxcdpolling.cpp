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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* Some code of this file comes from kdeautorun */

#include "linuxcdpolling.h"

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
	    && m_type != UnknownType;
}

bool DiscType::isDisc() const
{
	return m_type != None
	    && m_type != Unknown;
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

LinuxCDPolling::LinuxCDPolling(MediaList &list)
	: QObject(), BackendBase(list)
{

}



DiscType LinuxCDPolling::identifyMimeType(const QCString &devNode)
{
	int fd;
	struct cdrom_tochdr th;

	// open the device
	fd = open(devNode, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return DiscType::Unknown;

	switch (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT))
	{
	case CDS_DISC_OK:
	{
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

bool LinuxCDPolling::hasDirectory(const QCString &devNode, const QCString &dir)
{
	bool ret = false; // return value
	int fd = 0; // file descriptor for drive
	unsigned short bs; // the discs block size
	unsigned short ts; // the path table size
	unsigned int tl; // the path table location (in blocks)
	unsigned int len_di = 0; // length of the directory name in current path table entry
	unsigned int parent = 0; // the number of the parent directory's path table entry
	char *dirname = 0; // filename for the current path table entry
	int pos = 0; // our position into the path table
	int curr_record = 1; // the path table record we're on
	QCString fixed_directory = dir.upper(); // the uppercase version of the "directory" parameter

	// open the drive
	fd = open(devNode, O_RDONLY | O_NONBLOCK);
	if (fd == -1) return false;

	// read the block size
	lseek(fd, 0x8080, SEEK_CUR);
	read(fd, &bs, 2);

	// read in size of path table
	lseek(fd, 2, SEEK_CUR);
	read(fd, &ts, 2);

	// read in which block path table is in
	lseek(fd, 6, SEEK_CUR);
	read(fd, &tl, 4);

	// seek to the path table
	lseek(fd, ((int)(bs) * tl), SEEK_SET);

	// loop through the path table entries
	while (pos < ts)
	{
		// get the length of the filename of the current entry
		read(fd, &len_di, 1);

		// get the record number of this entry's parent
		// i'm pretty sure that the 1st entry is always the top directory
		lseek(fd, 5, SEEK_CUR);
		read(fd, &parent, 2);

		// allocate and zero a string for the filename
		dirname = (char *)malloc(len_di+1);
		for (unsigned i=0; i<len_di+1; i++) dirname[i] = '\0';

		// read the name
		read(fd, dirname, len_di);
		qstrcpy(dirname, QCString(dirname).upper());

		// if we found a folder that has the root as a parent, and the directory name matches
		// then return success
		if ((parent == 1) && (dirname == fixed_directory))
		{	ret = true;
			free(dirname);
			break;
		}
		free(dirname);

		// all path table entries are padded to be even, so if this is an odd-length table, seek a byte to fix it
		if (len_di%2 == 1)
		{	lseek(fd, 1, SEEK_CUR);
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
