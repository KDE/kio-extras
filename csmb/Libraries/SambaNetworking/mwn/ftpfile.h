/* Name: ftpfile.h

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

#ifndef __INC_FTPFILE_H__
#define __INC_FTPFILE_H__

#include "common.h"
#include "array.h"
#include "treeitem.h"
#include <stdlib.h> // for atol()

#include "smbfile.h"

class CFTPFileContainer : public CRemoteFileContainer
{
public:
	CFTPFileContainer(CListView *Parent, CListViewItem *pLogicalParent) :
		CRemoteFileContainer(Parent, pLogicalParent)
	{
		m_bRefreshForced = FALSE;
	}

	CFTPFileContainer(CListViewItem *Parent, CListViewItem *pLogicalParent) :
		CRemoteFileContainer(Parent, pLogicalParent)
	{
		m_bRefreshForced = FALSE;
	}

	CSMBErrorCode GetFTPFileList(LPCSTR Url, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles);

	void Fill();
	void Fill(CFileArray& list);

	BOOL CanCreateSubfolder();
	BOOL CreateSubfolder(QString& sFolderName);

  QString FullName(BOOL bDoubleSlashes);

	BOOL ContentsChanged(time_t /*SinceWhen*/, 
											 int /*nOldChildCount*/, 
											 CListViewItem * /*pOldFirst*/)
	{
		if (m_bRefreshForced)
		{
			m_bRefreshForced = FALSE;
			return TRUE;
		}

		return FALSE;
	}

	BOOL m_bRefreshForced;
};

class CFTPFileItem : public CFTPFileContainer, public CSMBFileInfo
{
public:
	CFTPFileItem(CListView *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CFTPFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	CFTPFileItem(CListViewItem *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
		CFTPFileContainer(parent, pLogicalParent), CSMBFileInfo(*pInfo)
	{
		Init();
	}

	QString GetTip();

	void Init();

	size_t GetFileSize() const
	{
		return (size_t)atol((LPCSTR)m_FileSize
#ifdef QT_20
  .latin1()
#endif
    );
	}

	virtual BOOL ItemAcceptsDrops()
	{
		return IsFolder();
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

	void setup();

	BOOL CanEditLabel()
	{
		return TRUE;
	}

	CSMBErrorCode Rename(LPCSTR /*sNewLabel*/);

	CItemKind Kind() { return keFTPFileItem; }
};

CSMBErrorCode GetFTPFileList(LPCSTR Url, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles);

#endif /* __INC_FTPFILE_H__ */
