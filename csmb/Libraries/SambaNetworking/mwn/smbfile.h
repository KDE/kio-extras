/* Name: smbfile.h

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

#ifndef __INC_SMBFILE_H__
#define __INC_SMBFILE_H__

#include "common.h"
#include "array.h"
#include "treeitem.h"
#include <stdlib.h> // for atol()
#include <sys/types.h>
#include <sys/stat.h>

class CSMBFileInfo
{
public:
	CSMBFileInfo() : m_FileName(""), m_FileAttributes(""), m_FileSize(""), m_Owner(""), m_Group(""), m_TargetName("")
	{
		m_FileDate = 0;
		m_TargetMode = 0;
	}

	CSMBFileInfo& operator=(const CSMBFileInfo& other)
	{
		m_FileName = (LPCSTR)other.m_FileName
#ifdef QT_20
				.latin1()
#endif
                ;
		m_FileAttributes = (LPCSTR)other.m_FileAttributes
#ifdef QT_20
				.latin1()
#endif
                ;
		m_FileSize = (LPCSTR)other.m_FileSize
#ifdef QT_20
				.latin1()
#endif
                ;
		m_FileDate = other.m_FileDate;
		m_Owner = other.m_Owner;
		m_Group = other.m_Group;
		m_TargetMode = other.m_TargetMode;
		m_TargetName = other.m_TargetName;
		return *this;
	}

	BOOL IsFolder() const
	{
		return m_FileAttributes[0] == 'd' || m_FileAttributes.contains('D') > 0 ||
			(IsLink() && S_ISDIR(m_TargetMode));
	}

	BOOL IsLink() const
	{
		return m_FileAttributes[0] == 'l';
	}

/*private:
*/
	QString m_FileName;
	QString m_FileAttributes;
	QString m_FileSize;
	time_t m_FileDate;
	QString m_Owner;
	QString m_Group;
	mode_t m_TargetMode;
	QString m_TargetName;
};

class CFileArray : public CVector<CSMBFileInfo,CSMBFileInfo&>
{
public:
    void Print();
};

class CNetworkFileContainer : public CRemoteFileContainer
{
public:
	CNetworkFileContainer(CListView *Parent, CListViewItem *pLogicalParent) :
		CRemoteFileContainer(Parent, pLogicalParent)
	{
	}

	CNetworkFileContainer(CListViewItem *Parent, CListViewItem *pLogicalParent) :
		CRemoteFileContainer(Parent, pLogicalParent)
	{
	}

	void Fill();
	void Fill(CFileArray& list);

	QString FullName(BOOL bDoubleSlashes);
  BOOL CanCreateSubfolder();
  BOOL CreateSubfolder(QString& NewFolderName);
};

class CFileItem : public CNetworkFileContainer, public CSMBFileInfo
{
public:
	CFileItem(CListView *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CNetworkFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	CFileItem(CListViewItem *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CNetworkFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	void Init();

	size_t GetFileSize() const
	{
		return (size_t)atol(m_FileSize
#ifdef QT_20
				.latin1()
#endif
                );
	}

	QTTEXTTYPE text(int column) const;

	QTKEYTYPE key(int nColumn, bool /*ascending*/) const
	{
		static QString s;

		if (nColumn == 1) /* Size */
		{
			if (IsFolder())
				s = QString("\1") + text(0);
			else
				s.sprintf("%.10ld", atol((LPCSTR)m_FileSize
#ifdef QT_20
				.latin1()
#endif
                                ));
		}
		else
		{
			if (nColumn == 3)
				s.sprintf("%.10ld", m_FileDate);
			else
			{
				s = IsFolder() ? "\1" : "";
				s += text(nColumn);
			}
		}

		return (QTKEYTYPE)s;
	}

	virtual BOOL ItemAcceptsDrops()
	{
		return IsFolder();
	}

	QPixmap *Pixmap()
	{
		return Pixmap(FALSE);
	}

	QPixmap *BigPixmap()
	{
		return Pixmap(TRUE);
	}

	QPixmap *Pixmap(BOOL bIsBig);
	void setup();

	CItemKind Kind() { return keFileItem; }

	BOOL CanEditLabel()
	{
		return TRUE;
	}

  CSMBErrorCode Rename(LPCSTR /*sNewLabel*/);
};

#endif /* __INC_SMBFILE_H__ */
