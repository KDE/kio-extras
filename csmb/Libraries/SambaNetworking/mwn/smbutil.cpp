/* Name: smbutil.cpp

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

#include "qapplication.h"
#include <stdio.h>
#include "smbutil.h"
#include "clientparser.h"
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbfile.h"
#include <stdlib.h>
#include "notifier.h"
#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qfileinfo.h>
#include "mapdialog.h"
#include <qmessagebox.h>

#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> // for getuid, getgid
#include <pwd.h>
#include <grp.h>
#include <netdb.h> // for gethostbyname
#include <ctype.h> // for toupper
#include <sys/mount.h>
#include <mntent.h>
#include <errno.h>
#include <sys/wait.h> // for waitpid
#include "PasswordDlg.h"
#include "inifile.h"
#include <dirent.h>
#include <sys/param.h> // for MAXPATHLEN
#include "sys/sysmacros.h" // for major(), minor()
#include "ftpsession.h"
#include <ctype.h>
#include <time.h>
#include "acttask.h"
#include <netinet/in.h>
#include <arpa/inet.h>

#define SMB_IOC_GETMOUNTUID _IOR('u', 1, unsigned short /*uid_t*/)

BOOL GetBroadcastWorkgroupList();
QString GetWorkgroupByIP(LPCSTR IP);

////////////////////////////////////////////////////////////////////////////

/*
 Retrieve NetBIOS name of the master of specified domain.
 Method:
     nmblookup <Workgroup>#1B -S -r
 or
     nmblookup <Workgroup>#1D -S -r
 will return a list of all registered NetBIOS names for
 the master. The one with <20> in it is the real NetBIOS name.
 We parse the output using pipe.
*/

////////////////////////////////////////////////////////////////////////////

void saveToFile(const char * text)
{
 QFile f("file.print");
 QString s;
 if ( f.open(IO_WriteOnly | IO_Append) )
 {
	 QTextStream t( &f );
   t <<text;
   f.close();
 }
}

QString GetMaster(LPCSTR Workgroup)
{
	CWaitLogo WaitLogo;
	QString s;
	int idcode=0x1b;
	FILE *f;

	BOOL bRFlag = FALSE;

TryAgain:;

	s.sprintf("nmblookup \"%s#%x\" -S", Workgroup, idcode);

	if (bRFlag)
		s += " -r";

	//printf("%s\n", (LPCSTR)s);

	f = popen(s,"r");

	s = "";

	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[2048];

			if (!WaitWithMessageLoop(f))
			{
				s = "__StoppedByUser__";
				break; // Stopped by user
			}

			fgets(buf, sizeof(buf), f);

			//printf("%s\n", buf);

			if (feof(f) || ferror(f))
				break;

			if (buf[0] == '\t' && Match(buf, "*<20>*"))
			{
				LPCSTR p = &buf[0];
				s = ExtractWord(p,"\t< ");
				break;
			}
		}
		pclose(f);
	}

	if (s.length() == 0 && !bRFlag)
	{
		bRFlag = TRUE;
		goto TryAgain;
	}

	if (s.length() == 0 && idcode == 0x1b)
	{
		bRFlag = FALSE;
		idcode = 0x1d;
		goto TryAgain;
	}

	if (s.length() == 0 && idcode == 0x1d)
	{
		bRFlag = FALSE;
		idcode = 0x1e;
		goto TryAgain;
	}

	return s;
}

////////////////////////////////////////////////////////////////////////////

QString GetIP(LPCSTR Service)
{
	CWaitLogo WaitLogo;
	QString s;

	QString Server = ExtractWord(Service, "/\\");

	FILE *f;

	s.sprintf("nmblookup \"%s\"", (LPCSTR)Server);

	f = popen(s, "r");

	s = "";

	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[2048];

			if (!WaitWithMessageLoop(f))
				break; // Stopped by user

			fgets(buf, sizeof(buf), f);

			if (feof(f) || ferror(f))
				break;

			if (Match(buf, "*<00>*"))
			{
				LPCSTR p = &buf[0];
				s = ExtractWord(p,"\t< ");
				break;
			}
		}

		pclose(f);
	}

	return s;
}

////////////////////////////////////////////////////////////////////////////
/*
  Retrieve a list of all servers from the specified workgroup (domain).
  Method:
	Use GetMaster(...) to retrieve name of the master browser of the workgroup.
	Use smbclient -L <master> -U<username>%<password> to retrieve the list.
	Parse the list and put the results into CServerArray.
*/

