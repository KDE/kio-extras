/* Name: PermissionsAddDlg.cpp

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

#include "PermissionsAddDlg.h"
#include <qmessagebox.h>
#include "qlabel.h"
#include "qbuttongroup.h"
#include "qpushbutton.h"
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <unistd.h>

#define Inherited CPermissionsAddDlgData

CPermissionsAddDlg::CPermissionsAddDlg
(
	BOOL bIsPrinter,
  QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	setCaption(LoadString(knADD_USERS_AND_GROUPS));
	m_pButtonGroup->setTitle(LoadString(knUSERS_AND_GROUPS));
	m_User->setChecked(TRUE);
	m_User->setText(LoadString(knUSER_COLON));
	
	struct passwd *pw = getpwuid(getuid());
	QString UserID((NULL == pw) ? "" : pw->pw_name);

	struct group *ge = getgrgid(getgid());
	QString GroupID((NULL == ge) ? "" : ge->gr_name);
	
	// Fill 'User' combo
	setpwent();

	QStrList list;

	while (NULL != (pw=getpwent()))
	{
		if (strlen(pw->pw_name) <= 12 ||
				strncmp(pw->pw_name, "$_SHAREUSER_", 12))
			list.inSort(pw->pw_name);
	}
	endpwent();

	m_UserCombo->insertStrList(&list);

	QStrListIterator it(list);
	int i;

	for (i=0; NULL != it.current(); ++it,++i)
	{
		if (!strcmp(it.current(), UserID))
		{
			m_UserCombo->setCurrentItem(i);
			break;
		}
	}
	
	m_UserCombo->setFocus();
	
	m_Group->setText(LoadString(knGROUP_COLON));
	
  // Fill 'Group' combo

	setgrent();
	list.clear();

	while (NULL != (ge=getgrent()))
	{
		list.inSort(ge->gr_name);
	}
	endgrent();

	m_GroupCombo->insertStrList(&list);

	for (i=0, it.toFirst(); NULL != it.current(); ++it,++i)
	{
		if (!strcmp(it.current(), GroupID))
		{
			m_GroupCombo->setCurrentItem(i);
			break;
		}
	}

	m_GroupCombo->setEnabled(FALSE);
	m_Everyone->setText(LoadString(knSTR_EVERYONE));
	m_pAccessTypeLabel->setText(LoadString(knTYPE_OF_ACCESS_COLON));
	
	if (bIsPrinter)
    m_AccessType->insertItem(LoadString(knSTR_PRINT));
  else
  {
    m_AccessType->insertItem(LoadString(knSTR_FULL_CONTROL));
	  m_AccessType->insertItem(LoadString(knSTR_READ));
  }
	
  m_AccessType->insertItem(LoadString(knSTR_NO_ACCESS));
	
	m_pOKButton->setText(LoadString(knOK));
	m_pOKButton->setAutoDefault(TRUE);
	m_pOKButton->setDefault(TRUE);
	m_pCancelButton->setText(LoadString(knCANCEL));
}


CPermissionsAddDlg::~CPermissionsAddDlg()
{
}

void CPermissionsAddDlg::done(int r)
{
	if (1 == r)
	{
		m_bIsGroup = FALSE;

		if (m_User->isChecked())
			m_Name = m_UserCombo->currentText();
		else
			if (m_Group->isChecked())
			{
				m_Name = m_GroupCombo->currentText();
				m_bIsGroup = TRUE;
			}
			else
				m_Name = LoadString(knSTR_EVERYONE);
	
		if (m_Name.isEmpty())
		{
			QMessageBox::critical(this, caption(), LoadString(knENTER_USER_GROUP_NAME));
			return;
		}
		
		m_Access = m_AccessType->currentText();
	}

	QDialog::done(r);
}

void CPermissionsAddDlg::OnAccessTypeComboChanged(int nIndex)
{
}

void CPermissionsAddDlg::OnUser()
{
	m_UserCombo->setEnabled(TRUE);
	m_GroupCombo->setEnabled(FALSE);
	m_UserCombo->setFocus();
}

void CPermissionsAddDlg::OnGroup()
{
	m_UserCombo->setEnabled(FALSE);
	m_GroupCombo->setEnabled(TRUE);
	m_GroupCombo->setFocus();
}

void CPermissionsAddDlg::OnEveryone()
{
	m_UserCombo->setEnabled(FALSE);
	m_GroupCombo->setEnabled(FALSE);
}

