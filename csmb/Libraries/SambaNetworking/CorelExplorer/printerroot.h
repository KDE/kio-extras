/* Name: printroot.h

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

#ifndef __INC_PRINTERROOT_H__
#define __INC_PRINTERROOT_H__

#include "treeitem.h"

class CPrinterRootItem : public QObject, public CNetworkTreeItem
{
	Q_OBJECT
public:
	CPrinterRootItem(CListView *parent);
	CPrinterRootItem(CListViewItem *parent);

	QTTEXTTYPE text(int column) const;
  
	QString FullName(BOOL /*bDoubleSlashes*/)
  { 
    return QString(GetHiddenPrefix(knPRINTERS_HIDDEN_PREFIX)) + text(0);
  }

	virtual int CredentialsIndex() { return 0; }

	void Fill();

	QPixmap *Pixmap()
	{ 
		return LoadPixmap(kePrintersIcon);
	}

	QPixmap *BigPixmap()
	{ 
		return LoadPixmap(kePrintersIconBig);
	}
	
	CItemKind Kind()
	{ 
		return kePrinterRootItem;
	}

	CListViewItem *FindAndExpand(LPCSTR Path);
	
	QTKEYTYPE key(int, bool) const
	{
		return "4";
	}

	BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst = NULL);

public slots:
	void CheckRefresh();
};

#endif /* __INC_PRINTERROOT_H__ */
