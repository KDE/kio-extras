/* Name: filesystem.h

   Description: This file is a part of the libmwn library.

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

#ifndef __INC_FILESYSTEM_H__
#define __INC_FILESYSTEM_H__

#include "common.h"
#include "array.h"
#include "localfile.h"
#include <stdlib.h> // for atol()

typedef enum
{
	keLocalDrive,
	keNetworkDrive,
	keCdromDrive,
	keFloppyDrive,
  keZIPDrive,
	keUnknownDrive
} CDriveType;

class CFileSystemInfo
{
public:
	CFileSystemInfo() :
		m_Name(""), m_Type(""), m_TotalK(""), m_UsedK(""), m_AvailableK(""), m_PercentUsed(""), m_MountedOn("")
	{
	}

	CFileSystemInfo& operator=(const CFileSystemInfo& other)
	{
		m_Name = other.m_Name;
		m_Type = other.m_Type;
		m_TotalK = other.m_TotalK;
		m_UsedK = other.m_UsedK;
		m_AvailableK = other.m_AvailableK;
		m_PercentUsed = other.m_PercentUsed;
		m_MountedOn = other.m_MountedOn;
		m_DriveType = other.m_DriveType;
		m_bIsReadOnly = other.m_bIsReadOnly;
		return *this;
	}

  QPixmap *Pixmap(BOOL bIsBig);

/*private:*/
	QString m_Name;
	QString m_Type;
	QString m_TotalK;
	QString m_UsedK;
	QString m_AvailableK;
	QString m_PercentUsed;
	QString m_MountedOn;
	CDriveType m_DriveType;
	BOOL m_bIsReadOnly;
};

class CFileSystemArray : public CVector<CFileSystemInfo,CFileSystemInfo&>
{
public:
	int Find(LPCSTR Name)
	{
		int i;

		for (i=count()-1; i >= 0; i--)
		{
			if ((*this)[i].m_MountedOn == Name)
				break;
		}

		return i;
	}
};

class CFileSystemItem : public CLocalFileContainer, public CFileSystemInfo
{
public:
	CFileSystemItem(CListView *parent, CFileSystemInfo *pInfo, CListViewItem *pLogicalParent) :
		CLocalFileContainer(parent, pLogicalParent)
	{
		Init(pInfo);
	}

	CFileSystemItem(CNetworkTreeItem *parent, CFileSystemInfo *pInfo, CListViewItem *pLogicalParent) :
		CLocalFileContainer(parent, pLogicalParent)
	{
		Init(pInfo);
	}

	void Init(CFileSystemInfo *pInfo)
	{
		m_Name = pInfo->m_Name;
		m_Type = pInfo->m_Type;
		m_TotalK = pInfo->m_TotalK;
		m_UsedK = pInfo->m_UsedK;
		m_AvailableK = pInfo->m_AvailableK;
		m_PercentUsed = pInfo->m_PercentUsed;
		m_MountedOn = pInfo->m_MountedOn;
		m_bIsReadOnly = pInfo->m_bIsReadOnly;

		GuessDriveType();
		InitPixmap();

		setText(2, SizeBytesFormat((double)atol((LPCSTR)m_TotalK
#ifdef QT_20
  .latin1()
#endif
    ) * 1024.));
		setText(3, SizeBytesFormat((double)atol((LPCSTR)m_AvailableK
#ifdef QT_20
  .latin1()
#endif
    ) * 1024.));
	}

	QTTEXTTYPE text(int column) const
	{
		switch (column)
		{
			case -1:
				return "MyComputer";

			case 0:
				return (LPCSTR)m_MountedOn
#ifdef QT_20
  .latin1()
#endif
        ;
			case 1:
			default:
				return (LPCSTR)m_Name
#ifdef QT_20
  .latin1()
#endif
        ;
			case 2:
				return CListViewItem::text(2);
			case 3:
				return CListViewItem::text(3);
		}
	}

	QPixmap *Pixmap()
	{
		return Pixmap(FALSE);
	}

	QPixmap *BigPixmap()
	{
		return Pixmap(TRUE);
	}

	CItemKind Kind()
	{
		return keFileSystemItem;
	}

  QPixmap *Pixmap(BOOL bIsBig);

	virtual BOOL ItemAcceptsDrops()
	{
		return
      !m_bIsReadOnly &&
      !access((LPCSTR)m_MountedOn
#ifdef QT_20
  .latin1()
#endif
      , W_OK);
	}

private:
	void GuessDriveType();
};

void GetFileSystemList(CFileSystemArray *pFileSystemList);
BOOL IsReadOnlyFileSystemPath(LPCSTR Path);

extern CFileSystemArray gFileSystemList;
extern time_t gFileSystemListTimestamp;

#endif /* __INC_FILESYSTEM_H__ */
