/* Name: printjob.h

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

#ifndef __INC_PRINTJOB_H__
#define __INC_PRINTJOB_H__

#include "common.h"
#include "qlist.h"

typedef struct 
{
  QString m_User;
  QString m_Document;
  QString m_Status;
  QString m_Size;
  QString m_Format;
  QString m_TimeSubmitted;
  int m_JobID;
} CPrintJobInfo;

typedef QList<CPrintJobInfo> CPrintJobList;

class CPrintJobItem : public CNetworkTreeItem, public CPrintJobInfo
{
public:
  CPrintJobItem(CListView *parent, CListViewItem *pLogicalParent, const CPrintJobInfo& info);
  CPrintJobItem(CListViewItem *parent, CListViewItem *pLogicalParent, const CPrintJobInfo &info);
  
  void Init();
		
	QTTEXTTYPE text(int column) const;
  
	QString FullName(BOOL bDoubleSlashes);

	virtual int CredentialsIndex() { return 0; }

	void Fill()
  {
  }

	QPixmap *Pixmap();
	QPixmap *BigPixmap();
	
	CItemKind Kind()
	{ 
		return kePrintJobItem;
	}
};

typedef BOOL (*LPFNJobHandler)(Aps_JobHandle hPrintJob, Aps_QuickJobInfo *pInfo, void *pUserData);

CSMBErrorCode IteratePrintQueue(LPCSTR sPrinterName, LPFNJobHandler lpfnJobHandler, void *pUserData);
CSMBErrorCode GetPrintQueue(LPCSTR sPrinterName, CPrintJobList &list);
CSMBErrorCode RemovePrintJob(LPCSTR sPrinterName, CPrintJobInfo *pInfo);
BOOL FindPrintJob(LPCSTR sPrinterName, int JobID);

#endif /* __INC_PRINTJOB_H__ */

