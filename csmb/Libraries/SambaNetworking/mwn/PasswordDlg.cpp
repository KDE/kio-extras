/* Name: PasswordDlg.cpp

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

#include "PasswordDlg.h"
#include "smbworkgroup.h"
#include <unistd.h> // for gethostname();

#define Inherited CPasswordDlgData

///////////////////////////////////////////////////////////////////////////////

CPasswordDlg::CPasswordDlg
(
	LPCSTR UNCName,
	LPCSTR DefaultWorkgroup /*= NULL*/,
	LPCSTR DefaultUserName /*= NULL*/,
	BOOL bNeedAskDomain /* = TRUE */,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
  m_pUsernameLabel->setText(LoadString(knUSER_COLON));
  m_pUsernameLabel->setBuddy(m_pUsername);

	m_pPasswordLabel->setText(LoadString(knPASSWORD));
  m_pPasswordLabel->setBuddy(m_pPassword);

	if (bNeedAskDomain)
	{
		m_pDomainLabel->setText(LoadString(kn_DOMAIN_COLON));
		m_pDomainLabel->setBuddy(m_pDomain);


#ifndef QT_20
    m_pDomain->SetPixmap(LoadPixmap(keWorkgroupIcon));
#endif

    QStrList list;
    QString Value;
    int i;

    QString Hostname = GetHostName();
    list.inSort(Hostname);

    if (NULL == DefaultWorkgroup)
      Value = Hostname;
    else
    {
      Value = DefaultWorkgroup;
      list.inSort(DefaultWorkgroup);
    }

    for (i=0; i < gWorkgroupList.count(); i++)
      list.inSort(gWorkgroupList[i].m_WorkgroupName);

    m_pDomain->insertStrList(&list);

    QStrListIterator it(list);

    for (i=0; NULL != it.current(); ++it,++i)
    {
      if (!strcmp(it.current(), Value))
      {
        m_pDomain->setCurrentItem(i);
        break;
      }
    }
	}
	else
	{
		int newheight = height() - m_pOKButton->y() + m_pDomain->y();

		m_pOKButton->move(m_pOKButton->x(), m_pDomain->y());
		m_pCancelButton->move(m_pCancelButton->x(), m_pDomain->y());

		m_pDomainLabel->hide();
		m_pDomain->hide();

		setMinimumSize(width(), newheight);
		setMaximumSize(width(), newheight);
		resize(width(), newheight);
	}

	m_pTopLine1->setText(LoadString(knBAD_PASSWORD));
	m_pTopLine2->setText(UNCName);

	if (NULL != DefaultUserName)
	{
		m_pUsername->setText(DefaultUserName);
		m_pUsername->setSelection(0, strlen(DefaultUserName));
	}

	m_pOKButton->setText(LoadString(knOK));
	m_pOKButton->setAutoDefault(TRUE);
	m_pOKButton->setDefault(TRUE);
	m_pCancelButton->setText(LoadString(knCANCEL));

	m_pUsername->setFocus();
	m_pPassword->setEchoMode(QLineEdit::Password);

	setCaption(LoadString(knENTER_NETWORK_PASSWORD));
}

///////////////////////////////////////////////////////////////////////////////

CPasswordDlg::~CPasswordDlg()
{
}

///////////////////////////////////////////////////////////////////////////////

void CPasswordDlg::done(int r)
{
	if (r == 1)
	{
		m_UserName = m_pUsername->text();
		m_Password = m_pPassword->text();
		m_Workgroup = m_pDomain->currentText();

		if (m_Workgroup.isEmpty())
		{
			m_Workgroup = gCredentials[0].m_Workgroup;
		}
	}

	QDialog::done(r);
}

///////////////////////////////////////////////////////////////////////////////

