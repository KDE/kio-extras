/* Name: PromptDialog.cpp

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

#include "PromptDialog.h"

#define Inherited CPromptDialogData

#include <qfocusdata.h>
#include "qapplication.h"

CPromptDialog::CPromptDialog
(
	LPCSTR Text1,
	LPCSTR Text2,
	char *pResult,
	int nResultBufferSize,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	QString Caption(qApp->mainWidget()->caption());
	Caption += " ";
	setCaption(Caption);
	m_pResult = pResult;
	m_nResultBufferSize = nResultBufferSize;
	m_pLabel1->setText(Text1);
	m_pLabel2->setText(Text2);
	m_pLabel2->setBuddy(m_pEdit);
	m_pEdit->setEchoMode(QLineEdit::Password);
	m_pEdit->setFocus();

	m_pOKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));
	m_pOKButton->setDefault(TRUE);
	m_pOKButton->setAutoDefault(TRUE);

	connect(m_pEdit, SIGNAL(returnPressed()), SLOT(accept()));
}


CPromptDialog::~CPromptDialog()
{
}

BOOL CPromptDialog::Prompt(LPCSTR Text1, LPCSTR Text2, char *Result, int ResultSize)
{
	CPromptDialog dlg(Text1, Text2, Result, ResultSize);
	return dlg.exec();
}

void CPromptDialog::done(int r)
{
	if (r == 1)
	{
    int nLen = strlen(m_pEdit->text());

		if (nLen > m_nResultBufferSize-1)
		{
			strncpy(m_pResult, m_pEdit->text(), m_nResultBufferSize-1);
			m_pResult[m_nResultBufferSize-1] = '\0';
		}
		else
			strcpy(m_pResult, m_pEdit->text());
	}
	QDialog::done(r);
}

