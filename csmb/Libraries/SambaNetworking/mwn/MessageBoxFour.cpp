/* Name: MessageBoxFour.cpp

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

#include "MessageBoxFour.h"
#include "qmessagebox.h"
#include "qpixmap.h"

#define Inherited CMessageBoxFourData

CMessageBoxFour::CMessageBoxFour
(
	LPCSTR pText,
	int nButton1ID,
	int nButton2ID,
	int nButton3ID,
	int nButton4ID,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_pIcon->setPixmap(QMessageBox::standardIcon(QMessageBox::Warning, WindowsStyle));
	m_pText->setText(pText);
	m_pButton1->setText(LoadString(nButton1ID));
	m_pButton2->setText(LoadString(nButton2ID));
	m_pButton3->setText(LoadString(nButton3ID));
	m_pButton4->setText(LoadString(nButton4ID));
	m_pButton1->setDefault(TRUE);
	m_pButton1->setAutoDefault(TRUE);
}


CMessageBoxFour::~CMessageBoxFour()
{
}

void CMessageBoxFour::OnButton1()
{
	done(1);
}

void CMessageBoxFour::OnButton2()
{
	done(2);
}

void CMessageBoxFour::OnButton3()
{
	done(3);
}

void CMessageBoxFour::OnButton4()
{
	done(4);
}
