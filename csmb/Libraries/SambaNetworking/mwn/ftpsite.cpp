/* Name: ftpsite.cpp

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

////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "common.h"
#include "ftpsite.h"
#include "PasswordDlg.h"
#include "ftpsession.h"

////////////////////////////////////////////////////////////////////////////

CFtpSiteItem::CFtpSiteItem(CListView *parent, LPCSTR Url) : 
	CFTPFileContainer(parent, NULL), m_URL(Url)
{
	InitPixmap();
	
	if (m_URL.length() < 6 || strnicmp(m_URL, "ftp://", 6))
		m_URL = "ftp://" + m_URL;

	SetCredentialsIndex();
}

////////////////////////////////////////////////////////////////////////////

CFtpSiteItem::CFtpSiteItem(CListViewItem *parent, LPCSTR Url) :
	CFTPFileContainer(parent, NULL), m_URL(Url)
{
	InitPixmap();
	
	if (m_URL.length() < 6 || strnicmp(m_URL, "ftp://", 6))
		m_URL = "ftp://" + m_URL;
	
	SetCredentialsIndex();
}

////////////////////////////////////////////////////////////////////////////

void CFtpSiteItem::SetCredentialsIndex(int nCredentialsIndex)
{
	if (-1 == m_URL.find('@'))
	{
		m_nCredentialsIndex = nCredentialsIndex;
		
		m_URL = "ftp://" + 
			gCredentials[m_nCredentialsIndex].m_UserName + 
			QString("@") + 
			m_URL.mid(6, m_URL.length()-6);
	}
}

void CFtpSiteItem::SetCredentialsIndex()
{
	ExtractCredentialsFromURL(m_URL, m_nCredentialsIndex);
	printf("%lx: In SetCredentialsIndex: %d\n", (long)this, m_nCredentialsIndex);
}

////////////////////////////////////////////////////////////////////////////

QString CFtpSiteItem::FullName(BOOL bDoubleSlashes) 
{ 
	return m_URL; 
}

////////////////////////////////////////////////////////////////////////////

int CFtpSiteItem::CredentialsIndex()
{ 
	//printf("Credentials index here is %d\n", m_nCredentialsIndex);
	return m_nCredentialsIndex; 
}
