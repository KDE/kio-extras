/* Name: MessageBoxFour.h

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: MessageBoxFour.h
	Last generated: Fri Nov 26 15:56:52 1999

 *********************************************************************/

#ifndef CMessageBoxFour_included
#define CMessageBoxFour_included

#include "common.h"
#include "MessageBoxFourData.h"

class CMessageBoxFour : public CMessageBoxFourData
{
	Q_OBJECT

public:

	CMessageBoxFour(LPCSTR pText, 
									int nButton1ID, 
									int nButton2ID, 
									int nButton3ID, 
									int nButton4ID,
									QWidget* parent = NULL,
									const char* name = NULL);

	virtual ~CMessageBoxFour();

protected slots:
	void OnButton1();
	void OnButton2();
	void OnButton3();
	void OnButton4();
};

#endif // CMessageBoxFour_included
