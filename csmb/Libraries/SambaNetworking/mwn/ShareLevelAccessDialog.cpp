/* Name: ShareLevelAccessDialog.cpp

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


#include "ShareLevelAccessDialog.h"
#include <time.h> // for time()
#include <stdlib.h> // for rand()
#include "qmessagebox.h"
#include "qapplication.h"

#define Inherited CShareLevelAccessDialogData

CShareLevelAccessDialog::CShareLevelAccessDialog
(
	CSection *pSection,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	setCaption(LoadString(knSTR_ACCESS_THROUGH_SHARE));
	m_pSection = pSection;

	m_pTopLabel->setText(LoadString(knSHARE_ACCESS_DIALOG_TOP));
	m_pAllowAnonymousAccessRadio->setText(LoadString(knANONYMOUS_ACCESS));
	m_pAllowAccessUsingPasswordRadio->setText(LoadString(knALLOW_ACCESS_USING_THIS_PASSWORD_COLON));
	m_pConfirmPasswordLabel->setText(LoadString(kn_CONFIRM_PASSWORD_COLON));
	m_pConfirmPasswordLabel->setBuddy(m_pConfirmPasswordEdit);
	m_pOKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));
	
	m_pReadOnlyAccessCheckbox->setText(LoadString(knACCESS_TO_THIS_SHARE_IS_READONLY));
	m_pButtonGroup = new QButtonGroup(this, "ButtonGroup");

	m_pButtonGroup->insert(m_pAllowAnonymousAccessRadio);
	m_pButtonGroup->insert(m_pAllowAccessUsingPasswordRadio);
	m_pButtonGroup->hide();

	if (pSection->IsPublic())
	{
    m_pAllowAnonymousAccessRadio->setChecked(TRUE);
		m_pAllowAnonymousAccessRadio->setFocus();
  }
	else
	{
    m_pAllowAccessUsingPasswordRadio->setChecked(TRUE);
		m_pPasswordEdit->setFocus();
	}

	m_pReadOnlyAccessCheckbox->setChecked(pSection->IsWritable());
	
	m_pOKButton->setAutoDefault(TRUE);
	m_pOKButton->setDefault(TRUE);
	m_bPasswordChanged = FALSE;
	m_pPasswordEdit->setEchoMode(QLineEdit::Password);
	
	if (!m_pSection->IsPublic())
	{
		m_pPasswordEdit->setText("********");
		m_pPasswordEdit->setSelection(0, strlen(m_pPasswordEdit->text()));
	}

	m_pConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
	m_pConfirmPasswordEdit->setEnabled(FALSE);
#ifdef QT_20
	connect(m_pPasswordEdit, SIGNAL(textChanged(const QString&)), this, SLOT(OnPasswordChanged_QT20(const QString &)));
#else
	connect(m_pPasswordEdit, SIGNAL(textChanged(const char *)), this, SLOT(OnPasswordChanged_QT144(const char *)));
#endif
}

CShareLevelAccessDialog::~CShareLevelAccessDialog()
{
	delete m_pButtonGroup;
}


void CShareLevelAccessDialog::OnAnonymousAccess()
{
	m_pPasswordEdit->setEnabled(FALSE);
	m_pConfirmPasswordEdit->setEnabled(FALSE);
}

void CShareLevelAccessDialog::OnPasswordAccess()
{
	m_pPasswordEdit->setEnabled(TRUE);
	
	if (m_bPasswordChanged)
		m_pConfirmPasswordEdit->setEnabled(TRUE);
	
	m_pPasswordEdit->setFocus();
}

void CShareLevelAccessDialog::done(int r)
{
	if (1 == r)
	{
		if (m_pAllowAnonymousAccessRadio->isChecked())
		{
			m_pSection->SetPublic(TRUE);
		}
		else
		{
			m_pSection->SetPublic(FALSE);
			
			LPCSTR p = m_pSection->Value("user");

			if (NULL == p || 
					strlen(p) < 12 || 
					strncmp(p, "$_SHAREUSER_", 12))
			{
				QString UserName;
        UserName.sprintf("$_SHAREUSER_%lx_%lx$", time(NULL), rand() & 0xffff);

				m_pSection->SetValue(UserName, "user");
			}

			if (m_bPasswordChanged)
			{
				QString Password(m_pPasswordEdit->text());
				QString ConfirmPassword(m_pConfirmPasswordEdit->text());

				if (Password != ConfirmPassword)
				{
					QMessageBox::critical(qApp->mainWidget(), 
																LoadString(knERROR), 
																LoadString(knPASSWORDS_NOT_IDENTICAL), 
																LoadString(knOK));
					return;
				}
				
				m_pSection->SetValue(Password, "sambapassword");
			}
		}
	}
	
	QDialog::done(r);
}

#ifdef QT_20
void CShareLevelAccessDialog::OnPasswordChanged_QT20(const QString &)
#else
void CShareLevelAccessDialog::OnPasswordChanged_QT144(const char *)
#endif
{
	if (!m_bPasswordChanged)
	{
		m_bPasswordChanged = TRUE;
		m_pConfirmPasswordEdit->setEnabled(TRUE);
	}
}

#ifdef QT_20
void CShareLevelAccessDialog::OnPasswordChanged_QT144(const char *)
#else
void CShareLevelAccessDialog::OnPasswordChanged_QT20(const QString&)
#endif
{
	// must be empty.
	// this added to allow compiling under both Qt1.44 and Qt2.x
}

