/* Name: NetFilePropGeneral.h

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

	File: NetFilePropGeneral.cpp
	Last generated: Thu Jan 7 09:51:31 1999

 *********************************************************************/

#include "NetFilePropGeneral.h"
#include "common.h"
#include <stdlib.h>
#include "propres.h"

#define Inherited CNetFilePropGeneralData

CNetFilePropGeneral::CNetFilePropGeneral
(
	LPCSTR sFileName,
  LPCSTR sFullName,
  LPCSTR sFileSize,
  LPCSTR sModifyDate,
  LPCSTR sFileAttributes,
  QPixmap *pPixmap,
	QWidget* parent,
	const char* name
)
	:
	Inherited(parent, name)
{
	m_pFullNameLabel->setText(LoadString(knFULL_NAME_COLON));
	m_SizeLabel->setText(LoadString(knSIZE_COLON));
	m_pModifiedLabel->setText(LoadString(knMODIFIED_COLON));
  m_pAttributesLabel->setText(LoadString(knATTRIBUTES_COLON));

	m_IconLabel->setPixmap(*pPixmap);

	m_FileName->setText(sFileName);

	QString Path(sFullName);

	m_FullName->setText(Path);

	if (NULL != sFileSize && *sFileSize != '\0')
	{
		long l = atol(sFileSize);

		QString a;

		a.sprintf(LoadString(knX_Y_BYTES),
				(LPCSTR)SizeBytesFormat((double)l)
#ifdef QT_20
  .latin1()
#endif
        ,
				(LPCSTR)NumToCommaString(l)
#ifdef QT_20
  .latin1()
#endif
        );

		m_Size->setText(a);
	}
	else
		m_Size->setText("");

	m_ModifyDate->setText(sModifyDate);
	m_Attributes->setText(sFileAttributes);
}


CNetFilePropGeneral::~CNetFilePropGeneral()
{
}

