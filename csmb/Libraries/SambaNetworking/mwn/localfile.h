/* Name: localfile.h

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

#ifndef __INC_LOCALFILE_H__
#define __INC_LOCALFILE_H__

#include "smbfile.h"
#include <stdlib.h> // for atol()
#include "unistd.h"

////////////////////////////////////////////////////////////////////////////

class CLocalFileContainer : public CNetworkTreeItem
{
public:
	CLocalFileContainer(CListView *Parent, CListViewItem *pLogicalParent) :
		CNetworkTreeItem(Parent, pLogicalParent)
	{
		m_nNumExtraItems = 0;
	}

	CLocalFileContainer(CListViewItem *Parent, CListViewItem *pLogicalParent) :
		CNetworkTreeItem(Parent, pLogicalParent)
	{
		m_nNumExtraItems = 0;
	}

	void Fill();
	void Fill(CFileArray& list);
	void Rescan();

	BOOL CanCreateSubfolder();
	BOOL CreateSubfolder(QString& sFolderName);
	QString FullName(BOOL bDoubleSlashes);
	BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst = NULL);

	int DesiredRefreshDelay()
	{
		return 2000; // 2 seconds because it's the local file system :-)
	}

	int CredentialsIndex() { return 0; }
	BOOL CanEditPermissions();

private:
	time_t m_LastRefreshTime;
public:
	int m_nNumExtraItems;
};

////////////////////////////////////////////////////////////////////////////

class CLocalFileItem : public CLocalFileContainer, public CSMBFileInfo
{
public:
	CLocalFileItem(CListView *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CLocalFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	CLocalFileItem(CListViewItem *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CLocalFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	QString GetTip(int nType = 0);

	void Init();

	size_t GetFileSize() const
	{
		return (size_t)atol((LPCSTR)m_FileSize.latin1()

    );
	}

	QTTEXTTYPE text(int column) const;

	QTKEYTYPE key(int nColumn, bool ascending) const;

	QPixmap *Pixmap()
	{
		return Pixmap(FALSE);
	}

	QPixmap *BigPixmap()
	{
		return Pixmap(TRUE);
	}

	QPixmap *Pixmap(BOOL bIsBig);

	CItemKind Kind() { return keLocalFileItem; }

	virtual BOOL ItemAcceptsDrops()
	{
		return IsFolder() && !access((LPCSTR)FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
    , W_OK);
	}

	BOOL CanEditLabel();
	CSMBErrorCode Rename(LPCSTR sNewLabel);

	void setup();
};

////////////////////////////////////////////////////////////////////////////

void CheckChangedFromItem(CListViewItem *pItem);

////////////////////////////////////////////////////////////////////////////

#endif /* __INC_LOCALFILE_H__ */
