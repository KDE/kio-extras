/* Name: nfsserver.h

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

#ifndef __INC_NFSSERVER_H__
#define __INC_NFSSERVER_H__

#include "common.h"
#include "qlist.h"
#include "treeitem.h"

class CNFSServerInfo
{
public:
	CNFSServerInfo() {}

	CNFSServerInfo& operator=(const CNFSServerInfo& other)
	{
		m_ServerName = (LPCSTR)other.m_ServerName
#ifdef QT_20
  .latin1()
#endif
    ;
		m_IP = (LPCSTR)other.m_IP
#ifdef QT_20
  .latin1()
#endif
    ;
		return *this;
	}

	QString m_ServerName;
	QString m_IP;
};

class CNFSServerItem : public CNetworkTreeItem, public CNFSServerInfo
{
public:
	CNFSServerItem(CListView *parent, CNFSServerInfo *pInfo) :
		CNetworkTreeItem(parent, NULL), CNFSServerInfo(*pInfo)
	{
		Init();
	}

	CNFSServerItem(CNetworkTreeItem *parent, CNFSServerInfo *pInfo) :
		CNetworkTreeItem(parent, NULL), CNFSServerInfo(*pInfo)
	{
		Init();
	}

	void Init()
	{
		m_nCredentialsIndex = -1;
		InitPixmap();
	}

	QTTEXTTYPE text(int column) const
	{
		switch (column)
		{
			case -1:
				return "Network";

			case 0:
				return (LPCSTR)m_ServerName
#ifdef QT_20
  .latin1()
#endif
        ;

			default:
				return "";
		}
	}

	QString FullName(BOOL /*bDoubleSlashes*/)
	{
		QString s;

		s.sprintf("nfs://%s", (LPCSTR)m_ServerName
#ifdef QT_20
  .latin1()
#endif
    );
		return s;
	}

	int CredentialsIndex();
	int m_nCredentialsIndex;

	CItemKind Kind() { return keNFSHostItem; }

	QPixmap *Pixmap()
	{
		return LoadPixmap(keServerIcon);
	}

	QPixmap *BigPixmap()
	{
		return LoadPixmap(keServerIconBig);
	}

	void Fill();
};

extern QList<CNFSServerInfo> gNFSHostList;

#endif /* __INC_NFSSERVER_H__ */


