/* Name: PasswordDlg.h

   Description: This file is a part of the libmwn library.

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

#ifndef CPasswordDlg_included
#define CPasswordDlg_included

#include "PasswordDlgData.h"
#include "common.h"

class CPasswordDlg : public CPasswordDlgData
{
    Q_OBJECT

public:

    CPasswordDlg
    (
			LPCSTR UNCName,
			LPCSTR DefaultWorkgroup = NULL,
			LPCSTR DefaultUserName = NULL,
			BOOL bNeedAskDomain = TRUE,
			QWidget* parent = NULL,
      const char* name = NULL
    );

    virtual ~CPasswordDlg();
		void done(int r);
		QString m_UserName;
		QString m_Password;
		QString m_Workgroup;
};
#endif // CPasswordDlg_included
