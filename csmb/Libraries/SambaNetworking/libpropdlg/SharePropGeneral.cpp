/* Name: SharePropGeneral.cpp

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


/**********************************************************************

	--- Qt Architect generated file ---

	File: SharePropGeneral.cpp
	Last generated: Wed Feb 3 13:40:06 1999

 *********************************************************************/

#include "SharePropGeneral.h"
#include "propres.h"

#define Inherited CSharePropGeneralData

CSharePropGeneral::CSharePropGeneral
(
	LPCSTR sShareName,
  LPCSTR sFullName,
  LPCSTR sShareType,
  LPCSTR sShareComment,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_pPixmap = strcmp(sShareType, "Disk") ? LoadPixmap(kePrinterIconBig) : LoadPixmap(keClosedFolderIconBig);

  m_IconLabel->setPixmap(*m_pPixmap);
	
	m_Name->setText(sShareName);

	QString Path(sFullName);

	m_pFullNameLabel->setText(LoadString(knFULL_NAME_COLON));
	m_pTypeLabel->setText(LoadString(knTYPE_COLON));
	m_pCommentLabel->setText(LoadString(knCOMMENT_COLON));

	m_FullName->setText(Path);
	m_Type->setText(sShareType);
	m_Comment->setText(sShareComment);
}


CSharePropGeneral::~CSharePropGeneral()
{
}

