/* Name: ShareLevelAccessDialog.h

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


#ifndef CShareLevelAccessDialog_included
#define CShareLevelAccessDialog_included

#include "ShareLevelAccessDialogData.h"
#include "inifile.h"
#include <qbttngrp.h>

class CShareLevelAccessDialog : public CShareLevelAccessDialogData
{
	Q_OBJECT

public:

	CShareLevelAccessDialog
	(
		CSection *pSection,
		QWidget* parent = NULL,
		const char* name = NULL
	);
	
	virtual ~CShareLevelAccessDialog();
protected:
	CSection *m_pSection;
	QButtonGroup *m_pButtonGroup;
	bool m_bPasswordChanged;
protected slots:
	void done(int r);
	void OnAnonymousAccess();
	void OnPasswordAccess();
	void OnPasswordChanged_QT144(const char *);
	void OnPasswordChanged_QT20(const QString &);
};
#endif // CShareLevelAccessDialog_included
