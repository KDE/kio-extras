/* Name: nfsshare.cpp

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


#include <stdio.h>
#include "nfsshare.h"
#include "nfsutils.h"

CNFSShareItem::CNFSShareItem(CListView *parent, CListViewItem *pLogicalParent, CNFSShareInfo *pInfo) : 
	CNetworkTreeItem(parent, pLogicalParent), CNFSShareInfo(*pInfo)
{
	Init();
	setExpandable(FALSE);
	SetPixmapID(keClosedFolderIcon, FALSE, FALSE);
}
	
CNFSShareItem::CNFSShareItem(CNetworkTreeItem *parent, CListViewItem *pLogicalParent, CNFSShareInfo *pInfo) :
	CNetworkTreeItem(parent, pLogicalParent), CNFSShareInfo(*pInfo)
{
	Init();
	setExpandable(FALSE);
	SetPixmapID(keClosedFolderIcon, FALSE, FALSE);
}

QTTEXTTYPE CNFSShareItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network"; // Internal ID, no localization please
		
		case 0:
		default:
			return (LPCSTR)m_ShareName;
		case 1:
			return (LPCSTR)m_ShareType;
	}
}

QPixmap *CNFSShareItem::Pixmap(BOOL bIsBig)
{ 
	return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon);
}

QString CNFSShareItem::FullName(BOOL bDoubleSlashes)
{
	QString s("nfs://");
	
	s += text(0);
	
	return s;
}


void CNFSShareItem::Fill()
{
	setExpandable(FALSE);
	SetExpansionStatus(keExpansionComplete);
	gTreeExpansionNotifier.Fire(this);
}

int CNFSShareItem::CredentialsIndex()
{
	return 0;
}

