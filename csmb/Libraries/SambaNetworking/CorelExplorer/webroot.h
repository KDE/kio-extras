 /* Name: webroot.h

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

#ifndef __INC_WEBROOT_H__
#define __INC_WEBROOT_H__

#include "treeitem.h"

class CWebRootItem : public CNetworkTreeItem
{
public:
	CWebRootItem(CListView *parent) 
		: CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}
	
	CWebRootItem(CListViewItem *parent) :
		CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}

	QTTEXTTYPE text(int nColumn) const
	{
		switch (nColumn)
		{
			case -1:
				return "Network";
			
			case 0:
				return "Web Browser";
			
			default:
				return "";
		}
	}

	void setOpen(bool bOpen)
	{
		CNetworkTreeItem::setOpen(bOpen);
	}
	
	QString FullName(BOOL /*bDoubleSlashes*/)
	{ 
		return QString("");
	}

	virtual int CredentialsIndex()
	{ 
		return 0;
	}
	
	void Fill()
	{ 
		SetExpansionStatus(keExpansionComplete);
	}

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(keWorldIcon);
	}

	CItemKind Kind()
	{ 
		return keWebRootItem;
	}

	QTKEYTYPE key(int, bool) const
	{
		return "3";
	}
};

#endif /* __INC_WEBROOT_H__ */
