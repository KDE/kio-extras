/* Name: SharingPage.cpp

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

#include "common.h"
#include <stdlib.h>
#include "SharingPage.h"
#include "inifile.h"
#include "NewShareDialog.h"
#include "PermissionsDlg.h"
#include "ShareLevelAccessDialog.h"
#include "smbutil.h"
#include <qmessagebox.h>

#define Inherited CSharingPageData

////////////////////////////////////////////////////////////////////////////

CSharingPage::CSharingPage
(
	LPCSTR Path,
  QPixmap *pPixmap,
	QWidget* parent,
	const char* name
)
	:
	Inherited(parent, name),
	m_Path(Path),
	m_Config(gSambaConfiguration)
{
  QSize sz;

	m_pCurrentSection = NULL;
	
	m_PathLabel->setText(Path);

  m_bIsPrinter = IsPrinterUrl(Path);
  
  m_IconLabel->setPixmap(*pPixmap);
	m_MaximumAllowed->setChecked(TRUE);
	m_Enabled->setChecked(TRUE);

	m_SharedAs->setText(LoadString(knSHARE_THIS_ITEM_AND_ITS_CONTENTS));
	m_SharedAs->resize(m_SharedAs->sizeHint().width(), m_SharedAs->sizeHint().height());
  
	/* ----------------- adjust label sizes so it looks nice on localized versions --- */

	m_ShareNameLabel->setText(LoadString(knSHARE_NAME_COLON));
  m_ShareNameLabel->setBuddy(m_ShareName);
	
	QSize ShareLabelSize = m_ShareNameLabel->sizeHint();
	
	m_CommentLabel->setText(LoadString(knSTR_COMMENT_COLON));
	m_CommentLabel->setBuddy(m_Comment);
	
	QSize CommentLabelSize = m_CommentLabel->sizeHint();
	
	int nMaxLabelWidth = CommentLabelSize.width() > ShareLabelSize.width() ? CommentLabelSize.width() : ShareLabelSize.width();
  m_ShareNameLabel->resize(nMaxLabelWidth, m_ShareNameLabel->height());
  int nNewX = m_ShareNameLabel->x() + m_ShareNameLabel->width() + 10;

	m_ShareName->setGeometry(nNewX, m_ShareName->y(), m_ShareName->width() - nNewX + m_ShareName->x(), m_ShareName->height());
	m_ShareNameEdit->setGeometry(nNewX, m_ShareName->y(), m_ShareName->width(), m_ShareName->height());

	m_CommentLabel->resize(nMaxLabelWidth, m_CommentLabel->height());
	nNewX = m_CommentLabel->x() + m_CommentLabel->width() + 10;
	m_Comment->setGeometry(nNewX, m_Comment->y(), m_Comment->width() - nNewX + m_Comment->x(), m_Comment->height());
	
  /* --------------------------------------------------- */

	m_Enabled->setText(LoadString(knSHARE_ENABLED));
	sz = m_Enabled->sizeHint();

	m_Enabled->setGeometry(width() - 10 - sz.width(), m_Enabled->y(), sz.width(), sz.height());
	
	/* "New Share" and "Remove Share" buttons */

	m_NewShareButton->setText(LoadString(knNEW_SHARE_DOTDOTDOT));
	m_RemoveShareButton->setText(LoadString(knREMOVE_SHARE));
  
	sz = (m_RemoveShareButton->sizeHint());
  
  m_RemoveShareButton->setGeometry(width() - 10 - sz.width(),
                                   m_RemoveShareButton->y(),
                                   sz.width(),
                                   m_RemoveShareButton->height());

	sz = (m_NewShareButton->sizeHint());

  m_NewShareButton->setGeometry(m_RemoveShareButton->x() - 10 - sz.width(),
                                m_NewShareButton->y(),
                                sz.width(),
                                m_NewShareButton->height());

	/* ------------------------------------------- */

	m_UserLimitLabel->setText(LoadString(knUSER_LIMIT));
	m_UserLimitLabel->resize(m_UserLimitLabel->sizeHint().width(), m_UserLimitLabel->height());

  nNewX = m_UserLimitLabel->x() + m_UserLimitLabel->width() + 3;
  m_pSeparator3->setGeometry(nNewX, m_pSeparator3->y(), m_pSeparator3->width() - nNewX + m_pSeparator3->x(), m_pSeparator3->height());
  
	m_MaximumAllowed->setText(LoadString(knALLOW_ALL_USERS));
	m_Allow->setText(LoadString(knALLOW_MAXIMUM));
	m_UsersLabel->setText(LoadString(knUSERS));
	m_PermissionsButton->setText(LoadString(kn_PERMISSIONS_DOTDOTDOT));
	
  sz = m_PermissionsButton->sizeHint();
	m_PermissionsButton->resize(sz.width(), sz.height());
	
	Refresh();
}

