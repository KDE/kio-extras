/* Name: clientparser.cpp

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

#include <stdio.h>
#include "clientparser.h"
#include <ctype.h>

BOOL gbShowHiddenShares = FALSE;

typedef struct 
{
	LPCSTR m_Text;
	CSMBParserState m_State;
} CSMBParserKeyword;

static CSMBParserKeyword KeywordList[] =
{
	{ "This machine has a browse list:", keReadingServers },
	{ "*Server*Comment*", keReadingServers },
	{ "This machine has a workgroup list:", keReadingWorkgroups },
	{ "*Workgroup*Master*", keReadingWorkgroups },
	
	{ "\tSharename*", keReadingShares },
	{ "timeout connecting to", keTimeout },
	{ "*Access denied*", keAccessDenied },
  { "*ERRbadpw*", keAccessDenied },
  { "*session setup failed*", keServerError },
  { "*ERRSRV - ERRaccess*", keAccessDenied },
	{ "*ERRDOS - ERRbadfile*", keServerError },
};

int gKeywordListLength = sizeof(KeywordList) / sizeof(CSMBParserKeyword);

CSMBClientParser::CSMBClientParser(BOOL bWantRegularFiles /* = FALSE */)
{
	m_State = keNotReading;
	m_bStateActive = FALSE;
	m_pWorkgroupList = NULL;
	m_pServerList = NULL;
	m_pShareList = NULL;
	m_bWantRegularFiles = bWantRegularFiles;
	m_bHadSomeData = FALSE;
}

void CSMBClientParser::TestForKeywords(LPCSTR s)
{
	for (int i=0; i < gKeywordListLength; i++)
	{
		if (!strncmp(s, KeywordList[i].m_Text, strlen(KeywordList[i].m_Text)) ||
				Match(s, KeywordList[i].m_Text))
		{
			m_State = KeywordList[i].m_State;
			m_bStateActive = FALSE;
			break;
		}
	}
}

void CSMBClientParser::AddToLists(LPCSTR s)
{
	QString z(s);
	
	switch (m_State)
	{
		default:
		break;
		
		case keReadingShares:
			if (m_pShareList != NULL)
			{
				CSMBShareInfo shi;

				shi.m_ShareName = QString(s).left(15).stripWhiteSpace();
        s += 15;

				shi.m_ShareType = ExtractWord(s);
				shi.m_Comment = ExtractTail(s);
				
				if (gbShowHiddenShares || shi.m_ShareName[shi.m_ShareName.length()-1] != '$')
					m_pShareList->Add(shi);

				m_bHadSomeData = TRUE;
			}
		break;

		case keReadingServers:
			if (m_pServerList != NULL)
			{
				CSMBServerInfo svi;
				svi.m_ServerName = ExtractWord(s);
				svi.m_ServerName = svi.m_ServerName.lower();
#ifdef QT_20
        svi.m_ServerName[0] = svi.m_ServerName[0].upper();
#else        
        svi.m_ServerName[0] = toupper(svi.m_ServerName[0]);
#endif
				svi.m_Comment = ExtractTail(s);
				m_pServerList->Add(svi);
			}
		break;

		case keReadingWorkgroups:
			if (m_pWorkgroupList != NULL)
			{
				CSMBWorkgroupInfo wgi;
				QString wg(s);
				wg = wg.left(15).stripWhiteSpace();
				wg = wg.lower();
#ifdef QT_20
        wg[0] = wg[0].upper();
#else
        wg[0] = toupper(wg[0]);
#endif

				if (!wg.contains(' '))
				{
					wgi.m_WorkgroupName = wg;
					
					if (strlen(s) > 15)
					{
						s += 15;
						wgi.m_MasterName = ExtractTail(s);
					}
					
					m_pWorkgroupList->Add(wgi);
				}
			}
		break;
	}
}

