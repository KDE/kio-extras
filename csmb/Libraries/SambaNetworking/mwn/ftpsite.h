/* Name: ftpsite.h

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

#ifndef __INC_FTPSITE_H__
#define __INC_FTPSITE_H__

#include "ftpfile.h"
#include "treeitem.h"

class CFtpSiteItem : public CFTPFileContainer
{
public:
	CFtpSiteItem(CListView *parent, LPCSTR Url);
	CFtpSiteItem(CListViewItem *parent, LPCSTR Url);

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

	QString FullName(BOOL bDoubleSlashes);

	virtual int CredentialsIndex();

	QPixmap *Pixmap()
	{
		return LoadPixmap(keWorldIcon);
	}

	QPixmap *BigPixmap()
	{
		return LoadPixmap(keWorldIconBig);
	}

	CItemKind Kind()
	{
		return keFTPSiteItem;
	}

	virtual BOOL ItemAcceptsDrops()
	{
		return TRUE;
	}

	QTKEYTYPE key(int, bool) const
	{
		return "3";
	}

public:
	void SetCredentialsIndex(int nCredentialsIndex);
private:
	QString m_URL;
	void SetCredentialsIndex();
public:
	int m_nCredentialsIndex;
};

#endif /* __INC_FTPSITE_H__ */
