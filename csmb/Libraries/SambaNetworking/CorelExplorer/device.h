/* Name: device.h

   Description: This file is a part of the Corel File Manager application.

   Author:	Oleg Noskov (olegn@corel.com)

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

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#ifndef __INC_DEVICE_H__
#define __INC_DEVICE_H__

#include "localfile.h"
#include "filesystem.h"

class CDeviceInfo : public CFileSystemInfo
{
public:
	CDeviceInfo() :
    m_DisplayName(""),
		m_Device(""),
    m_Driver(""),
    m_Model(""),
    m_Manufacturer(""),
    m_Description(""),
    m_AutoMountLocation("")
	{
	}

	CDeviceInfo& operator=(const CDeviceInfo& other)
	{
		*((CFileSystemInfo *)this) = *((CFileSystemInfo *)&other);
    m_DisplayName = other.m_DisplayName;
    m_Device = other.m_Device;
		m_Driver = other.m_Driver;
		m_Model = other.m_Model;
		m_Manufacturer = other.m_Manufacturer;
    m_AutoMountLocation = other.m_AutoMountLocation;
		return *this;
	}

/*private:*/
  QString m_DisplayName; // display name like "CD-ROM 2"
	QString m_Device; // something like /dev/scd0
	QString m_Driver; // just for information
	QString m_Model;  // just for information
	QString m_Manufacturer; // not available for all devices
  QString m_Description;
  //QString m_MountedOn; // where mounted
  QString m_AutoMountLocation; // where auto-mount will mount it.
};

class CDeviceItem : public QObject, public CLocalFileContainer, public CDeviceInfo
{
  Q_OBJECT
public:
	CDeviceItem(CListView *parent, CDeviceInfo *pInfo) :
    CLocalFileContainer(parent, NULL)
	{
		Init(pInfo);
	}

	CDeviceItem(CListViewItem *parent, CDeviceInfo *pInfo) :
		CLocalFileContainer(parent, NULL)
	{
		Init(pInfo);
	}

	void Init(CDeviceInfo *pInfo);

	QTTEXTTYPE text(int column) const;

	QPixmap *Pixmap()
	{
		return Pixmap(FALSE);
	}

	QPixmap *BigPixmap()
	{
		return Pixmap(TRUE);
	}

	QPixmap *Pixmap(BOOL bIsBig)
  {
    return CFileSystemInfo::Pixmap(bIsBig);
  }

	CItemKind Kind()
	{
		return keDeviceItem;
	}

	BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst);

  virtual BOOL ItemAcceptsDrops()
	{
		return
      !m_MountedOn.isEmpty() &&
      !m_bIsReadOnly &&
      !access((LPCSTR)m_MountedOn
#ifdef QT_20
  .latin1()
#endif
      , W_OK);
	}

  QString FullName(BOOL bDoubleSlashes);
  void Fill();
  void CheckRefresh();
  void AccessDevice();

public slots:
  void OnMountListChanged();
private:
  time_t m_LastUpdateTime;
};

#endif /* __INC_DEVICE_H__ */
