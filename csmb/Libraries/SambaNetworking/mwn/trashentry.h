/* Name: trashentry.h

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

#ifndef __INC_TRASHENTRY_H__
#define __INC_TRASHENTRY_H__

#include "treeitem.h"

class CTrashEntryInfo
{
public:
	CTrashEntryInfo() :
    m_FileName(""),
    m_OriginalLocation("")
	{
		m_DateDeleted = 0;
    m_OriginalCreationDate = 0;
    m_OriginalModifyDate = 0;
    m_OriginalFileMode = 0;
    m_FileSize = 0;
	}

	CTrashEntryInfo& operator=(const CTrashEntryInfo& other)
	{
		m_FileName = (LPCSTR)other.m_FileName
#ifdef QT_20
  .latin1()
#endif
                                          ;
		m_OriginalLocation = (LPCSTR)other.m_OriginalLocation
#ifdef QT_20
  .latin1()
#endif
                                                          ;
		m_DateDeleted = other.m_DateDeleted;
    m_OriginalCreationDate = other.m_OriginalCreationDate;
    m_OriginalModifyDate = other.m_OriginalModifyDate;
    m_OriginalFileMode = other.m_OriginalFileMode;
    m_FileSize = other.m_FileSize;

    return *this;
	}

	BOOL IsFolder() const
	{
		return S_ISDIR(m_OriginalFileMode); //FALSE;
	}

	BOOL IsLink() const
	{
		return FALSE;
	}

/*private:
*/
	QString m_FileName;
  QString m_OriginalLocation;
  time_t m_DateDeleted;
  time_t m_OriginalCreationDate;
  time_t m_OriginalModifyDate;
  mode_t m_OriginalFileMode;
  off_t m_FileSize;
};

typedef CVector<CTrashEntryInfo,CTrashEntryInfo&> CTrashEntryArray;

class CTrashEntryItem : public CNetworkTreeItem, public CTrashEntryInfo
{
public:
	CTrashEntryItem(CListView *parent, CListViewItem *pLogicalParent, CTrashEntryInfo *pInfo) :
		CNetworkTreeItem(parent, pLogicalParent), CTrashEntryInfo(*pInfo)
	{
		Init();
	}

	CTrashEntryItem(CListViewItem *parent, CListViewItem *pLogicalParent, CTrashEntryInfo *pInfo) :
		CNetworkTreeItem(parent, pLogicalParent), CTrashEntryInfo(*pInfo)
	{
		Init();
	}

	void Init();

	size_t GetFileSize() const;

  QTTEXTTYPE text(int column) const;

	QTKEYTYPE key(int nColumn, bool ascending) const;

	virtual BOOL ItemAcceptsDrops()
	{
		return FALSE;
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
	//void setup();

  int DesiredRefreshDelay()
	{
		return 2000; // 2 seconds because it's the local file system :-)
	}

  void Fill()
  {
  }

  QString FullName(BOOL)
  {
    return m_FileName;
  }

	int CredentialsIndex() { return 0; }

	CItemKind Kind() { return keTrashEntryItem; }
};

BOOL GetTrashEntryList(LPCSTR Path, CTrashEntryArray& list);
BOOL CheckTrashFolder(QString Path, time_t SinceWhen, int nOldChildCount);
void EmptyTrash();
BOOL IsTrashEmpty();

#endif /* __INC_TRASHENTRY_H__ */

