/* Name: nfsroot.h

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

#ifndef __INC_NFSOOT_H__
#define __INC_NFSOOT_H__

#include "treeitem.h"

class CNFSNetworkItem : public CNetworkTreeItem
{
public:
	CNFSNetworkItem(CListView *parent) 
		: CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}
	
	CNFSNetworkItem(CListViewItem *parent) :
		CNetworkTreeItem(parent, NULL)
	{
	  InitPixmap();
	}

	QTTEXTTYPE text(int column) const;
  
  void Rescan();

	QString FullName(BOOL /*bDoubleSlashes*/)
  { 
    return QString(GetHiddenPrefix(knNFSSERVERS_HIDDEN_PREFIX)) + text(0);
  }

	virtual int CredentialsIndex() { return 0; }

	void Fill();

	//void Fill(CNFSNetworkItem *);	// Populate from another tree

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(keNFSRootIcon);
	}

	QPixmap *BigPixmap()
	{ 
		return LoadPixmap(keNFSRootIconBig);
	}
	
	CItemKind Kind()
	{ 
		return keNFSRootItem;
	}

	CListViewItem *FindAndExpand(LPCSTR Path);
	
	QTKEYTYPE key(int, bool) const
	{
		return "3";
	}
};

#endif /* __INC_NFSROOT_H__ */
