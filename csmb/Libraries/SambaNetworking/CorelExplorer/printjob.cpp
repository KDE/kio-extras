/* Name: printjob.cpp

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

#include "common.h"
#include <aps.h>
#include "treeitem.h"
#include "printjob.h"

////////////////////////////////////////////////////////////////////////////

typedef struct 
{
	int m_JobID;
	BOOL m_bResult;
} CPrintJobSearchRequest;

////////////////////////////////////////////////////////////////////////////

BOOL SearchPrintJob_Handler(Aps_JobHandle /*hPrintJob*/, Aps_QuickJobInfo *pInfo, void *pUserData)
{
	CPrintJobSearchRequest *pRequest = (CPrintJobSearchRequest*)pUserData;

	if (pRequest->m_JobID == pInfo->jobID)
	{
		pRequest->m_bResult = TRUE; // Found!
		return FALSE; // stop iteration
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

BOOL ListPrintJob_Handler(Aps_JobHandle /*hPrintJob*/, Aps_QuickJobInfo *pInfo, void *pUserData)
{
  CPrintJobList *pList = (CPrintJobList *)pUserData;
  CPrintJobInfo *pJob = new CPrintJobInfo;
  pJob->m_User = pInfo->ownerName;
  pJob->m_Document = pInfo->jobName;
  pJob->m_Status.sprintf("%d", pInfo->jobStatus);
  pJob->m_Size.sprintf("%d", pInfo->jobSize);  
  pJob->m_Format = pInfo->jobFormat;
  pJob->m_JobID = pInfo->jobID;
  pJob->m_TimeSubmitted.sprintf("%ld", pInfo->jobCreationTime);
  pList->append(pJob);
  
  return TRUE; // continue iteration
}

////////////////////////////////////////////////////////////////////////////

BOOL RemovePrintJob_Handler(Aps_JobHandle hPrintJob, Aps_QuickJobInfo *pInfo, void *pUserData)
{
  CPrintJobInfo *pJob = (CPrintJobInfo *)pUserData;

  if (pJob->m_JobID == pInfo->jobID && 
      pJob->m_User == pInfo->ownerName)
  {                   	
    Aps_AddRef(hPrintJob);  //very important -- lock this job
      
    Aps_Result rc = Aps_JobIssueCommand(hPrintJob, APS_OP_JDELETE, NULL);

    if (APS_SUCCESS != rc)
    {
      char errmes[200];
      Aps_GetResultText(rc, errmes,200);
      printf(" - Failed: %s\n", errmes);
    }
      
    return FALSE; // stop iterating
  }
    
  return TRUE; // continue iteration
}
				
////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetPrintQueue(LPCSTR sPrinterName, CPrintJobList &list)
{
  list.clear();
  return IteratePrintQueue(sPrinterName, ListPrintJob_Handler, (void *)&list);
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode RemovePrintJob(LPCSTR sPrinterName, CPrintJobInfo *pInfo)
{
  return IteratePrintQueue(sPrinterName, RemovePrintJob_Handler, (void *)pInfo);
}

////////////////////////////////////////////////////////////////////////////

BOOL FindPrintJob(LPCSTR sPrinterName, int JobID)
{
	CPrintJobSearchRequest request;
	request.m_JobID = JobID;
	request.m_bResult = FALSE;

	if (keSuccess == IteratePrintQueue(sPrinterName, SearchPrintJob_Handler, (void *)&request))
	{
		return request.m_bResult;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode IteratePrintQueue(LPCSTR sPrinterName, LPFNJobHandler lpfnJobHandler, void *pUserData)
{
	Aps_PrinterHandle hPrinter;
	Aps_QueueHandle hQueue;
	Aps_JobHandle hPrintJob = NULL;
	
  if (APS_SUCCESS != Aps_OpenPrinter(sPrinterName, &hPrinter))
    return keErrorAccessDenied;
     
	if (APS_SUCCESS != Aps_PrinterOpenQueue(hPrinter, &hQueue))
  {
    Aps_ReleaseHandle(hPrinter);
    return keErrorAccessDenied;
  }
	
	while (Aps_Succeeded(Aps_QueueIterateJobs(hQueue, &hPrintJob)) && hPrintJob)
 	{
    Aps_QuickJobInfo *pInfo = NULL;

    if (APS_SUCCESS == Aps_JobMakeQuickJobInfo(hPrintJob, &pInfo) &&
        NULL != pInfo)
    {
      BOOL bKeepGoing = (*lpfnJobHandler)(hPrintJob, pInfo, pUserData);
      
      Aps_ReleaseBuffer(pInfo);

      if (!bKeepGoing)
        break;
    }
	}
	
	if (NULL != hPrintJob)
    Aps_ReleaseHandle(hPrintJob);
			
  Aps_ReleaseHandle(hQueue);
	Aps_ReleaseHandle(hPrinter);

  return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

CPrintJobItem::CPrintJobItem(CListView *parent, CListViewItem *pLogicalParent, const CPrintJobInfo &info)
  : CNetworkTreeItem(parent, pLogicalParent), CPrintJobInfo(info)
{
  InitPixmap();
}

////////////////////////////////////////////////////////////////////////////

CPrintJobItem::CPrintJobItem(CListViewItem *parent, CListViewItem *pLogicalParent, const CPrintJobInfo &info)
  : CNetworkTreeItem(parent, pLogicalParent), CPrintJobInfo(info)
{
  InitPixmap();
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CPrintJobItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network";
		
    case 0:
      return m_Document;

    case 1:
      return m_User;

    case 2:
      return m_Status;

    case 3:
      return m_Size;

    case 4:
      return m_Format;
    
    case 5:
      return m_TimeSubmitted;
    
    default:
      break;
	}

  return TOQTTEXTTYPE(""); // we should never get there
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CPrintJobItem::Pixmap()
{ 
  return LoadPixmap(kePrinterIcon); // TODO: create print job icon
}
                      
////////////////////////////////////////////////////////////////////////////

QPixmap *CPrintJobItem::BigPixmap()
{ 
  return LoadPixmap(kePrinterIconBig);
}

////////////////////////////////////////////////////////////////////////////

QString CPrintJobItem::FullName(BOOL /*bDoubleSlashes*/)
{ 
  return QString("");
}

////////////////////////////////////////////////////////////////////////////

