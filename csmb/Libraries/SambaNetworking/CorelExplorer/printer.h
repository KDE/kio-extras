/* Name: printer.h

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

#ifndef __INC_PRINTER_H__
#define __INC_PRINTER_H__

#include "treeitem.h"
#include <aps.h>

class CPrinterInfo
{
public:
  CPrinterInfo()
  {
  }

  CPrinterInfo(const CPrinterInfo &other) :
    m_Name((LPCSTR)other.m_Name
#ifdef QT_20
  .latin1()
#endif
    ),
    m_Location((LPCSTR)other.m_Location
#ifdef QT_20
  .latin1()
#endif
    ),
    m_Manufacturer((LPCSTR)other.m_Manufacturer
#ifdef QT_20
  .latin1()
#endif
    ),
    m_Model((LPCSTR)other.m_Model
#ifdef QT_20
  .latin1()
#endif
    )
  {
    m_ConnectionType = other.m_ConnectionType;
  }

  CPrinterInfo& operator=(const CPrinterInfo &other)
  {
    m_Name = other.m_Name;
    m_Location = other.m_Location;
    m_Manufacturer = other.m_Manufacturer;
    m_Model = other.m_Model;
    m_ConnectionType = other.m_ConnectionType;

    return *this;
  }

protected:
  QString m_Name;
  QString m_Location;
  QString m_Manufacturer;
  QString m_Model;
  Aps_ConnectionType m_ConnectionType;
};

class CPrinterItem : public CNetworkTreeItem, public CPrinterInfo
{
public:
	CPrinterItem(CListView *parent, LPCSTR name);
  CPrinterItem(CListView *parent, const CPrinterInfo& info);
	CPrinterItem(CListViewItem *parent, LPCSTR name);
  CPrinterItem(CListViewItem *parent, const CPrinterInfo &info);

  void Init();

	QTTEXTTYPE text(int column) const;

	QString FullName(BOOL bDoubleSlashes);

	virtual int CredentialsIndex() { return 0; }

	void Fill();

	QPixmap *Pixmap();
	QPixmap *BigPixmap();

	CItemKind Kind()
	{
		return keLocalPrinterItem;
	}

	BOOL ItemAcceptsDrops()
	{
		return TRUE;
	}

	BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst = NULL);
};

CSMBErrorCode PurgePrinter(LPCSTR sPrinterName);

#endif /* __INC_PRINTER_H__ */

