/* Name: msroot.h

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

#ifndef __INC_MSROOT_H__
#define __INC_MSROOT_H__

#include "treeitem.h"

class CMSWindowsNetworkItem : public CNetworkTreeItem
{
public:
	CMSWindowsNetworkItem(CListView *parent, CDetailLevel Level = keDetail_Folders) 
		: CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
		m_DetailLevel = Level;
	}
	
	CMSWindowsNetworkItem(CListViewItem *parent, CDetailLevel Level = keDetail_Folders) :
		CNetworkTreeItem(parent, NULL)
	{
	  InitPixmap();
		m_DetailLevel = Level;
	}

	QTTEXTTYPE text(int column) const
	{
		switch (column)
		{
			case -1:
				return "Network";
			
			case 0:
				return LoadString(knSTR_WINDOWS_NETWORK);
			
			default:
				return "";
		}
	}
	
	QString FullName(BOOL /*bDoubleSlashes*/)
  { 
    return QString(GetHiddenPrefix(knWORKGROUPS_HIDDEN_PREFIX)) + text(0);
  }

	virtual int CredentialsIndex() { return 0; }

	void Fill();

	void Fill(CMSWindowsNetworkItem *);	// Populate from another tree

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(keMSRootIcon);
	}

	QPixmap *BigPixmap()
	{ 
		return LoadPixmap(keMSRootIconBig);
	}
	
	CItemKind Kind()
	{ 
		return keMSRootItem;
	}

	CListViewItem *FindAndExpand(LPCSTR UNCPath);
	
	QTKEYTYPE key(int, bool) const
	{
		return "2";
	}

	CDetailLevel DetailLevel()
	{
		return m_DetailLevel;
	}

private:
	CDetailLevel m_DetailLevel;
};

#endif /* __INC_MSROOT_H__ */
