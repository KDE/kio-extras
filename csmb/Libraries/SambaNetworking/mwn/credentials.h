/* Name: credentials.h

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

#ifndef __INC_CREDENTIALS_H__
#define __INC_CREDENTIALS_H__

#include <qstring.h>
#include "array.h"

class CCredentials
{
public:
	CCredentials()
	{
	}

    CCredentials(const QString& UserName, const QString& Password, const QString& Workgroup) :
	  m_UserName(UserName),
	  m_Password(Password),
	  m_Workgroup(Workgroup)
	{
	}

	CCredentials& operator=(const CCredentials& other)
	{
		m_UserName = other.m_UserName;
		m_Password = other.m_Password;
		m_Workgroup = other.m_Workgroup;

		return *this;
	}

	BOOL operator==(const CCredentials& other) const
	{
		return
			!stricmp(m_UserName
#ifdef QT_20
				.latin1()
#endif
                        , other.m_UserName
#ifdef QT_20
				.latin1()
#endif
                        ) &&
			!stricmp(m_Workgroup
#ifdef QT_20
				.latin1()
#endif
                        , other.m_Workgroup
#ifdef QT_20
				.latin1()
#endif
                        );
	}

	QString m_UserName;
	QString m_Password;
	QString m_Workgroup;
};

class CCredentialsArray : public CVector<CCredentials,CCredentials&>
{
public:
	int Find(const CCredentials& what)
	{
		int i;

		for (i=count()-1; i >= 0; i--)
		{
			if ((*this)[i] == what)
				break;
		}

		return i;
	}
};

extern CCredentialsArray gCredentials;

#endif /* __INC_CREDENTIALS_H__ */
