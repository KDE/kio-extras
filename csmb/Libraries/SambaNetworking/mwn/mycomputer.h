/* Name: mycomputer.h

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

#ifndef __INC_MYCOMPUTER_H__
#define __INC_MYCOMPUTER_H__

#include "treeitem.h"
#include <qtimer.h>
#include <sys/types.h>

class CMyComputerItem : public QObject, public CNetworkTreeItem
{
	Q_OBJECT
public:
	CMyComputerItem(CListView *parent) : CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}
	
	CMyComputerItem(CListViewItem *parent) :
		CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}

	QTTEXTTYPE text(int column) const;
	
	QString FullName(BOOL /*bDoubleSlashes*/)
	{ 
		return text(0);
	}

	virtual int CredentialsIndex() { return 0; }

	void Fill();

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(keMyComputerIcon);
	}

	QPixmap *BigPixmap()
	{ 
		return LoadPixmap(keMyComputerIconBig);
	}
	
	CItemKind Kind()
	{ 
		return keMyComputerItem;
	}
	
	QTKEYTYPE key(int, bool) const
	{
		return "1";
	}
	
  BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst = NULL);
	
	int DesiredRefreshDelay()
	{
		return 300;
	}

	CListViewItem* FindAndExpand(LPCSTR Path);
	void AddFileSystem(LPCSTR Name);
	void UpdateFileSystemInfo();
public slots:
	void CheckRefresh();
private:
	QTimer m_Timer;
	time_t m_LastUpdateTime;
};

#endif /* __INC_MYCOMPUTER_H__ */
