/* Name: webpage.h

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

#ifndef __INC_WEBPAGE_H__
#define __INC_WEBPAGE_H__

#include "treeitem.h"

class CWebPageItem : public CNetworkTreeItem
{
public:
	CWebPageItem(CListView *parent, LPCSTR Url)
		: CNetworkTreeItem(parent, NULL), m_URL(Url)
	{
		InitPixmap();
	}

	CWebPageItem(CListViewItem *parent, LPCSTR Url) :
		CNetworkTreeItem(parent, NULL), m_URL(Url)
	{
		InitPixmap();
	}

	QTTEXTTYPE text(int column) const
	{
		switch (column)
		{
			case -1:
				return "Network";

			case 0:
				return (LPCSTR)m_URL
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
		return m_URL;
	}

	virtual int CredentialsIndex()
	{ 
		return 0;
	}
	
	void Fill()
	{
	}

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(keHTMLFileIcon);
	}

	CItemKind Kind()
	{ 
		return keWebPageItem;
	}

	QTKEYTYPE key(int, bool) const
	{
		return "3";
	}

protected:
	QString m_URL;
};

#endif /* __INC_WEBPAGE_H__ */