////////////////////////////////////////////////////////////////////////////

CSharingPage::~CSharingPage()
{
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnNewShare()
{
	CNewShareDialog dlg(m_Path, m_bIsPrinter, m_Config, this);
	
	if (QDialog::Accepted == dlg.exec())
		Refresh();
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnRemoveShare()
{
	if (NULL != m_pCurrentSection)
	{
		m_pCurrentSection->clear();
		m_pCurrentSection->m_bChanged = TRUE;
		Refresh();
	}
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnComboChange(int nIndex)
{
	if (NULL != m_pCurrentSection && m_pCurrentSection->count() > 0)
	{
		m_pCurrentSection->SetValue(m_Comment->text(), "comment");
		m_pCurrentSection->SetValue(m_MaximumAllowed->isChecked() ? "0" : (LPCSTR)(m_UsersMax->text()), "max connections");
		m_pCurrentSection->SetAvailable(m_Enabled->isChecked());
	}

	QString ShareName(m_ShareName->currentText());
	
	QListIterator<CSection> it(m_Config);
	
	for (; it.current() != NULL; ++it)
	{
		if (!stricmp(it.current()->m_SectionName, ShareName))
		{
			m_pCurrentSection = it.current();

			m_Comment->setText(it.current()->Value("comment", NULL));
			
			m_Enabled->setChecked(m_pCurrentSection->IsAvailable());

			LPCSTR sUsersMax = it.current()->Value("max connections", NULL);

			QString UsersMax(sUsersMax == NULL ? "" : sUsersMax);

			if (UsersMax == "0")
				UsersMax = "";

			if (UsersMax.isEmpty())
			{
				m_MaximumAllowed->setChecked(TRUE);
        m_Allow->setChecked(FALSE);
				m_UsersMax->setEnabled(FALSE);
			}
			else
			{
				m_MaximumAllowed->setChecked(FALSE);
        m_Allow->setChecked(TRUE);
				m_UsersMax->setValue(atoi(UsersMax));
				m_UsersMax->setEnabled(TRUE);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnPermissions()
{
	if (m_ShareNameEdit->isVisible())
		m_pCurrentSection->m_SectionName = m_ShareNameEdit->text();

	LPCSTR SecurityLevel = gSambaConfiguration.Value("security");

	if (!stricmp(SecurityLevel, "share")) // Share level security
	{
		CShareLevelAccessDialog dlg(m_pCurrentSection);
		dlg.exec();
	}
	else
	{
		CPermissionsDlg dlg(m_pCurrentSection);
		dlg.exec();
	}
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnSharedAs()
{
	EnableSharedAsControls(m_SharedAs->isChecked());
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::EnableSharedAsControls(BOOL bEnable)
{
	m_SharedAs->setChecked(bEnable);
	m_ShareNameEdit->setEnabled(bEnable);
	m_Comment->setEnabled(bEnable);
	m_Allow->setEnabled(bEnable);
	m_UsersMax->setEnabled(bEnable && m_Allow->isChecked());
	m_MaximumAllowed->setEnabled(bEnable);
	m_NewShareButton->setEnabled(bEnable);
	m_RemoveShareButton->setEnabled(bEnable);
	m_PermissionsButton->setEnabled(bEnable);
	m_ShareName->setEnabled(bEnable);
	m_Enabled->setEnabled(bEnable);				   
  m_ShareNameLabel->setEnabled(bEnable);
  m_UsersLabel->setEnabled(bEnable);
  m_CommentLabel->setEnabled(bEnable);
  m_UserLimitLabel->setEnabled(bEnable);

	if (bEnable && m_ShareNameEdit->isVisible())
	{
		if (m_ShareNameEdit->text()[0] == '\0')
		{
			LPCSTR p = (LPCSTR)m_Path;
			LPCSTR p2 = p + strlen(p);
			
			while (p2 > p && *p2 != '/')
				p2--;

			if (*p2 == '/')
				p2++;

			
			m_ShareNameEdit->setText(p2);
			
			if (!m_Config.ShareExists(p2))
				m_Comment->setFocus();
			else
				m_ShareNameEdit->setFocus();
		}

		if (m_pCurrentSection == NULL)
		{
			m_pCurrentSection = new CSection;
			m_pCurrentSection->SetPublic(TRUE);
			m_pCurrentSection->SetWritable(TRUE);
			m_pCurrentSection->SetBrowseable(TRUE);
			m_pCurrentSection->SetValue(m_Path, "path");
			m_pCurrentSection->m_FileName = m_Config.first()->m_FileName;
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::Refresh()
{
  QListIterator<CSection> it(m_Config);
	
	m_ShareName->clear();

	for (; it.current() != NULL; ++it)
	{
		if ((m_bIsPrinter && 
         (
          (!stricmp(it.current()->m_SectionName, "printers") && gSambaConfiguration.IsLoadingPrinters()) ||
          (it.current()->IsPrintable() && 
           !strcmp(it.current()->Value("printer", NULL), (LPCSTR)m_Path+10))
         ) 
        ) ||
        (stricmp(it.current()->m_SectionName, "global") &&
         !strcmp(it.current()->Value("path", NULL), m_Path)))
    {
				m_ShareName->insertItem(it.current()->m_SectionName);
		}
	}
	
	if (m_ShareName->count() > 1)
		m_RemoveShareButton->show();
	else
		m_RemoveShareButton->hide();

	if (m_ShareName->count() == 0)
	{
		m_ShareNameEdit->show();
		
		m_ShareName->hide();
		m_NewShareButton->hide();
		
    EnableSharedAsControls(FALSE);
	}
	else
	{
		m_ShareNameEdit->hide();
		m_ShareName->show();
		m_NewShareButton->show();
		
    EnableSharedAsControls(TRUE);
		OnComboChange(0);
	}
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnMaximumAllowed()
{
  BOOL bOther = !m_MaximumAllowed->isChecked();
  m_Allow->setChecked(bOther);
	m_UsersMax->setEnabled(bOther);
}

////////////////////////////////////////////////////////////////////////////

void CSharingPage::OnAllow()
{
  BOOL bOther = !m_Allow->isChecked();
  m_MaximumAllowed->setChecked(bOther);
	m_UsersMax->setEnabled(!bOther);
}

////////////////////////////////////////////////////////////////////////////

BOOL CSharingPage::Apply()
{
	if (!m_SharedAs->isChecked())
	{
		QListIterator<CSection> it(m_Config);
		
		for (; it.current() != NULL; ++it)
		{
			if (stricmp(it.current()->m_SectionName, "global"))
			{
				if (!strcmp(it.current()->Value("path", NULL), m_Path))
				{
					LPCSTR UserName = it.current()->Value("user");

					if (NULL != UserName && 
							strlen(UserName) > 12 &&
							!strncmp(UserName, "$_SHAREUSER_", 12))
					{
						QString s;
						s.sprintf("deluser \"%s\"", UserName);
						ServerExecute(s);
					}
					
					it.current()->clear();
					it.current()->m_bChanged = TRUE;
				}
			}
		}
	}
	else
	{												  
		m_pCurrentSection->SetValue(m_Comment->text(), "comment");
		m_pCurrentSection->SetValue(m_MaximumAllowed->isChecked() ? "0" : (LPCSTR)(m_UsersMax->text()), "max connections");
		m_pCurrentSection->SetAvailable(m_Enabled->isChecked());

		if (m_ShareNameEdit->isVisible())
		{
			m_pCurrentSection->m_SectionName = m_ShareNameEdit->text();

			if (!m_pCurrentSection->m_SectionName.length())
			{
				QMessageBox::critical(this, caption(), LoadString(knENTER_SHARE_NAME));
				m_ShareNameEdit->setFocus();
				return FALSE;
			}

			if (m_pCurrentSection->m_SectionName.length() > 13)
			{
				QString a;
				a.sprintf(LoadString(knSTR_SHARE_TOOLONG),
						  (LPCSTR)m_pCurrentSection->m_SectionName);

				QMessageBox::critical(this, caption(), (LPCSTR)a);
				m_ShareNameEdit->setFocus();
				return FALSE;
			}
			
			// See if we already have share with that name
			
			QListIterator<CSection> it(m_Config);
		
			for (; NULL != it.current(); ++it)
			{
				if (!stricmp(it.current()->m_SectionName, m_pCurrentSection->m_SectionName))
				{
					break;
				}
			}

			if (NULL != it.current())
			{
				LPCSTR OldPath = it.current()->Value("path");

				if (NULL == OldPath)
					m_Config.remove(it.current());
				else
				{
					LPCSTR NewPath = m_pCurrentSection->Value("path");


					if (!strcmp(OldPath, NewPath))
					{
						QString a;
						a.sprintf(LoadString(knSTR_SHARE_DUP),
								  (LPCSTR)m_pCurrentSection->m_SectionName);

						QMessageBox::critical(this, caption(), (LPCSTR)a);
						goto DoSave;
					}
					else
					{
						QString a;
						
						a.sprintf(LoadString(knSTR_SHARE_DUP2),
							OldPath, (LPCSTR)m_pCurrentSection->m_SectionName,
							NewPath, (LPCSTR)m_pCurrentSection->m_SectionName);

						if (0 != QMessageBox::warning(this, caption(), (LPCSTR)a, LoadString(knYES), LoadString(knNO)))
							goto DoSave;

						//it.current()->Empty();
						it.current()->clear();
						it.current()->m_bChanged = TRUE;
					}
				}
			}
			
			m_pCurrentSection->m_bChanged = TRUE;
			m_Config.append(m_pCurrentSection);
		}
	}

	// Save any changes
DoSave:;
	QListIterator<CSection> it(m_Config);
	BOOL bChanged = FALSE;
	
	for (; it.current() != NULL; ++it)
	{
		if (it.current()->m_bChanged)
		{
			LPCSTR SecurityLevel = gSambaConfiguration.Value("security");

			if (it.current()->count() > 0 &&
					NULL != SecurityLevel &&
					!stricmp(SecurityLevel, "share")) // Share level security
			{
				LPCSTR UserName = it.current()->Value("user");
				QString s;
				
				if (it.current()->IsPublic())
				{
					if (NULL != UserName)
					{
						s.sprintf("deluser \"%s\"", UserName);
						it.current()->SetValue(NULL, "user");
					}
				}
				else
				{
					s.sprintf("adduser \"%s\" \"%s\"", UserName, it.current()->Value("sambapassword"));
					it.current()->SetValue(NULL, "sambapassword");
				}
				
				if (!s.isEmpty())
					ServerExecute(s);
			}
			
			if (!it.current()->Write())
			{
				if (NULL != m_pCurrentSection)
				{
					m_Config.removeRef(m_pCurrentSection);
					m_pCurrentSection = NULL;
				}
				
				m_Config.clear();
				ReReadSambaConfiguration();
				m_Config.Copy(gSambaConfiguration);
				EnableSharedAsControls(m_SharedAs->isChecked());

				return FALSE;
			}
			
			bChanged = TRUE;
		}
	}

	if (bChanged)
	{
		RestartSamba();
		gSambaConfiguration.Copy(m_Config);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////