CSMBErrorCode CSMBClientParser::ProcessDirLine(FILE *f, CFileArray *FileList)
{
	char buf[2048];

	if (!WaitWithMessageLoop(f))
		return keStoppedByUser;

	fgets(buf, sizeof(buf), f);
	
	//printf("%s", buf);

	if (feof(f))
		return keSuccess;
	    
	if (ferror(f))
		return keFileReadError;

	if (Match(buf, "*ERRDOS - ERRbadpath*") || 
      Match(buf, "*ERRDOS - ERRnoaccess*") ||
      Match(buf, "*ERRSRV - ERRbadpw*") ||
			Match(buf, "session setup failed:*"))
	{
		m_State = keAccessDenied;
		return keErrorAccessDenied;
	}

	if (Match(buf,"*: Unknown host *"))
	{
		m_State = keAccessDenied;
		return keUnknownHost;
	}

	if (buf[0] == ' ' && buf[1] == ' ')
	{
		CSMBFileInfo sfi;
		buf[strlen(buf)-1] = '\0'; /* eliminate newline */
		sfi.m_FileDate = ParseDate(&buf[0] + strlen(buf) - 24);
		buf[strlen(buf)-24] = '\0';
		QStringArray a;
		ExtractFromTail(&buf[0], 2, a);
		
		sfi.m_Owner = "root";
		sfi.m_Group = "root";

		if (a[2] != "." && a[2] != "..")
		{
			if (a[2].isEmpty())
			{
				sfi.m_FileAttributes = "";
				sfi.m_FileName = a[1];
			}
			else
			{
				sfi.m_FileAttributes = a[1];
				sfi.m_FileName = a[2];
			}

			BOOL bFolder = (sfi.m_FileAttributes.contains('D') > 0);
		        
			if (!bFolder)
			    sfi.m_FileSize = a[0];
			    
			if ((m_bWantRegularFiles || bFolder) && (gbShowHiddenFiles || !sfi.m_FileAttributes.contains('H')))
				FileList->Add(sfi);
		}
	}
	
	return keSuccess;
}

CSMBErrorCode CSMBClientParser::ProcessLine(FILE *f)
{
	char buf[2048];

	if (!WaitWithMessageLoop(f))
		return keStoppedByUser;

	fgets(buf, sizeof(buf), f);

	//printf("Line read: %s\n", buf);

	if (feof(f))
	    return keSuccess;
	    
	if (ferror(f))
		return keFileReadError;

	TestForKeywords(buf);
	
	if (IsTimeout())
		return keTimeoutDetected;

	if (IsServerError())
    return keNetworkError;

  if (IsAccessDenied())
	{
	  while (!ferror(f) && !feof(f))
    {
      fgets(buf, sizeof(buf), f);
    }
	    //printf("Access denied!\n");	
    return keErrorAccessDenied;
	}
	
  if (Match(buf, "*---*"))
		m_bStateActive = TRUE;
	else
		if (buf[0] == '\t' && m_bStateActive)
			AddToLists(buf);

	return keSuccess;
}

CSMBErrorCode CSMBClientParser::Read(FILE *f, 
										  CWorkgroupArray* pWorkgroupList,
										  CServerArray* pServerList, 
										  CShareArray* pShareList)
{
	m_State = keNotReading;
	m_pWorkgroupList = pWorkgroupList;
	m_pServerList = pServerList;
	m_pShareList = pShareList;

	CSMBErrorCode retcode = keSuccess;

	while (!feof(f))
	{
		retcode = ProcessLine(f);
			
		if (retcode != keSuccess)
			break;
	}

	return retcode;
}

CSMBErrorCode CSMBClientParser::Read(FILE *f, CFileArray *FileList)
{
	CSMBErrorCode retcode = keSuccess;
	
	m_State = keNotReading;

	while (!feof(f))
	{
		retcode = ProcessDirLine(f, FileList);
			
		if (retcode != keSuccess)
			break;
	}

	return retcode;
}

