/* Name: PromptDialog.h

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

	File: PromptDialog.h
	Last generated: Wed Jul 21 09:58:13 1999

 *********************************************************************/

#ifndef CPromptDialog_included
#define CPromptDialog_included

#include "common.h"
#include "PromptDialogData.h"

class CPromptDialog : public CPromptDialogData
{
	Q_OBJECT

public:

	CPromptDialog
	(
		LPCSTR Text1,
		LPCSTR Text2,
		char *pResult,
		int nResultBufferSize,
		QWidget* parent = NULL,
		const char* name = NULL
	);

	virtual ~CPromptDialog();

	static BOOL Prompt(LPCSTR Text1, LPCSTR Text2, char *Result, int ResultSize);

	char *m_pResult;
	int m_nResultBufferSize;

protected slots:
	
	void done(int r);
};

#endif // CPromptDialog_included
