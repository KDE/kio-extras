/* Name: NewShareDialog.h

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

	File: NewShareDialog.h
	Last generated: Tue Nov 17 05:33:21 1998

 *********************************************************************/

#ifndef CNewShareDialog_included
#define CNewShareDialog_included

#include "common.h"
#include "inifile.h"
#include "NewShareDialogData.h"

class CNewShareDialog : public CNewShareDialogData
{
  Q_OBJECT

public:

  CNewShareDialog
  (
    LPCSTR Path,
    BOOL bIsPrinter,
		CSectionList& Config,
		QWidget* parent = NULL,
    const char* name = NULL
  );

  virtual ~CNewShareDialog();

protected slots:
	void OnPermissions();
	void done(int r);
private:
	CSection m_Section;
	CSectionList& m_Config;
  BOOL m_bIsPrinter;
};
#endif // CNewShareDialog_included

