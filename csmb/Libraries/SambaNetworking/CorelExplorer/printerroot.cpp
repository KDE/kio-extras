/* Name: printroot.cpp

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

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "printerroot.h"
#include "printer.h"
#include <malloc.h>
#include <aps.h>

////////////////////////////////////////////////////////////////////////////

CPrinterRootItem::CPrinterRootItem(CListView *parent)
		: CNetworkTreeItem(parent, NULL)
{
	InitPixmap();
	connect(&gTreeExpansionNotifier, SIGNAL(CheckRefresh()), SLOT(CheckRefresh()));
}

CPrinterRootItem::CPrinterRootItem(CListViewItem *parent) :
	CNetworkTreeItem(parent, NULL)
{
  InitPixmap();
	connect(&gTreeExpansionNotifier, SIGNAL(CheckRefresh()), SLOT(CheckRefresh()));
}

void CPrinterRootItem::Fill()
{
	SetExpansionStatus(keExpanding);

  int count;
	char **names;

	if (APS_SUCCESS == Aps_GetPrinters(&names, &count))
	{
		if (count > 0)
		{
			int i;

			for (i=0; i < count; i++)
				new CPrinterItem(this, names[i]);
		}

		Aps_ReleaseBuffer(names);
	}

	SetExpansionStatus(keExpansionComplete);
	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CPrinterRootItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return TOQTTEXTTYPE("Network");

		case 0:
			return TOQTTEXTTYPE(LoadString(knPRINTERS));

		default:
			return TOQTTEXTTYPE("");
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *CPrinterRootItem::FindAndExpand(LPCSTR Path)
{
	if (!isOpen())
		setOpen(TRUE);

	if (IsPrinterUrl(Path))
		Path += 10;

	CListViewItem *child;

	for (child = firstChild(); child != NULL; child = child->nextSibling())
	{
		if (!strcmp((LPCSTR)child->text(0)
#ifdef QT_20
  .latin1()
#endif
    , Path))
			return child;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

BOOL CPrinterRootItem::ContentsChanged(time_t /*SinceWhen*/,
																			 int nOldChildCount,
																			 CListViewItem *pOldFirst)
{
	if (ExpansionStatus() == keNotExpanded)
		return FALSE;

	int count;
	char **names;

  if (APS_SUCCESS != Aps_GetPrinters(&names, &count))
		return FALSE;

	if (count != nOldChildCount)
	{
		Aps_ReleaseBuffer(names);
		return TRUE; // Number of printers changed!
	}

	BOOL retcode = FALSE; // means unchanged

	if (count > 0)
	{
		int i;

		for (i=0; i < count; i++)
		{
			CListViewItem *pI;

			for (pI=pOldFirst; NULL != pI; pI=pI->nextSibling())
			{
				if (!strcmp((LPCSTR)pI->text(0)
#ifdef QT_20
  .latin1()
#endif
        , names[i]))
					break;
			}

			if (NULL == pI) // Not found, exit the loop and return TRUE
				break;
		}

		retcode = (i < count); // if i < count means break was invoked and change occured
	}

	Aps_ReleaseBuffer(names);
	return retcode;
}

void CPrinterRootItem::CheckRefresh()
{
	if (IsScreenSaverRunning())
		return;

	if (ContentsChanged(0, childCount(), firstChild()))
	{
		RescanItem(this);
	}
}
