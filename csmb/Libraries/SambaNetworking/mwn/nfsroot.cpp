/* Name: nfsroot.cpp

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

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "nfsroot.h"
#include "nfsserver.h"
#include "nfsshare.h"
#include "nfsutils.h"

////////////////////////////////////////////////////////////////////////////

void CNFSNetworkItem::Rescan()
{
  gNFSHostList.clear();
  
  if (!isOpen())
    SetExpansionStatus(keNotExpanded);

  CNetworkTreeItem::Rescan();
}

////////////////////////////////////////////////////////////////////////////

void CNFSNetworkItem::Fill()
{
	SetExpansionStatus(keExpanding);
	
	if (gNFSHostList.count() == 0)
	{
		GetNFSHostList(this);
	}
	else
	{
	
		QListIterator<CNFSServerInfo> it(gNFSHostList);
	
		for (it.toFirst(); it.current() != NULL;++it)
		{
			new CNFSServerItem(this, it.current());
		}
	}
	 
	SetExpansionStatus(keExpansionComplete);
	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CNFSNetworkItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network";
		
		case 0:
			return LoadString(knSTR_NFS_NETWORK);
		
		default:
			return "";
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *CNFSNetworkItem::FindAndExpand(LPCSTR Path)
{
	if (!isOpen())
		setOpen(TRUE);
	
	if (!strnicmp(Path, "nfs://", 6))
			Path += 6;

	QString Server = ExtractWord(Path, ":");
	QString Share = ExtractTail(Path, ":");

	CListViewItem *child;

	for (child = firstChild(); child != NULL; child = child->nextSibling())
	{
		if (Server == child->text(0))
		{
			if (!child->isOpen())
				child->setOpen(TRUE);
			
			if (Share.isEmpty())
				return child;

			CListViewItem *subchild;

			for (subchild=child->firstChild(); subchild != NULL; subchild=subchild->nextSibling())
			{
				QString s = subchild->text(0);
				int nIndex = s.findRev(':');
				
				if (-1 != nIndex)
				{
					s = s.mid(nIndex+1, s.length());
				}

				if (!strcmp(s, Share))
				{
					return subchild;
				}
      }

			return child;
		}
	}

	CNFSShareArray list;
	CSMBErrorCode retcode = GetNFSShareList(Server, &list);

	if (keSuccess == retcode)																								  
	{
		CNFSServerInfo *pInfo = new CNFSServerInfo;
		
		pInfo->m_ServerName = Server;
		pInfo->m_IP = Server;
		
		gNFSHostList.append(pInfo);

    child = new CNFSServerItem(this, pInfo);

		return child;
	}

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

