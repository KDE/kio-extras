/* Name: printer.cpp

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
#include "printer.h"
#include <aps.h>
#include <malloc.h>
#include "printjob.h"

////////////////////////////////////////////////////////////////////////////

CPrinterItem::CPrinterItem(CListView *parent, LPCSTR name)
  : CNetworkTreeItem(parent, NULL)
{
  m_Name = name;
  Init();
}

////////////////////////////////////////////////////////////////////////////

CPrinterItem::CPrinterItem(CListView *parent, const CPrinterInfo &other)
  : CNetworkTreeItem(parent, NULL), CPrinterInfo(other)
{
  InitPixmap();
}

////////////////////////////////////////////////////////////////////////////

CPrinterItem::CPrinterItem(CListViewItem *parent, const CPrinterInfo &other)
  : CNetworkTreeItem(parent, NULL), CPrinterInfo(other)
{
  InitPixmap();
}

////////////////////////////////////////////////////////////////////////////

CPrinterItem::CPrinterItem(CListViewItem *parent, LPCSTR name) :
  CNetworkTreeItem(parent, NULL)
{
  m_Name = name;
  Init();
}

////////////////////////////////////////////////////////////////////////////

void CPrinterItem::Init()
{
  char *manufacturer, *model;
  Aps_PrinterHandle h;

  if (APS_SUCCESS == Aps_OpenPrinter((LPCSTR)m_Name
#ifdef QT_20
  .latin1()
#endif
  , &h))
  {
    if (APS_SUCCESS == Aps_PrinterGetModel(h, &manufacturer, &model))
    {
      m_Manufacturer = manufacturer;

			Aps_ReleaseBuffer(manufacturer);
      m_Model = model;
      Aps_ReleaseBuffer(model);
    }

    char *location = NULL;

    if (APS_SUCCESS == Aps_PrinterGetConnectInfo(h, &m_ConnectionType, &location))
    {
      m_Location = location;
      Aps_ReleaseBuffer(location);

      if (IsUNCPath((LPCSTR)m_Location
#ifdef QT_20
  .latin1()
#endif
      ))
      {
        m_Location = MakeSlashesBackward((LPCSTR)m_Location
#ifdef QT_20
  .latin1()
#endif
        );
      }
    }
  }

  InitPixmap();

  SetExpansionStatus(keExpansionComplete);
  setExpandable(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CPrinterItem::Fill()
{
  SetExpansionStatus(keExpansionComplete);
  setExpandable(FALSE);
	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CPrinterItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network";

		case 0:
			return m_Name; //LoadString(knSTR_NFS_NETWORK);

    case 1:
    {
      switch (m_ConnectionType)
      {
        default:
        case APS_CONNECT_ALL:
          return TOQTTEXTTYPE(LoadString(knUNKNOWN));
        break;

        case APS_CONNECT_LOCAL:
          return TOQTTEXTTYPE(LoadString(knLOCAL));
        break;

        case APS_CONNECT_NETWORK_LPD:
          return TOQTTEXTTYPE(LoadString(knNETWORK_LPD));
        break;

        case APS_CONNECT_NETWORK_SMB:
          return TOQTTEXTTYPE(LoadString(knSTR_WINDOWS_NETWORK));
        break;
      }
    }
    break;

    case 2:
      return (QTTEXTTYPE)m_Location;
    break;

    case 3:
      return (QTTEXTTYPE)m_Manufacturer;

    case 4:
      return (QTTEXTTYPE)m_Model;

    default:
			return TOQTTEXTTYPE("");
	}

  return TOQTTEXTTYPE(""); // we should never get there
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CPrinterItem::Pixmap()
{
  return LoadPixmap(m_ConnectionType == APS_CONNECT_LOCAL ?
                    kePrinterIcon :
                    keNetworkPrinterIcon);
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CPrinterItem::BigPixmap()
{
  return LoadPixmap(m_ConnectionType == APS_CONNECT_LOCAL ?
                    kePrinterIconBig :
                    keNetworkPrinterIconBig);
}

////////////////////////////////////////////////////////////////////////////

QString CPrinterItem::FullName(BOOL /*bDoubleSlashes*/)
{
  return QString(GetHiddenPrefix(knPRINTER_HIDDEN_PREFIX)) + m_Name;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode PurgePrinter(LPCSTR sPrinterName)
{
  CSMBErrorCode rc = keSuccess;
  Aps_PrinterHandle hPrinter;
	Aps_QueueHandle hQueue;

	if (APS_SUCCESS != Aps_OpenPrinter(sPrinterName, &hPrinter))
    return keErrorAccessDenied;

  if (APS_SUCCESS != Aps_PrinterOpenQueue(hPrinter, &hQueue))
  {
    Aps_ReleaseHandle(hPrinter);
    return keErrorAccessDenied;
  }

	if (APS_SUCCESS != Aps_QueueIssueCommand(hQueue, APS_OP_QPURGE, NULL))
    rc = keErrorAccessDenied;

  Aps_ReleaseHandle(hQueue);
 	Aps_ReleaseHandle(hPrinter);

  return rc;
}

////////////////////////////////////////////////////////////////////////////

BOOL CPrinterItem::ContentsChanged(time_t /*SinceWhen*/,
																			 int nOldChildCount,
																			 CListViewItem * /*pOldFirst*/)
{
	CPrintJobList list;

	if (keSuccess != GetPrintQueue((LPCSTR)m_Name
#ifdef QT_20
  .latin1()
#endif
  , list))
		return FALSE;

	if ((int)list.count() != nOldChildCount)
	{
		return TRUE;
	}

	return FALSE;
}

