/* Name: smbshare.cpp

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
#include "smbshare.h"
#include "smbfile.h"
#include "smbutil.h"

CShareItem::CShareItem(CListView *parent, CListViewItem *pLogicalParent, CSMBShareInfo *pInfo) :
	CNetworkFileContainer(parent, pLogicalParent), CSMBShareInfo(*pInfo)
{
	Init();
	SetPixmapID(keClosedFolderIcon, FALSE, FALSE);
}

CShareItem::CShareItem(CNetworkTreeItem *parent, CListViewItem *pLogicalParent, CSMBShareInfo *pInfo) :
	CNetworkFileContainer(parent, pLogicalParent), CSMBShareInfo(*pInfo)
{
	Init();
	SetPixmapID(keClosedFolderIcon, FALSE, FALSE);
}

QTTEXTTYPE CShareItem::text(int column) const
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
		case 2:
			return (LPCSTR)m_Comment;
	}
}

QPixmap *CShareItem::Pixmap(BOOL bIsBig)
{
	if (m_ShareType == "Printer")
		return LoadPixmap(bIsBig ? kePrinterIconBig : kePrinterIcon);

	return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon);
}

