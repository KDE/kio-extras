/* Name: PrinterSelectionDialog.cpp

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

#include "PrinterSelectionDialog.h"
#include <qheader.h>
#include "msroot.h"
#include "qmessagebox.h"

#define Inherited CPrinterSelectionDialogData

CPrinterSelectionDialog::CPrinterSelectionDialog
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name)
{
	m_nCredentialsIndex = 0;

	m_pPrinterLabel->setText(LoadString(knPRINTER_COLON));
	m_pConnectAsLabel->setText(LoadString(knCONNECT_AS_COLON));
	m_pConnectAsLabel->setBuddy(m_pConnectAs);
	m_pOKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));

	/* Tree */

	((QWidget*)m_pTree->header())->hide();
	
	m_pRoot = new CMSWindowsNetworkItem(m_pTree, keDetail_Printers);
	m_pRoot->setOpen(TRUE);
	
	connect(m_pTree, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnDoubleClicked(CListViewItem*)));
	connect(m_pPath, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
	
	// We update "Connect As..." control because filling the tree
	// could potentially update gCredentials[0]

	UpdateConnectAs();

	m_pPath->setFocus();
}


CPrinterSelectionDialog::~CPrinterSelectionDialog()
{
}

void CPrinterSelectionDialog::OnDoubleClicked(CListViewItem *pItem)
{
	if (!pItem->isExpandable())
	{
		CNetworkTreeItem *pTreeItem = (CNetworkTreeItem*)pItem;

		m_pPath->setText(pTreeItem->FullName(FALSE));

		if (!stricmp(m_pConnectAs->text(), m_DefaultConnectAs))
		{
			m_nCredentialsIndex = pTreeItem->CredentialsIndex();
			UpdateConnectAs();
		}
	}
	
	//	pItem->setOpen(!pItem->isOpen());
	//else
}

void CPrinterSelectionDialog::OnReturnPressed()
{
	QString Server, Share, Path;
	Path = m_pPath->text();

	ParseUNCPath(m_pPath->text(), Server, Share, Path);

	if (!Server.isEmpty() && Share.isEmpty() && Path.isEmpty())
	{
		CListViewItem *pItem = m_pRoot->FindAndExpand(m_pPath->text());

		if (NULL != pItem)
		{
			m_pTree->setSelected(pItem, TRUE);
      m_pTree->ensureItemVisible(pItem);
			m_pTree->setFocus();
		}
	}
}																				 

void CPrinterSelectionDialog::UpdateConnectAs()
{
	m_DefaultConnectAs.sprintf("%s\\%s", (LPCSTR)gCredentials[m_nCredentialsIndex].m_Workgroup, (LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName);
	m_pConnectAs->setText(m_DefaultConnectAs);
}


void CPrinterSelectionDialog::done(int r)
{
	if (r == Accepted)
	{
		/* Get and validate UNC path */

		m_Path = m_pPath->text();
		
		if (m_Path.isEmpty())
		{
			QMessageBox::warning(this, LoadString(knUNC_PATH), LoadString(knENTER_UNC_PATH));
			m_pPath->setFocus();
			return;
		}

		/* Get "connect as" string */

		QString s(m_pConnectAs->text());

		if (!s.isEmpty())
		{
			CCredentials cred;

			LPCSTR p = (LPCSTR)s;
			cred.m_Workgroup = ExtractWord(p, "/\\");
			cred.m_UserName = ExtractWord(p, "/\\");
			
			cred.m_Password = gCredentials[0].m_Password;

			if (cred.m_UserName.isEmpty())
			{
				cred.m_UserName = cred.m_Workgroup;
				cred.m_Workgroup = gCredentials[m_nCredentialsIndex].m_Workgroup;
			}
		
			m_nCredentialsIndex = gCredentials.Find(cred);
		
			if (m_nCredentialsIndex == -1)
				m_nCredentialsIndex = gCredentials.Add(cred);
		}
	}

	QDialog::done(r);
}

