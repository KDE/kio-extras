/* Name: smbworkgroup.h

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

#ifndef __INC_SMBWORKGROUP_H__
#define __INC_SMBWORKGROUP_H__

#include "common.h"
#include "array.h"
#include "treeitem.h"

class CSMBWorkgroupInfo
{
public:
	CSMBWorkgroupInfo& operator=(const CSMBWorkgroupInfo& other)
	{
		m_WorkgroupName = (LPCSTR)other.m_WorkgroupName
#ifdef QT_20
  .latin1()
#endif
                                                    ;
		m_MasterName = (LPCSTR)other.m_MasterName
#ifdef QT_20
  .latin1()
#endif
                                             ;
		return *this;
	}

/*private:*/
	QString m_WorkgroupName;
	QString m_MasterName;
};

class CWorkgroupArray : public CVector<CSMBWorkgroupInfo,CSMBWorkgroupInfo&>
{
public:
	void Print();
};

extern CWorkgroupArray gWorkgroupList; /* Global workgroup list */

class CWorkgroupItem : public CNetworkTreeItem, public CSMBWorkgroupInfo
{
public:
	CWorkgroupItem(CListView *parent, CSMBWorkgroupInfo *pInfo) :
		CNetworkTreeItem(parent, NULL), CSMBWorkgroupInfo(*pInfo)
	{
		Init();
	}

	CWorkgroupItem(CNetworkTreeItem *parent, CSMBWorkgroupInfo *pInfo) :
		CNetworkTreeItem(parent, NULL), CSMBWorkgroupInfo(*pInfo)
	{
		Init();
	}

	void Init()
	{
		m_nCredentialsIndex = 0;
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
				return (LPCSTR)m_WorkgroupName
#ifdef QT_20
  .latin1()
#endif
        ;

			case 1:
				return "";
		}
	}

	QString FullName(BOOL /*bDoubleSlashes*/)
	{
		return QString(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX)) + m_WorkgroupName;
	}

	int CredentialsIndex();
	int m_nCredentialsIndex;

	QPixmap *Pixmap()
	{
		return LoadPixmap(keWorkgroupIcon);
	}

	QPixmap *BigPixmap()
	{
		return LoadPixmap(keWorkgroupIconBig);
	}

	CItemKind Kind()
	{
		return keWorkgroupItem;
	}

	void Fill();
};

#endif /* __INC_SMBWORKGROUP_H__ */
