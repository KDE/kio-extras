/* Name: ExportsEntryDialog.cpp

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

#include "ExportsEntryDialog.h"
#include "common.h"
#include "qmessagebox.h"
#include "qapplication.h"

#define Inherited CExportsEntryDialogData

///////////////////////////////////////////////////////////////////////////////

CExportsEntryDialog::CExportsEntryDialog
(
	const char *pHostname /* = NULL */,
	const char *pAccessType /* = NULL */,
	const char *pOptions /* = NULL */,
	QWidget* parent,
	const char* name
)
	:
	Inherited(parent, name)
{
	if (NULL != pHostname)
	{
		m_pHostnameEdit->setText(pHostname);
		m_pHostnameEdit->setSelection(0, strlen(pHostname));
		setCaption(LoadString(knEDIT_HOST_PERMISSIONS));
	}
	else
		setCaption(LoadString(knADD_HOST_PERMISSIONS));
	
	m_pHostnameEdit->setFocus();

	m_pAccessType->insertItem(LoadString(knSTR_FULL_CONTROL));
	m_pAccessType->insertItem(LoadString(knSTR_READ));
	m_pAccessType->insertItem(LoadString(knSTR_NO_ACCESS));
	
	m_pAccessType->setCurrentItem(0);

	if (NULL != pAccessType)
	{
		for (int i=0; i < m_pAccessType->count(); i++)
		{
			if (!strcmp(m_pAccessType->text(i), pAccessType))
			{
				m_pAccessType->setCurrentItem(i);
				break;
			}
		}
	}

	m_pHostnameLabel->setText(LoadString(kn_HOST_COLON));
	m_pHostnameLabel->setBuddy(m_pHostnameEdit);
	m_pAccessTypeLabel->setText(LoadString(kn_PERMISSIONS_COLON));
	m_pAccessTypeLabel->setBuddy(m_pAccessType);
	m_pOptionsLabel->setText(LoadString(kn_OPTIONS_COLON));
	m_pOptionsLabel->setBuddy(m_pOptionsEdit);
	
	if (NULL != pOptions)
	{
		m_pOptionsEdit->setText(pOptions);
	}
	
	m_pOKButton->setText(LoadString(knOK));
	m_pOKButton->setAutoDefault(TRUE);
	m_pOKButton->setDefault(TRUE);
	m_pCancelButton->setText(LoadString(knCANCEL));
}

///////////////////////////////////////////////////////////////////////////////

CExportsEntryDialog::~CExportsEntryDialog()
{
}

///////////////////////////////////////////////////////////////////////////////

void CExportsEntryDialog::done(int r)
{
	if (r == 1)
	{
		m_Hostname = m_pHostnameEdit->text();
		m_Hostname = m_Hostname.stripWhiteSpace();

		/*
    if (m_Hostname.isEmpty())
		{
			QMessageBox::critical(qApp->mainWidget(), 
														LoadString(knERROR), 
														LoadString(knPLEASE_ENTER_HOSTNAME), 
														LoadString(knOK));
			return;
		}
    */
		
		m_AccessType = m_pAccessType->currentText();
		m_Options  = m_pOptionsEdit->text();
	}

	QDialog::done(r);
}

///////////////////////////////////////////////////////////////////////////////

