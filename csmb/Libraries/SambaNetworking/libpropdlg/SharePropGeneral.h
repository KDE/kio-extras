/* Name: SharePropGeneral.h

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

	File: SharePropGeneral.h
	Last generated: Wed Feb 3 13:40:06 1999

 *********************************************************************/

#ifndef CSharePropGeneral_included
#define CSharePropGeneral_included

#include "common.h"
#include "SharePropGeneralData.h"

class CShareItem;

class CSharePropGeneral : public CSharePropGeneralData
{
  Q_OBJECT

public:

  CSharePropGeneral::CSharePropGeneral
  (
  	LPCSTR sShareName,
    LPCSTR sFullName,
    LPCSTR sShareType,
    LPCSTR sShareComment,
  	QWidget* parent = NULL,
  	const char* name = NULL
  );

  virtual ~CSharePropGeneral();

  QPixmap *Pixmap()
  {
    return m_pPixmap;
  }

private:
  QPixmap *m_pPixmap;
};

#endif // CSharePropGeneral_included
