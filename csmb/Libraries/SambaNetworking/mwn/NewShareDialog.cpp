/* Name: NewShareDialog.cpp

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

#include "NewShareDialog.h"
#include "PermissionsDlg.h"
#include <qmessagebox.h>

#define Inherited CNewShareDialogData

CNewShareDialog::CNewShareDialog
(
	LPCSTR Path,
  BOOL bIsPrinter,
	CSectionList& Config,
	QWidget* parent,
	const char* name
)
	:
	Inherited(parent, name),
	m_Config(Config)
{
	setCaption(LoadString(knSTR_NEW_SHARE));
	
  m_bIsPrinter = bIsPrinter;

	m_pShareNameLabel->setText(LoadString(knSHARE_NAME_COLON));
	m_pShareNameLabel->setBuddy(m_ShareName);

	QSize ShareLabelSize = m_pShareNameLabel->sizeHint();
	
	if (m_pShareNameLabel->width() < ShareLabelSize.width())
	{
		int nDelta = ShareLabelSize.width() - m_pShareNameLabel->width() + 10;
		
		m_pShareNameLabel->resize(ShareLabelSize.width(), m_pShareNameLabel->height());
		m_ShareName->setGeometry(m_ShareName->x() + nDelta, m_ShareName->y(), m_ShareName->width() - nDelta, m_ShareName->height());
	}
	
	m_pCommentLabel->setText(LoadString(knSTR_COMMENT_COLON));
	m_pCommentLabel->setBuddy(m_Comment);

	m_Enabled->setText(LoadString(knSHARE_ENABLED));
	m_MaximumAllowed->setText(LoadString(knALLOW_ALL_USERS));
	m_Allow->setText(LoadString(knALLOW_MAXIMUM));
	m_pUsersLabel->setText(LoadString(knUSERS));
	m_pPermissionsButton->setText(LoadString(kn_PERMISSIONS_DOTDOTDOT));
	m_pOKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));
	m_ButtonGroup->setTitle(LoadString(knUSER_LIMIT));
	
	m_Section.SetPublic(TRUE); // Everyone/full control initially...
  m_Section.SetBrowseable(TRUE);
	
  if (m_bIsPrinter)
  {
    m_Section.SetValue("yes", "printable");
    m_Section.SetValue(Path+10, "printer");
    m_Section.SetValue("/tmp", "path");
  }
  else
  {
    m_Section.SetValue(Path, "path");
    m_Section.SetWritable(TRUE); 
  }

	m_Section.m_FileName = m_Config.first()->m_FileName;

	m_ShareName->setFocus();
	m_ButtonGroup->insert(m_MaximumAllowed);
	m_ButtonGroup->insert(m_Allow);
	m_MaximumAllowed->setChecked(TRUE);

	m_Enabled->setChecked(m_Section.IsAvailable());
}

////////////////////////////////////////////////////////////////////////////

CNewShareDialog::~CNewShareDialog()
{
}

////////////////////////////////////////////////////////////////////////////

void CNewShareDialog::OnPermissions()
{
	m_Section.m_SectionName = m_ShareName->text();

	CPermissionsDlg dlg(&m_Section);

	dlg.exec();
}

////////////////////////////////////////////////////////////////////////////

void CNewShareDialog::done(int r)
{
	if (1 == r)
	{
		m_Section.m_SectionName = m_ShareName->text();
		
		if (!m_Section.m_SectionName.length())
		{
			QMessageBox::critical(this, caption(), LoadString(knENTER_SHARE_NAME));
			m_ShareName->setFocus();
			return;
		}

		if (m_Section.m_SectionName.length() > 13)
		{
			QString a;
			a.sprintf(LoadString(knSTR_SHARE_TOOLONG),
					  (LPCSTR)m_Section.m_SectionName);

			QMessageBox::critical(this, caption(), (LPCSTR)a);
			m_ShareName->setFocus();
			return;
		}
		
		// See if we already have share with that name
		
		QListIterator<CSection> it(m_Config);
	
		for (; NULL != it.current(); ++it)
		{
			if (!stricmp(it.current()->m_SectionName, m_Section.m_SectionName))
				break;
		}

		if (NULL != it.current())
		{
			LPCSTR NewPath = m_Section.Value("path");
			LPCSTR OldPath = it.current()->Value("path");

			if (!strcmp(OldPath, NewPath))
			{
				QString a;
				a.sprintf(LoadString(knSTR_SHARE_DUP),
						  (LPCSTR)m_Section.m_SectionName);

				QMessageBox::critical(this, caption(), (LPCSTR)a);
				return;
			}
			else
			{
				QString a;
				
				a.sprintf(LoadString(knSTR_SHARE_DUP2),
					OldPath, (LPCSTR)m_Section.m_SectionName,
					NewPath, (LPCSTR)m_Section.m_SectionName);

				if (0 != QMessageBox::warning(this, caption(), (LPCSTR)a, LoadString(knYES), LoadString(knNO)))
					return;

				//it.current()->Empty();
				it.current()->clear();
				it.current()->m_bChanged = TRUE;
			}
		}
		
		m_Section.SetValue(m_Comment->text(), "comment");
		
		QString MaxConnections;

		if (m_MaximumAllowed->isChecked())
		{
			MaxConnections = "0";
		}
		else
		{
			MaxConnections = m_UsersMax->text();
		}

		m_Section.SetValue(MaxConnections, "max connections");

		//m_Section.Write();
		m_Section.m_bChanged = TRUE;
		m_Config.append(new CSection(m_Section));
	}

	QDialog::done(r);
}

////////////////////////////////////////////////////////////////////////////

