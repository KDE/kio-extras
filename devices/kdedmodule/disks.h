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
  QString uniqueIdentifier() const { return m_identifier; };
  QString deviceName() const { return device; };
  QString realDeviceName() const { return realDevice; };
  QString mountPoint() const { return mountedOn; };
  bool mounted() const { return isMounted; }
  ino_t inode() const {return m_inode; };
  bool inodeType() const {return m_inodeType;};
  QString fsType() const { return type; };
  bool old();
  
  QString discType();
  QString niceDescription();
  QString userDescription() {return m_userDescription;};

public slots:
  void setUniqueIdentifier(const QString & uniqueIdentifier);
  void setDeviceName(const QString & deviceName);
  void setMountPoint(const QString & mountPoint);
  void setFsType(const QString & fsType);
  void setMounted(bool nowMounted);
  void setOld(bool);
  void setUserDescription(const QString & userDescription);

private:
  void init();
  QString prettyPrint(int kBValue) const;

  QString     m_identifier;
  QString     device;
  QString     realDevice;
  QString     type;
  QString     mountedOn;

  bool        isMounted;
  bool        m_inodeType;

  ino_t	      m_inode;
  
  bool        isOld;

  QString     m_userDescription;
};

#endif
