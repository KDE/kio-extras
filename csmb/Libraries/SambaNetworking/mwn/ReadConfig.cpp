/* Name: ReadConfig.cpp

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

#include <qstring.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "inifile.h"
#include "smbutil.h"
#include <qstrlist.h>
#include <unistd.h>
#include "kapp.h"
#include "exports.h"

////////////////////////////////////////////////////////////////////////////

QString gMasterBrowser;
QStrList gExcludeList;

CCredentialsArray gCredentials;
QString gSmbConfLocation;
QString gDefaultHomeDirectory;

BOOL gbSmbConfReadonly;
BOOL gbNetworkAvailable;

typedef const char * LPCSTR;

////////////////////////////////////////////////////////////////////////////

void SearchSmbConfLocation()
{
	gbNetworkAvailable = FALSE;
	
  LPCSTR PossibleLocationsSmbConf[] =
	{
		"/etc/samba/smb.conf",
		"/etc/smb.conf",
		"/usr/local/samba/lib/smb.conf"
	};

	int i;

  for (i=0; i < (int)(sizeof(PossibleLocationsSmbConf)/sizeof(LPCSTR)); i++)
	{
		if (!access(PossibleLocationsSmbConf[i], R_OK))
		{
			gSmbConfLocation = PossibleLocationsSmbConf[i];
      break;
    }
  }

  if (!gSmbConfLocation.isEmpty())
  {
    gbSmbConfReadonly = access(gSmbConfLocation, W_OK) ? 1 : 0;
	  gbNetworkAvailable = TRUE;
  }
}

////////////////////////////////////////////////////////////////////////////

void GetWorkgroupName(QString& val)
{
	LPCSTR ret = gSambaConfiguration.Value("workgroup");
	
	if (ret != NULL)
		val = ret;
}

////////////////////////////////////////////////////////////////////////////

void GetDefaultCredentials()
{
	// Initialize default credentials

	CCredentials cred;

	cred.m_Workgroup = GetServerVariable("DEFAULTWORKGROUP");
	
	if (cred.m_Workgroup.isEmpty())
		GetWorkgroupName(cred.m_Workgroup);
    
	cred.m_UserName = GetServerVariable("DEFAULTUSER");
	
	cred.m_Password = "$DEFAULTPASSWORD";

	if (gCredentials.count() > 0)
		gCredentials[0] = cred;
	else
		gCredentials.Add(cred);

	gDefaultHomeDirectory = GetServerVariable("HOME");

	if (gDefaultHomeDirectory.isEmpty())
	{
		gbNetworkAvailable = FALSE;

		if (getenv("HOME") != NULL)
			gDefaultHomeDirectory = getenv("HOME");
		else
			gDefaultHomeDirectory = "~";
	}

	// Initialize default FTP credentials for anonymous access
	// Index is always = 1
	
	cred.m_UserName = "anonymous";
	cred.m_Password = "CorelLinuxUser@";
	
	if (gCredentials.count() > 1)
		gCredentials[1] = cred;
	else
		gCredentials.Add(cred);
}

////////////////////////////////////////////////////////////////////////////

void ReReadSambaConfiguration()
{
	// Read smb.conf file

	gSambaConfiguration.clear(); // remove the old data (useful if we are doing refresh)
	
	if (gbNetworkAvailable)
	{
		gSambaConfiguration.Read(gSmbConfLocation);
	}
}

////////////////////////////////////////////////////////////////////////////

static void ReadExcludeList()
{
#if (QT_VERSION < 200)
  QString ExcludeFileName(KApplication::kde_datadir());

  if (ExcludeFileName.right(1) != "/")
    ExcludeFileName += "/";

  ExcludeFileName += "cfm/cfm.exclude";

  FILE *f = fopen(ExcludeFileName, "r");

  if (NULL != f)
  {
    char buf[1024];

    while (!feof(f))
    {
      fgets(buf, sizeof(buf)-1, f);

      if (feof(f))
        break;

      if (buf[strlen(buf)-1] == '\n')
        buf[strlen(buf)-1] = '\0';
      
      if (strlen(buf) > 0)
        gExcludeList.append(buf);
    }
    
    fclose(f);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////

void ReadConfiguration()
{
	if (gSmbConfLocation.isEmpty())
		SearchSmbConfLocation();
	
	LocateSmbMount();

	if (gbNetworkAvailable)
		ReReadSambaConfiguration();
	
	ReadNFSShareList();
	ReadExcludeList();

  // Set up credentials
	GetDefaultCredentials();
}

////////////////////////////////////////////////////////////////////////////

BOOL IsExcludedFromRescan(LPCSTR Path)
{
  static QString LastYes;
  static QString LastNo;

  if (LastYes == Path)  // cache last "yes" value
  {
    return TRUE;
  }
  
  if (LastNo == Path)  // cache last "no" value
  {
    return FALSE;
  }

  QStrListIterator it(gExcludeList);

  for (it.toFirst(); NULL != it.current(); ++it)
  {
    LPCSTR pCurrent = it.current();
    int l1 = strlen(pCurrent);
    int l2 = strlen(Path);
    
    if (l1 <= l2 && !strncmp(pCurrent, Path, l1))
    {
      if (l1 == l2 || Path[l1] == '/')
      {
        LastYes = Path;
        return TRUE;
      }
    }
  }

  LastNo = Path;
  return FALSE;
}

////////////////////////////////////////////////////////////////////////////

