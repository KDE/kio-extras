/* Name: clientparser.h

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
      
#ifndef __INC_CLIENTPARSER_H__
#define __INC_CLIENTPARSER_H__

#include "common.h"
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbfile.h"

/* ---------------- Error codes --------------- */

typedef enum 
{ 
	keNotReading,
	keReadingShares, 
	keReadingServers, 
	keReadingWorkgroups,
	keTimeout,
	keAccessDenied,
  keServerError
} CSMBParserState;

class CSMBClientParser
{
public:
	CSMBClientParser(BOOL bWantRegularFiles = FALSE);

	CSMBParserState m_State;
	BOOL m_bStateActive;
	BOOL m_bWantRegularFiles;

	CWorkgroupArray *m_pWorkgroupList;
	CServerArray *m_pServerList;
	CShareArray *m_pShareList;

	CSMBErrorCode Read(
		FILE *f, 
		CWorkgroupArray* WorkgroupList,
		CServerArray* ServerList, 
		CShareArray* ShareList);

	CSMBErrorCode Read(FILE *f, CFileArray *FileList);
	
	void TestForKeywords(LPCSTR s);
	void AddToLists(LPCSTR s);
	CSMBErrorCode ProcessLine(FILE *f);
	CSMBErrorCode ProcessDirLine(FILE *f, CFileArray *FileList);
	BOOL IsTimeout()
	{
		return m_State == keTimeout;
	}

	BOOL IsAccessDenied()
	{
		return m_State == keAccessDenied;
	}

  BOOL IsServerError()
  {
    return m_State == keServerError;
  }

	BOOL HadSomeData()
	{
		return m_bHadSomeData;
	}
protected:
	BOOL m_bHadSomeData;
};

#endif /* __INC_CLIENTPARSER_H__ */
