/* Name: nfsserver.cpp

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
#include "nfsserver.h"
#include "nfsshare.h"
#include "nfsutils.h"
#include "qmessagebox.h"
#include "PasswordDlg.h"

QList<CNFSServerInfo> gNFSHostList;

int CNFSServerItem::CredentialsIndex()
{
	CNetworkTreeItem *pParent = (CNetworkTreeItem*)parent();
    
	if (m_nCredentialsIndex == -1)													  
		return pParent == NULL ? 0 : pParent->CredentialsIndex();
  else	
		return m_nCredentialsIndex;
}

void CNFSServerItem::Fill()
{
	SetExpansionStatus(keExpanding);
	
	CNFSShareArray list;

	if (keSuccess == GetNFSShareList(m_ServerName,  &list))
	{
		int i;

		for (i=0; i < list.count(); i++)
			new CNFSShareItem(this, this, &list[i]);

		SetExpansionStatus(keExpansionComplete);
	}
	
	gTreeExpansionNotifier.Fire(this);
}
	

