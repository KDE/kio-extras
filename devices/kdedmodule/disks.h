/*
 * disks.h
 *
 * Copyright (c) 2002 Joseph Wenninger <jowenn@kde.org>
 * Copyright (c) 1998 Michael Kropfberger <michael.kropfberger@gmx.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef __DISKS_H__
#define __DISKS_H__

#include <qobject.h>
#include <qstring.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qprogressbar.h>
#include <qfile.h>

#include <kio/global.h>
#include <kprogress.h>
#include <kprocess.h>
#include <klocale.h>

class DiskEntry : public QObject
{
  Q_OBJECT
public:
  DiskEntry(QObject *parent=0, const char *name=0);
  QString deviceName() const { return device; };
  QString realDeviceName() const { return realDevice; };
  QString mountPoint() const { return mountedOn; };
  QString mountOptions() const { return options; };
  bool mounted() const { return isMounted; }
  ino_t inode() const {return m_inode; };
  bool inodeType() const {return m_inodeType;};
  QString fsType() const { return type; };
  int kBSize() const { return size; };

  QString discType();
  QString niceDescription();

  QString prettyKBSize() const { return KIO::convertSizeFromKB(size); };
  int kBUsed() const { return used; };
  QString prettyKBUsed() const { return KIO::convertSizeFromKB(used); };
  int kBAvail() const  { return avail; };
  QString prettyKBAvail() const { return KIO::convertSizeFromKB(avail); };
  float percentFull() const;

signals:
  void deviceNameChanged();
  void mountPointChanged();
  void mountOptionsChanged();
  void fsTypeChanged();
  void mountedChanged();
  void kBSizeChanged();
  void kBUsedChanged();
  void kBAvailChanged();

public slots:
  void setDeviceName(const QString & deviceName);
  void setMountPoint(const QString & mountPoint);
  void setMountOptions(const QString & mountOptions);
  void setFsType(const QString & fsType);
  void setMounted(bool nowMounted);
  void setKBSize(int kb_size);
  void setKBUsed(int kb_used);
  void setKBAvail(int kb_avail);

private:
  void init();
  QString prettyPrint(int kBValue) const;

  QString     device,
	      realDevice,
              type,
              mountedOn,
      options;


  int         size,
              used,
              avail;       // ATTENTION: used+avail != size (clustersize!)

  bool        isMounted,
	      m_inodeType;

  ino_t	      m_inode;
};

#endif
