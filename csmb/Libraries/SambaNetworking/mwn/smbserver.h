/* Name: smbserver.h

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

#ifndef __INC_SMBSERVER_H__
#define __INC_SMBSERVER_H__

#include "common.h"
#include "array.h"
#include "treeitem.h"

class CSMBServerInfo
{
public:
	CSMBServerInfo() {}

	CSMBServerInfo& operator=(const CSMBServerInfo& other)
	{
		m_ServerName = (LPCSTR)other.m_ServerName
#ifdef QT_20
  .latin1()
#endif
                                              ;
		m_Comment = (LPCSTR)other.m_Comment
#ifdef QT_20
  .latin1()
#endif
                                        ;
		return *this;
	}

/*private:*/
	QString m_ServerName;
	QString m_Comment;
};

class CServerArray : public CVector<CSMBServerInfo,CSMBServerInfo&>
{
public:
    void Print();
};

class CServerItem : public CNetworkTreeItem, public CSMBServerInfo
{
public:
	CServerItem(CListView *parent, CListViewItem *pLogicalParent, CSMBServerInfo *pInfo) :
		CNetworkTreeItem(parent, pLogicalParent), CSMBServerInfo(*pInfo)
	{
		Init();
	}

	CServerItem(CNetworkTreeItem *parent, CListViewItem *pLogicalParent, CSMBServerInfo *pInfo) :
		CNetworkTreeItem(parent, pLogicalParent), CSMBServerInfo(*pInfo)
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
			default:
				return (LPCSTR)m_ServerName
#ifdef QT_20
  .latin1()
#endif
                                    ;

			case 1:
				return (LPCSTR)m_Comment
#ifdef QT_20
  .latin1()
#endif
                                ;
		}
	}

	QString FullName(BOOL bDoubleSlashes)
	{
		QString s;
		LPCSTR Slash = bDoubleSlashes ? "\\\\" : "\\";

		s.sprintf("%s%s%s", Slash, Slash, (LPCSTR)m_ServerName
#ifdef QT_20
  .latin1()
#endif
    );
		return s;
	}

	int CredentialsIndex();
	int m_nCredentialsIndex;

	CItemKind Kind()
	{
		return keServerItem;
	}

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

#endif /* __INC_SMBSERVER_H__ */


