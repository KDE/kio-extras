/* Name: ExportsEntryDialog.h

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

#ifndef CExportsEntryDialog_included
#define CExportsEntryDialog_included

#include "ExportsEntryDialogData.h"

class CExportsEntryDialog : public CExportsEntryDialogData
{
	Q_OBJECT

public:

	CExportsEntryDialog
	(
		const char *pHostname = NULL,
		const char *pAccessType = NULL,
		const char *pOptions = NULL,
		QWidget* parent = NULL,
		const char* name = NULL
	);
	
	virtual ~CExportsEntryDialog();
	void done(int r);

public:
	QString m_Hostname;
	QString m_AccessType;
	QString m_Options;
};

#endif // CExportsEntryDialog_included