CSMBErrorCode GetServerList(LPCSTR Workgroup, LPCSTR MasterHint, CServerArray *pServerList, int nCredentialsIndex)
{
	CWaitLogo WaitLogo;
	CSMBErrorCode retcode = keSuccess;

	if (pServerList == NULL)
		return keWrongParameters;

	pServerList->clear();

  QString Master(GetMaster(Workgroup));

	if (Master == "__StoppedByUser__")
		return keStoppedByUser;

	if (Master.isEmpty())
		Master = MasterHint;

	if (Master.length() == 0)
		return keNetworkError;

	CSMBClientParser p;
	QString s;

	s.sprintf("smbclient -L \"%s\" -U \"%%\" -W \"%s\" -N",
            (LPCSTR)Master,
            Workgroup);
            //(LPCSTR)GetIP(Master));

	FILE *f;
	int retries=0;
	int ntimeouts = 0;

TryAgain:;

	f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, NULL, pServerList, NULL);
		//pclose(f);
		fclose(f);

		if (retcode == keStoppedByUser)
		{
			printf("Stopped by user!\n");
			return retcode;
		}

		if (p.IsTimeout() && ntimeouts < MAX_TIMEOUTS)
		{
			ntimeouts++;
			goto TryAgain;
		}
	}

	if (retries &&
      nCredentialsIndex &&
      keSuccess == retcode)
  {
		s.sprintf("setcred \"\\\\\\\\%s\" \"%s\" \"%s\" \"%s\"",
			(LPCSTR)Master,
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup);

		ServerExecute(s);
  }

  if (!retries && pServerList->count() == 0)
	{
    if (!nCredentialsIndex)
      s.sprintf("getcred \"\\\\\\\\%s\";smbclient -L \"%s\" -W \"$CURRENTWORKGROUP\" -U \"$CURRENTUSER\"",
        (LPCSTR)Master,
        (LPCSTR)Master);
    else
  		s.sprintf("setenv USER %s%%%s;smbclient -L \"%s\" -N -W \"%s\" -U \"%s\"",
  			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
  			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
  			(LPCSTR)Master,
  			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
  			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName);

		retries++;

		goto TryAgain;
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetShareList(LPCSTR Server,
				  CShareArray *pShareList,
				  int nCredentialsIndex)
{
	CWaitLogo WaitLogo;
	CSMBErrorCode retcode = keSuccess;

	if (pShareList == NULL)
		return keWrongParameters;

	pShareList->clear();

  CSMBClientParser p;
	QString s;

	if (!nCredentialsIndex)
	{
    s.sprintf("getcred \"\\\\\\\\%s\";smbclient -L \"%s\" -W \"$CURRENTWORKGROUP\" -U \"$CURRENTUSER\"",
			(LPCSTR)Server, (LPCSTR)Server);
	}
	else
		s.sprintf("setenv USER %s%%%s;smbclient -L \"%s\" -N -W \"%s\" -U \"%s\"",
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			Server,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName);

	FILE *f;
	int retries=0;
	int ntimeouts = 0;

TryAgain:;

	f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, NULL, NULL, pShareList);

		fclose(f);

		if (retcode == keStoppedByUser)
		{
			printf("Stopped by user!\n");
			return retcode;
		}

		if (nCredentialsIndex && keSuccess == retcode)
		{
			s.sprintf("setcred \"\\\\\\\\%s\" \"%s\" \"%s\" \"%s\"",
			(LPCSTR)Server,
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup);

			ServerExecute(s);
		}

		if (p.IsTimeout() && ntimeouts < MAX_TIMEOUTS)
		{
			ntimeouts++;
			goto TryAgain;
		}
	}

	if (p.IsAccessDenied() || !p.HadSomeData())
	{
		if (!retries)
		{
			s.sprintf("smbclient -L \"%s\" -U \"%%\"", Server);
			retries++;
			goto TryAgain;
		}
		else
			if (!p.IsAccessDenied())
				retcode = keNetworkError; // Error getting share list
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetWorkgroupList()
{
  // Special case to initialize netserv.
	// This will only happen once and if we don't use KDM and netserv is started by us.

	QString a;

	if (gCredentials[0].m_Workgroup == "%notset%")
	{
TryAgain:;
		{
			CPasswordDlg dlg("Microsoft Windows Network", gSambaConfiguration.Value("workgroup"), gCredentials[0].m_UserName);

			switch (dlg.exec())
			{
				case 1:
				{
					if (gCredentials[0].m_UserName != dlg.m_UserName)
					{
						a.sprintf("setenv DEFAULTUSER %s", (LPCSTR)dlg.m_UserName);
						ServerExecute(a);
						gCredentials[0].m_UserName = dlg.m_UserName;
					}

					gCredentials[0].m_Workgroup = dlg.m_Workgroup;
					a.sprintf("setenv DEFAULTWORKGROUP %s", (LPCSTR)gCredentials[0].m_Workgroup);
					ServerExecute(a);

					a.sprintf("setenv DEFAULTPASSWORD %s", (LPCSTR)dlg.m_Password);
					ServerExecute(a);

					void GetDefaultCredentials();

          GetDefaultCredentials();

					gMasterBrowser = "";
					GetMasterBrowser();

					if (gMasterBrowser.isEmpty())
					{
						printf("Master browser is empty!\n");
						goto TryAgain;
					}
				}
				break;

				default: // Quit or Escape
					gCredentials[0].m_Workgroup = "%notset%";
					a.sprintf("setenv DEFAULTWORKGROUP %s", "%notset%");
					ServerExecute(a);
					return keStoppedByUser;
			}
		}
	}


	CWaitLogo WaitLogo;

	int nCredentialsIndex = 0;

	gWorkgroupList.clear();

	CSMBClientParser p;
	QString s;
	CSMBErrorCode retcode = keNetworkError;

	s.sprintf("setenv USER %s%%%s;smbclient -L \"%s\" -N -U \"%s\" -W \"%s\"",
		(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
		(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
		(LPCSTR)gMasterBrowser,
		(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
		(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup);
    //(LPCSTR)GetIP(gMasterBrowser));

	FILE *f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, &gWorkgroupList, NULL, NULL);
		fclose(f);
	}

	if (keSuccess != retcode)
	{
    if (GetBroadcastWorkgroupList())
      return keSuccess;

    a.sprintf("setenv DEFAULTWORKGROUP %s", "%notset%");
		ServerExecute(a);

		goto TryAgain;
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode SMBRename(LPCSTR UNCPath, LPCSTR NewLabel, int nCredentialsIndex /* = 0 */)
{
	CSMBErrorCode retcode = keNetworkError;
	QString ShareName, Directory;

	GetShareAndDirectory(UNCPath, ShareName, Directory);

	ShareName = MakeSlashesBackwardDouble(ShareName);
  Directory = MakeSlashesBackwardDouble(Directory);

  QString NewPath(Directory);
  int nIndex = NewPath.findRev('\\');
  NewPath = NewPath.left(nIndex+1) + NewLabel;

	if (Directory.isEmpty())
		return keWrongParameters;


	QSTRING_WITH_SIZE(s, Directory.length() + strlen(NewLabel) + ShareName.length() * 2 + 256);

  FILE *f;
	CSMBClientParser p;

	if (!nCredentialsIndex)
	{
		s.sprintf("getcred \"%s\";smbclient \"%s\" -W \"$CURRENTWORKGROUP\" -c \"rename \\\"%s\\\" \\\"%s\\\"\"",
			(LPCSTR)ShareName,
			(LPCSTR)ShareName,
			(LPCSTR)Directory,
			(LPCSTR)NewPath);
  }
	else
		s.sprintf("setenv USER %s%%%s;smbclient \"%s\" -W \"%s\" -c \"rename \\\"%s\\\" \\\"%s\\\"\"",
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)ShareName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
			(LPCSTR)Directory,
			(LPCSTR)NewPath);

  //printf("[%s]\n", (LPCSTR)s);

	f = ServerOpen(s);

	if (NULL != f)
	{
		while (!feof(f))
    {
      retcode = p.ProcessLine(f);

      if (keSuccess != retcode)
        break;
    }

		fclose(f);
	}

	//printf("retcode = %d\n", retcode);

	return retcode;
}

CSMBErrorCode SMBMkdir(LPCSTR UNCPath, int nCredentialsIndex /* = 0 */)
{
	CSMBErrorCode retcode = keNetworkError;
	QString ShareName, Directory;

	GetShareAndDirectory(UNCPath, ShareName, Directory);

	ShareName = MakeSlashesBackwardDouble(ShareName);
  Directory = MakeSlashesBackwardDouble(Directory);

	if (Directory.isEmpty())
		return keWrongParameters;

	QSTRING_WITH_SIZE(s, 256 + Directory.length());

	FILE *f;
	CSMBClientParser p;

	if (!nCredentialsIndex)
	{
		s.sprintf("getcred \"%s\";smbclient \"%s\" -W \"$CURRENTWORKGROUP\" -c \"mkdir \\\"%s\\\"\"",
			(LPCSTR)ShareName,
			(LPCSTR)ShareName,
			(LPCSTR)Directory);
	}
	else
		s.sprintf("setenv USER %s%%%s;smbclient \"%s\" -W \"%s\" -c \"mkdir \\\"%s\\\"\"",
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)ShareName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
			(LPCSTR)Directory);

	f = ServerOpen(s);

	if (NULL != f)
	{
		while (!feof(f))
    {
      retcode = p.ProcessLine(f);

      if (keSuccess != retcode)
        break;
    }

		fclose(f);
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetFileList(LPCSTR UNCPath, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles /* = FALSE */, LPCSTR sFilter /* = NULL */)
{
	CWaitLogo WaitLogo;
	CSMBErrorCode retcode = keSuccess;

	if (pFileList == NULL)
		return keWrongParameters;

	pFileList->clear();

	CSMBClientParser p(bWantRegularFiles);
	QString ShareName, Directory;

	GetShareAndDirectory(UNCPath, ShareName, Directory);

	QString s;

	QString sLsCommand;

  if (NULL == sFilter)
    sLsCommand = "ls";
  else
    sLsCommand.sprintf("ls \\\"%s\\\"", sFilter);

  if (!nCredentialsIndex)
	{
		s.sprintf("getcred \"%s\";smbclient \"%s\" -W \"$CURRENTWORKGROUP\" -c \"%s\"",
			(LPCSTR)ShareName,
			(LPCSTR)ShareName,
      (LPCSTR)sLsCommand);
	}
	else
		s.sprintf("setenv USER %s%%%s;smbclient \"%s\" -W \"%s\" -c \"%s\"",
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)ShareName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
      (LPCSTR)sLsCommand);

	if (!Directory.isEmpty())
	{
		s.append(" -D \"");
		s.append(Directory);
		s.append("\"");
	}

	FILE *f;
	//int retries=0;
	int ntimeouts = 0;

TryAgain:;

//printf("%s\n",(LPCSTR)s);
  QString work = getenv("CURRENTWORKGROUP");
  saveToFile("CURRENTWORKGROUP = ");
  saveToFile((LPCSTR)work.latin1());
  saveToFile("\n");

  saveToFile("s = ");
  saveToFile((LPCSTR)s.latin1());
  saveToFile("\n");

	f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, pFileList);

		fclose(f);

		if (nCredentialsIndex && keSuccess == retcode)
		{
			s.sprintf("setcred \"%s\" \"%s\" \"%s\" \"%s\"",
			(LPCSTR)ShareName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup);

			ServerExecute(s);
		}

		if (p.IsTimeout() && ntimeouts < MAX_TIMEOUTS)
		{
			ntimeouts++;
			goto TryAgain;
		}
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetWorkgroupByServer(LPCSTR Server, QString& Workgroup)
{
	CWaitLogo WaitLogo;

	CSMBErrorCode retcode = keNetworkError;
	QString s;

	s.sprintf("smbclient -L \"%s\" -N -U \"%%\"", (LPCSTR)Server);

	//printf("%s\n", (LPCSTR)s);

	FILE *f = ServerOpen((LPCSTR)s);

	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[2048];

			if (!WaitWithMessageLoop(f))
			{
				printf("Stopped by user!\n");
				retcode = keStoppedByUser;
				break; // Stopped by user
			}

			fgets(buf, sizeof(buf), f);

			LPCSTR Tag = "Domain=[";
			LPCSTR pFound = strstr(buf, Tag);

			if (NULL != pFound)
			{
				LPCSTR p = &pFound[strlen(Tag)];
				Workgroup = ExtractWord(p,"] ");
				retcode = keSuccess;
				break;
			}
		}
		fclose(f);
	}

	if (Workgroup.isEmpty())
	{
		struct hostent *hp = gethostbyname(Server);

		if (NULL == hp)
		{
			printf("Unknown host %s!!\n", (LPCSTR)Server);
			return keUnknownHost;
		}

		in_addr a;
		memcpy(&a, hp->h_addr, hp->h_length);

		Workgroup = GetWorkgroupByIP(inet_ntoa(a));

		if (!Workgroup.isEmpty())
			retcode = keSuccess;
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetWorkgroupAndCommentByServer(LPCSTR Server, QString& Workgroup, QString& Comment)
{
	CWaitLogo WaitLogo;

	CSMBErrorCode retcode = keNetworkError;
	QString s;

	s.sprintf("smbclient -L \"%s\" -N -U \"%%\"", (LPCSTR)Server);

	//printf("%s\n", (LPCSTR)s);

	FILE *f = ServerOpen((LPCSTR)s);

	BOOL bWorkgroupFound = FALSE;

	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[2048];

			if (!WaitWithMessageLoop(f))
			{
				retcode = keStoppedByUser;
				break; // Stopped by user
			}

			fgets(buf, sizeof(buf), f);

			if (!bWorkgroupFound)
			{
				LPCSTR Tag = "Domain=[";
				LPCSTR pFound = strstr(buf, Tag);

				if (NULL != pFound)
				{
					LPCSTR p = &pFound[strlen(Tag)];
					Workgroup = ExtractWord(p,"] ");
					retcode = keSuccess;
					bWorkgroupFound = TRUE;
				}
			}

      if (buf[0] == '\t' && !strnicmp(buf+1, Server, strlen(Server)))
			{
				LPCSTR p = &buf[1+strlen(Server)];
				Comment = ExtractTail(p);
				break;
			}
		}
		fclose(f);
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

BOOL MountNFSShare(LPCSTR ServiceName,
									 LPCSTR MountPoint,
									 BOOL bReconnectAtLogon)
{
	QString cmd;

	cmd.sprintf("mount -o nosuid -t nfs \"%s\" \"%s\"",
								(LPCSTR)ServiceName + 6,
								(LPCSTR)MountPoint);

	char buf[1024];

	QString msg;
	msg.sprintf("Mounting share '%s'", (LPCSTR)ServiceName);

	int nRetcodeMount = SuperUserExecute(msg, cmd, buf, sizeof(buf)-1);

	if (nRetcodeMount && nRetcodeMount != -1024)
	{
		LPSTR p = &buf[0];

		if (!strncmp(p, "\r\nmount: ", 9))
			p += 9;

		*p = toupper(*p);

		cmd.sprintf(LoadString(knUNABLE_TO_MOUNT_X_Y), ServiceName, p);

		QMessageBox::critical(NULL,
													LoadString(knMOUNT_NETWORK_SHARE),
													(LPCSTR)cmd);

		return FALSE;
	}

	if (bReconnectAtLogon)
	{
		// Prepare the automount file entry.
		// For security, we don't write password there.
		// This entry is expected to be read by the Corel AutoMount application.

		cmd.sprintf("\"%s\" \"%s\"\n",
								(LPCSTR)ServiceName,
								(LPCSTR)MountPoint);

		if (FindAutoMountEntry(NULL, (LPCSTR)MountPoint))
			RemoveAutoMountEntry((LPCSTR)MountPoint);

		AddAutoMountEntry(cmd);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

BOOL MountSMBShare(LPCSTR UNCPath, LPCSTR Path, LPCSTR Workgroup, LPCSTR UserName, LPCSTR Password, BOOL bMakeAutoMount)
{
	BOOL bTriedEmpty = FALSE;

	if (NULL == gSmbUmountLocation)
		LocateSmbMount();

	QString Service(MakeSlashesForward(UNCPath));
	LPCSTR service = (LPCSTR)Service;

	QSTRING_WITH_SIZE(cmd, 1024+strlen(UNCPath)+strlen(Path));

DoAgain:;
	if (gbUseSmbMount_2_2_x)
	{
		extern QString gSmbMountVersion;

    cmd.sprintf(gSmbMountVersion.left(5) == "2.0.5" ?
                "setenv USER %s%%%s;smbmount \"%s\" \"%s\" -W \"%s\"" :
                "setenv USER %s%%%s;smbmount \"%s\" \"%s\" -o workgroup=\"%s\"%s",
								UserName,
								Password,
								service,
								(LPCSTR)EscapeString(Path),
								Workgroup,
								UserName == "" && Password == "" ? ",username=,password=" : "");
	}
	else
	{
		if (gbUseSmbMount_2_1_x)
		{
			cmd.sprintf("setenv USER %s%%%s;smbmount \"%s\" -c \"mount \\\"%s\\\" -u %u -g %u\" -W \"%s\"",
									UserName,
									Password,
									service,
									(LPCSTR)EscapeString(Path),
									getuid(),
									getgid(),
									Workgroup);
		}
		else
		{
			cmd.sprintf("smbmount \"%s\" \"%s\" -U \"%s\" -P \"%s\" -D \"%s\" -u %u -g %u",
									service,
									(LPCSTR)EscapeString(Path),
									UserName,
									Password,
									Workgroup,
									getuid(),
									getgid());

			QString IP = GetIP(service);

			if (!IP.isEmpty())
			{
				cmd.append(" -I ");
				cmd.append(IP);
			}
		}
	}

	//printf("CMD = [%s]\n", (LPCSTR)cmd);

  int status = ServerExecute(cmd);

  struct stat st;

  if (stat(Path, &st))
  {
    if (errno == EIO)
    {
      cmd.sprintf("%s \"%s\"", gSmbUmountLocation, Path);
      system(cmd);
      status = -256;
    }
    else
    {
      if (errno == EACCES)
      {
        QString Location(Path);
        URLEncode(Location);

        cmd.sprintf("fdumount %s", (LPCSTR)Location);
        ServerExecute(cmd);
        status = -256;
      }
    }
  }

  if (status == -256 || status == 256)   // Access denied
	{
		if (!bTriedEmpty)
		{
			UserName = "";
			Password = "";
			bTriedEmpty = TRUE;
			goto DoAgain;
		}

		CPasswordDlg dlg(service, Workgroup, UserName);

		if (1 == dlg.exec())
		{
			CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

			int nCredentialsIndex = gCredentials.Find(cred);

			if (nCredentialsIndex == -1)
			  nCredentialsIndex = gCredentials.Add(cred);

			Workgroup = gCredentials[nCredentialsIndex].m_Workgroup;
			UserName = gCredentials[nCredentialsIndex].m_UserName;
			gCredentials[nCredentialsIndex].m_Password = dlg.m_Password;
			Password = gCredentials[nCredentialsIndex].m_Password;
			goto DoAgain;
		}

		return FALSE;
	}

	if (!status && bMakeAutoMount)
	{
		// Prepare the automount file entry.
		// For security, we don't write password there.
		// This entry is expected to be read by the Corel AutoMount application.

		cmd.sprintf("\"%s\" \"%s\" \"%s\" \"%s\"\n", service, Path, UserName, Workgroup);

		if (FindAutoMountEntry(NULL, Path))
			RemoveAutoMountEntry(Path);

		AddAutoMountEntry(cmd);
	}

	return status == 0;
}

////////////////////////////////////////////////////////////////////////////

BOOL CanUmountSMB(LPCSTR mount_point)
{
	int fid = open(mount_point, O_RDONLY, 0);
	/*uid_t*/ unsigned short mount_uid;

	if (fid == -1)
	{
		printf("Could not open %s: %s\n", mount_point, strerror(errno));
		return FALSE;
	}

	if (ioctl(fid, SMB_IOC_GETMOUNTUID, &mount_uid) < 0)
	{
		printf("%s probably not smb-filesystem\n", mount_point);
		return FALSE;
	}

	close(fid);

	return (getuid() == 0) || (mount_uid == getuid());
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode UmountFilesystem(LPCSTR MountPoint, BOOL bIsNFS, QString &ErrorDescription)
{
	QString cmd;
	cmd.sprintf("umount \"%s\"", MountPoint);

	char buf[1024];

	int nRetcodeUmount = SuperUserExecute(bIsNFS ? "Umount NFS share" : "Unmount", cmd, buf, sizeof(buf)-1);

	if (nRetcodeUmount)
	{
		if (-1024 == nRetcodeUmount) // Cancelled by user
			return keStoppedByUser;

		LPSTR p = &buf[0];

		if (!strncmp(p, "\r\numount: ", 10))
			p += 10;

		*p = toupper(*p);

		ErrorDescription = p;

		return keErrorAccessDenied;
	}

	return keSuccess;
}

BOOL UmountSMBShare(LPCSTR MountPointPath)
{
	if (NULL == gSmbUmountLocation)
		LocateSmbMount();

	pid_t pid;
	int status;

	if ((pid = fork()) < 0)
	{
		printf("Unable to fork()\n");
		return FALSE;
	}
	else
	{
		if (pid == 0)
		{
			/* child */
			close(1); // close stdout
			close(2); // close stderr

			QString s;
			s.sprintf("%s \"%s\"", gSmbUmountLocation, (LPCSTR)EscapeString(MountPointPath));

			execl("/bin/sh", "sh", "-c", (LPCSTR)s, NULL);

			_exit(127);     /* execl error */
    }
		else
		{
			/* parent */
			while (waitpid(pid, &status, 0) < 0)
			{
				if (errno != EINTR)
				{
					status = -1;
					break;
				}
			}
    }
	}

	return status == 0;
}

////////////////////////////////////////////////////////////////////////////

BOOL OnMountNetworkShare(QString& MountPointPath, CMSWindowsNetworkItem *pOtherTree, int nCredentialsIndex, QWidget *pParent)
{
	BOOL retcode = FALSE;
	QString StartUNC;

	if (IsUNCPath(MountPointPath) || MountPointPath.left(6) == "nfs://")
	{
		StartUNC = MountPointPath;
		MountPointPath = "";
	}

	CMapDialog dlg(MountPointPath, StartUNC, TRUE, nCredentialsIndex, pOtherTree, pParent);

	if (!dlg.m_bAbort)
	{
		if (QDialog::Accepted == dlg.exec())
		{
			MountPointPath = dlg.m_MountPoint;

			int nCredentialsIndex = dlg.m_nCredentialsIndex;

			if (dlg.m_UNCPath.left(6) == "nfs://")
			{
				MountNFSShare(dlg.m_UNCPath, dlg.m_MountPoint, dlg.m_bReconnectAtLogon);
			}
			else
			{
				if (!MountSMBShare(dlg.m_UNCPath,
									 dlg.m_MountPoint,
									 gCredentials[nCredentialsIndex].m_Workgroup,
									 gCredentials[nCredentialsIndex].m_UserName,
									 gCredentials[nCredentialsIndex].m_Password,
									 dlg.m_bReconnectAtLogon))
				{
					QString s;
					s.sprintf(LoadString(knMOUNT_X_FAILED), (LPCSTR)dlg.m_UNCPath);

					QMessageBox::critical(pParent, LoadString(knMOUNT_NETWORK_SHARE), (LPCSTR)s);
				}
				else
					retcode = TRUE;
			}
		}
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

void RestartSamba()
{
	int fd = GetServerOpenHandle("smbreset");

	if (fd >= 0)
		close(fd);
}

////////////////////////////////////////////////////////////////////////////

void CopySMBDir(LPCSTR Src, LPCSTR Dst, int nCredentialsIndex)
{
	CWaitLogo WaitLogo;
	LPCSTR cmd = NULL;
	LPCSTR UNCPath = NULL;

	if (IsUNCPath(Src) && !IsUNCPath(Dst))	 // mget
	{
		UNCPath = Src;
		cmd = "mget";
		chdir(Dst);
	}
	else
	{
		if (IsUNCPath(Dst) && !IsUNCPath(Src)) // mput
		{
			UNCPath = Dst;
			cmd = "mput";
			chdir(Src);
		}
	}

	QString ShareName, Directory;

	GetShareAndDirectory(UNCPath, ShareName, Directory);

	QString ParentDir, TargetDir;

	SplitPath((LPCSTR)Directory, ParentDir, TargetDir);

	QString s;

	s.sprintf("%s%%%s", (LPCSTR)gCredentials[nCredentialsIndex].m_UserName, (LPCSTR)gCredentials[nCredentialsIndex].m_Password);
	setenv("USER", (LPCSTR)s, TRUE);

	s.sprintf("smbclient \"%s\" -W \"%s\" -c \"prompt;recurse;%s %s;exit\"",
		(LPCSTR)ShareName,
		(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
		cmd,
		(LPCSTR)TargetDir);

	if (!ParentDir.isEmpty())
	{
		s.append(" -D \"");
		s.append(ParentDir);
		s.append("\"");
	}

	FILE *f;
	//int retries=0; int ntimeouts = 0;

	f = ServerOpen(s);

	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[1024];

			if (!WaitWithMessageLoop(f))
				break; // Stopped by user

			fgets(buf, sizeof(buf), f);

			if (feof(f) || ferror(f))
				break;
		}

		fclose(f);
	}
}


////////////////////////////////////////////////////////////////////////////

void MakeFileCopyList(LPCSTR UNCPath,
					  CFileJob *pResult,
					  int nCredentialsIndex,
					  unsigned long &nTotalSize,
					  unsigned long &nFileCount)
{
	LPCSTR p = UNCPath + strlen(UNCPath) - 1;

	LPCSTR pStart = UNCPath+2;

	while (*pStart != '/' && *pStart != '\\' && *pStart != '\0')
		pStart++;

	if (*pStart == '\0')
		return; // Ooops, no share name

	pStart++; // Skip slash

	while (*pStart != '/' && *pStart != '\\' && *pStart != '\0')  // Now skip share name
		pStart++;

	while (p > pStart && *p != '\\' && *p != '/')
		p--;

	if (p <= pStart)
		p = UNCPath + strlen(UNCPath); // There's just share name, that's OK...

#if (QT_VERSION < 200)
  QString ParentDir(UNCPath, p - UNCPath + 1);
#else
  QString ParentDir = QString(UNCPath).left(p - UNCPath);
#endif

	if (*p != '\0')
		p++;

	CFileArray *pList = new CFileArray;

	if (keSuccess != GetFileList(ParentDir, pList, nCredentialsIndex, TRUE))
		return; // Oops, unable to get parent listing

	for (int i=0; i < pList->count(); i++)
	{
		CSMBFileInfo *pInfo = &((*pList)[i]);

		if (*p == '\0' || !stricmp(pInfo->m_FileName, p))
		{
			BOOL bIsFolder = (pInfo->IsFolder() > 0);

			nFileCount++;

			if (bIsFolder)
			{
				//printf("Found subfolder %s/%s\n", (LPCSTR)ParentDir, (LPCSTR)pInfo->m_FileName);
				MakeFileCopyList(ParentDir + "/" + pInfo->m_FileName + "/", pResult, nCredentialsIndex, nTotalSize, nFileCount);
			}
			else
			{
				//printf("Adding file %s/%s (size %s)\n", (LPCSTR)ParentDir, (LPCSTR)pInfo->m_FileName, (LPCSTR)pInfo->m_FileSize);

				nTotalSize += atol(pInfo->m_FileSize);
				pResult->append(new CFileJobElement(pInfo->m_FileName, pInfo->m_FileDate, (pInfo->IsFolder() ? S_IFDIR : S_IFREG), (size_t)atol(pInfo->m_FileSize), nFileCount, nTotalSize, 0, 1));

				//pResult->Add((*pList)[i]);
			}

			if (*p != '\0') // If there's a filename (i.e. *p != '\0') then we already found all we needed, break!
				break;
		}
	}

	delete pList;
}

////////////////////////////////////////////////////////////////////////////

CSMBFileInfo *FillFileInfo(CSMBFileInfo *pFileInfo, LPCSTR FileName, LPCSTR Shortname, struct stat *pStat, struct stat *pSttarget)
{
	struct stat st;
	struct stat sttarget;

	if (NULL == pStat)
	{
		lstat(FileName, &st);
		pStat = &st;

		if (S_ISLNK(pStat->st_mode))
		{
			if (stat(FileName, &sttarget))
				sttarget.st_mode = 0xffffffff;

			pSttarget = &sttarget;
		}
	}

	pFileInfo->m_FileName = Shortname;
	FileModeToString(pFileInfo->m_FileAttributes, pStat->st_mode);

	if (S_IFCHR == (pStat->st_mode & S_IFMT) || S_IFBLK == (pStat->st_mode & S_IFMT))
		pFileInfo->m_FileSize.sprintf("%d, %d", major(pStat->st_rdev), minor(pStat->st_rdev));
	else
		pFileInfo->m_FileSize.sprintf("%lu", pStat->st_size);

	// Slight optimization to avoid lookups for the same uid/gid

	static uid_t lastuid = (uid_t)-1;
	static QString lastusername;
	static gid_t lastgid = (gid_t)-1;
	static QString lastgroupname;

	if (lastuid != pStat->st_uid)
	{
		struct passwd *pw = getpwuid(pStat->st_uid);

		if (NULL != pw)
		{
			lastusername = pw->pw_name;
			lastuid = pStat->st_uid;
		}
	}

	pFileInfo->m_Owner = lastusername;

	if (lastgid != pStat->st_gid)
	{
		struct group *gr = getgrgid(pStat->st_gid);

		if (NULL != gr)
			lastgroupname = gr->gr_name;
	}

	pFileInfo->m_Group = lastgroupname;
	pFileInfo->m_FileDate = pStat->st_mtime;

	if (S_ISLNK(pStat->st_mode))
	{
		pFileInfo->m_TargetMode = pSttarget->st_mode;

		char bf[MAXPATHLEN+1];

		int nLength = readlink(FileName, bf, sizeof(bf)-1);

		if (nLength > 0)
		{
			bf[nLength] = '\0';
			pFileInfo->m_TargetName = bf;
		}
		else
			pFileInfo->m_TargetName = "<UNKNOWN>";
	}

	return pFileInfo;
}

////////////////////////////////////////////////////////////////////////////

BOOL GetLocalFileList(LPCSTR Path, CFileArray *pFileList, BOOL bWantRegularFiles)
{
	char *buf = new char[strlen(Path)+2];
	strcpy(buf, Path);

	if (buf[strlen(buf)-1] != '/')
		strcat(buf, "/");

  if (access(Path, X_OK) || access(Path, R_OK))
	{
		errno = EACCES;
		return TRUE; // no access!
	}

	DIR *thisDir = opendir(Path);

	if (thisDir == NULL)
	{
		printf("Unable to opendir %s\n", Path);
		return FALSE;
	}

	struct dirent *p;

	while ((p = readdir(thisDir)) != NULL)
	{
		if (!strcmp(p->d_name, ".") ||
			!strcmp(p->d_name, "..") ||
			(!gbShowHiddenFiles && p->d_name[0] == '.'))
			continue;

		struct stat st;
		struct stat sttarget;

		char *filename = new char[strlen(buf)+strlen(p->d_name)+1];

		strcpy(filename, buf);
		strcat(filename, p->d_name);

		BOOL bOK = !lstat(filename, &st);

    if (!bOK && (errno == EIO || errno == ETXTBSY || errno== EACCES || errno == ENOENT))
    {
      st.st_mode = S_IFREG;
      st.st_mtime =
      st.st_ctime =
      st.st_atime = time(NULL);
      st.st_size = 0;
      bOK = TRUE;
    }

    if (bOK)
		{
			if (S_ISLNK(st.st_mode))
			{
				if (stat(filename, &sttarget))
					sttarget.st_mode = 0xffffffff;
			}

			if ((bWantRegularFiles || S_ISDIR(st.st_mode) || (S_ISLNK(st.st_mode) && sttarget.st_mode != 0xffffffff && S_ISDIR(sttarget.st_mode))))
			{
				CSMBFileInfo fi;
				FillFileInfo(&fi, filename, p->d_name, &st, &sttarget);
				pFileList->Add(fi);
			}
		}
		delete []filename;

		qApp->processEvents();
	}

	closedir(thisDir);
	delete []buf;

  errno = 0;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

/* SMBFolderExists checks for the presence of folder on Microsoft Windows Network.
   Returns:
	-3 - exists but is not a folder
	-2 - permission denied
	-1 - doesn't exist
	0 - exists and writable
	1 - exists and not writable
*/

int SMBFolderExists(LPCSTR UNCPath, int nCredentialsIndex /* = 0 */)
{
	CSMBErrorCode retcode = keNetworkError;
	QString ShareName, Directory;

	GetShareAndDirectory(UNCPath, ShareName, Directory);

	ShareName = MakeSlashesBackwardDouble(ShareName);
  Directory = MakeSlashesBackwardDouble(Directory);

	if (Directory.isEmpty())
		return keWrongParameters;

	QString s;
	FILE *f;
	CSMBClientParser p(TRUE);

	s.sprintf("setenv USER %s%%%s;smbclient \"%s\" -W \"%s\" -c \"ls \\\"%s\\\"\"",
		(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
		(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
		(LPCSTR)ShareName,
		(LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
		(LPCSTR)Directory);

	CFileArray list;

	f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, &list);
		fclose(f);
	}

	if (retcode != keSuccess)
		return -2; // error, most likely permission denied

	if (list.count() == 0)
		return -1; // doesn't exist

	if (!list[0].IsFolder())
		return -3; // exists, but is not a folder

	if (list[0].m_FileAttributes.contains("R"))
		return 1; // exists and read-only

	return 0; // exists and writable
}

////////////////////////////////////////////////////////////////////////////

int SMBPrint(LPCSTR UNCPath, LPCSTR FileName, int nCredentialsIndex)
{
	//printf("%s %s\n", UNCPath, FileName);

	CSMBErrorCode retcode = keNetworkError;
	QString NetworkPath = MakeSlashesBackwardDouble(UNCPath);

	QString s;
	FILE *f;
	CSMBClientParser p(TRUE);

	if (!nCredentialsIndex)
    s.sprintf("getcred \"%s\";smbclient \"%s\" -P -W \"$CURRENTWORKGROUP\" -c \"print %s\"",
      (LPCSTR)NetworkPath,
		  (LPCSTR)NetworkPath,
		  (LPCSTR)FileName);
	else
    s.sprintf("setenv USER %s%%%s;smbclient \"%s\" -P -W \"%s\" -c \"print %s\"",
			(LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[nCredentialsIndex].m_Password,
  		(LPCSTR)NetworkPath,
		  (LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup,
		  (LPCSTR)FileName);

	CFileArray list;

	f = ServerOpen(s);

	if (NULL != f)
	{
		retcode = p.Read(f, &list);
		fclose(f);
	}

  if (nCredentialsIndex && keSuccess == retcode)
  {
    s.sprintf("setcred \"%s\" \"%s\" \"%s\" \"%s\"",
 		(LPCSTR)NetworkPath,
    (LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
    (LPCSTR)gCredentials[nCredentialsIndex].m_Password,
    (LPCSTR)gCredentials[nCredentialsIndex].m_Workgroup);

    ServerExecute(s);
  }

	return retcode;
}

BOOL GetBroadcastWorkgroupList()
{
  FILE *f = popen("nmblookup -M -", "r");

  if (NULL == f)
    return FALSE;

  char buf[100];
  const char Signature[] = "__MSBROWSE__\2<01>\n";

  QStrList IPs;

  while (!feof(f))
  {
    if (!WaitWithMessageLoop(f))
      break;

    fgets(buf, sizeof(buf)-1, f);

    if (feof(f))
      break;

    if (!strcmp(buf+strlen(buf)-sizeof(Signature)+1, Signature))
    {
      LPCSTR p = &buf[0];
      QString s = ExtractWord(p);
      IPs.append(s);
    }
  }

  pclose(f);

  QStrListIterator it(IPs);

  for (it.toFirst(); NULL != it.current(); ++it)
    GetWorkgroupByIP(it.current());

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////

QString GetWorkgroupByIP(LPCSTR IP)
{
  QString retval;
	pid_t pid;
	int input[2];

	pipe(input);

	if ((pid = fork()) < 0)
	{
		return retval; // Unable to fork()...
	}
	else
	{
		if (pid == 0)
		{
			/* child */
			dup2(input[1], fileno(stdout));
			close(input[0]);

      execl("/usr/bin/nmblookup", "nmblookup", "-A", IP, NULL);
    }
  }

	// parent

	gTasks.append(new CActiveTask(pid, NULL, NULL, 1)); // run for 1 second only!

	close(input[1]);

	/* now just read from input[0] */
	/* and write to output[1] */

	FILE *f = fdopen(input[0], "r");

	if (NULL != f)
	{
		setbuf(f, NULL);
    QString MasterName;

		while (!feof(f))
		{
			if (!WaitWithMessageLoop(f))
				break;

      char buf[100];
      fgets(buf, sizeof(buf)-1, f);

      if (feof(f))
        break;

      LPCSTR p = &buf[0];
      buf[strlen(buf)-1] = '\0';

      if (Match(buf, "*<20> -*"))
        MasterName = ExtractWord(p,"\t< ");
      else
      {
        if (Match(buf, "*<00> - <GROUP>*"))
        {
          QString s = ExtractWord(p,"\t<");

#if (QT_VERSION < 200)
          QString wg(s, 15);
#else
          QString wg = QString(s).left(15);
#endif
          wg = wg.stripWhiteSpace();
          wg = wg.lower();

#if (QT_VERSION < 200)
          wg[0] = toupper(wg[0]);
#else
          wg[0] = wg[0].upper();
#endif
					retval = wg;
					int n;

          for (n=0; n < gWorkgroupList.count(); n++)
          {
            if (gWorkgroupList[n].m_WorkgroupName == wg)
              break;
          }

          if (n == gWorkgroupList.count())
          {
            CSMBWorkgroupInfo wgi;
            wgi.m_WorkgroupName = wg;
            wgi.m_MasterName = MasterName;
            gWorkgroupList.Add(wgi);
          }
        }
      }
    }

    fclose(f);
  }

	return retval;
}

////////////////////////////////////////////////////////////////////////////

void GetMasterBrowser()
{
	if (gMasterBrowser.isEmpty())
  {
    gMasterBrowser = GetMaster(gCredentials[0].m_Workgroup);

    if (gMasterBrowser.isEmpty())
      gMasterBrowser = "localhost";
  }
}


