/* Name: PropDialog.h

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

#ifndef __INC_FILEPROPDIALOG_H__
#define __INC_FILEPROPDIALOG_H__

#include "PropDialogData.h"
#include "common.h"
#include <qlist.h>

class CNetworkTreeItem;
class CSharingPage;
class CSharingPageNFS;
class CFilePermissions;
class KPrinterObject;
class KPrinterGeneralDlg;
class KPrinterAdvancedDlg;
class KPrinterOutputDlg;

class CPropDialog : public CPropDialogData
{
    Q_OBJECT

public:

  CPropDialog(QList<CNetworkTreeItem> &ItemList,
              BOOL bStartFromSharing = FALSE,
              QWidget* parent = NULL,
              const char* name = NULL);

  CPropDialog::CPropDialog(LPCSTR URL,
                           BOOL bStartFromSharing = FALSE,
                           QWidget* parent = NULL,
                           const char* name = NULL);

  virtual ~CPropDialog();

protected slots:
	
	void OnOK();
	void OnInitDialog();
  void FixSize();

private:
	CSharingPage *m_pWindowsSharingPage;
	CFilePermissions *m_pFilePermissionsPage;
	CSharingPageNFS *m_pNFSSharingPage;
	
  KPrinterGeneralDlg *m_pPrinterGeneral;
  KPrinterAdvancedDlg *m_pPrinterAdvanced;
  KPrinterOutputDlg *m_pPrinterOutput;

  BOOL m_bStartFromSharing;
  void HandleLocalPrinter(LPCSTR PrinterName, BOOL& bCanDoSambaSharing);
  KPrinterObject *m_pPrinterObject;
};

#endif /* __INC_FILEPROPDIALOG_H__ */

