/* Name: ftpsession.cpp

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
#include "unistd.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "ftpsession.h"
#include <signal.h>
#include "PasswordDlg.h"
#include "ftpsite.h"
#include <qapplication.h>
#include <qmessagebox.h>
#include "kapp.h"
#include <ctype.h>

/*#define FTPDEBUG 1*/ // Uncomment this to get FTP debug info printed to stdout

#if (QT_VERSION >= 200)
#include "kstddirs.h"
#include "kglobal.h"
#endif

CFtpSessionList gFtpSessions;

////////////////////////////////////////////////////////////////////////////

mode_t ParseFileMode(LPCSTR sMode)
{
	mode_t mode = S_IFREG;
	
	mode_t Permissions[] = { 0, S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH };

	switch (*sMode++)
	{
		case 'd':
			mode = S_IFDIR;
		break;

		case 'l':
			mode = S_IFLNK;
		break;

		case '-':
			mode = S_IFREG;
		break;

		case 'c':
			mode = S_IFCHR;
		break;

		case 'b':
			mode = S_IFBLK;
		break;

		case 'p':
			mode = S_IFIFO;
		break;
	}

	for (int i=0; i < 6; i++)
	{
		if (*sMode++ != '-')
			mode |= Permissions[i];
	}

	return mode;
}

static void	TryFindSymlinks(CFileArray *pFileList, LPCSTR Url, BOOL bWantRegularFiles)
{
	int nSize = pFileList->count();

	for (int i=nSize-1; i >= 0; i--)
	{
		CSMBFileInfo *pFI = &((*pFileList)[i]);
		
		if (pFI->m_FileAttributes[0] == 'l')
		{
			if (!pFI->m_TargetName.contains("/"))
			{
        int j;

        for (j=0; j < nSize; j++)
				{
					if (j != i && (*pFileList)[j].m_FileName == pFI->m_TargetName)
					{
            pFI->m_TargetMode = ParseFileMode((*pFileList)[j].m_FileAttributes);
						
						if (!bWantRegularFiles && !S_ISDIR(pFI->m_TargetMode))
						{
							pFileList->RemoveAt(i); 
							nSize--;
						}
						break;
					}
				}

				if (!bWantRegularFiles && j == nSize) // remove it if not found!
				{
					pFileList->RemoveAt(i); 
					nSize--;
				}
			}
			else
			{
#ifdef FTPDEBUG
        printf("Trying IsFTPvalid %s\n", (LPCSTR)pFI->m_FileName);
#endif /* FTPDEBUG*/

				if (IsFTPValidFolder(QString(Url) + "/" + pFI->m_FileName))
					pFI->m_TargetMode = S_IFDIR | 0777;
				else
				{
					if (bWantRegularFiles)
						pFI->m_TargetMode = 0777;
					else
					{
						pFileList->RemoveAt(i); 
						nSize--;
					}
				}
			}
		}
	}
}

CListViewItem *CFtpSessionList::FindAndExpand(CListViewItem *pParent, LPCSTR Url)
{
	CWaitLogo WaitLogo;
	QString Hostname, Path;
	CListViewItem *pTargetItem, *child;
	int nCredentialsIndex;
	ParseURL(Url, Hostname, Path, nCredentialsIndex);
		
	QListIterator<CFtpSession> it(*this);
	
	CNetworkTreeItem *pSiteNode = NULL;

	for (; it.current() != NULL; ++it)
	{
		if (it.current()->m_URL == Hostname && it.current()->m_nCredentialsIndex == nCredentialsIndex)
		{
			pSiteNode = it.current()->m_pItem;
			break;
		}
	}

	if (pSiteNode == NULL)
		pSiteNode = new CFtpSiteItem(pParent, (LPCSTR)Hostname);

	// Step 2: Expand site node
	
	pTargetItem = (CListViewItem*)pSiteNode;

	pTargetItem->setOpen(TRUE);

  if (pSiteNode->ExpansionStatus() == keDeleteRequested)
  {
    delete pSiteNode;
    return NULL;
  }

	while (!Path.isEmpty())	// This will go deep to last folder
	{
		LPCSTR p = (LPCSTR)Path;

		QString Folder = ExtractWord(p,"\\/");
		Path = p;
		
		for (child = pTargetItem->firstChild(); child != NULL; child = child->nextSibling())
		{
			if (!stricmp(child->text(0), Folder))
				break;
		}

		if (child != NULL)
		{
			pTargetItem = child;
			pTargetItem->setOpen(TRUE);
		}
	}

	return pTargetItem;
}

////////////////////////////////////////////////////////////////////////////

CFtpSession *CFtpSessionList::GetSession(CNetworkTreeItem *pItem, 
																				 LPCSTR Url, 
																				 int /*nDefaultCredentialsIndex*/)
{
	QString Hostname;
	QString SiteRelativePath;
	int nCredentialsIndex;

	ParseURL(Url, Hostname, SiteRelativePath, nCredentialsIndex);

	QListIterator<CFtpSession> it(*this);
	
	// See if we have any idle sessions here

	for (; it.current() != NULL; ++it)
	{
		if (it.current()->m_Status == keFTP_Idle &&
			it.current()->m_URL == Hostname)
			return it.current();
	}

	// Create a new session then...

	// First find destination FTP site node in our tree
	if (NULL != pItem)
	{
		while (pItem->Kind() == keFTPFileItem && pItem != NULL)
			pItem = (CNetworkTreeItem*)pItem->parent();
	}

	//int nCredentialsIndex = (pItem == NULL) ? nDefaultCredentialsIndex : ((CNetworkTreeItem*)pItem)->CredentialsIndex();

	CFtpSession *pSession = new CFtpSession(pItem, Hostname, nCredentialsIndex);
 	append(pSession);
	
	if (pSession->Login() == keSuccess)
	{
		if (pSession->m_nCredentialsIndex != nCredentialsIndex && pItem->Kind() == keFTPSiteItem)
		{
			((CFtpSiteItem *)pItem)->SetCredentialsIndex(pSession->m_nCredentialsIndex);
		}
    
		return pSession;
	}

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

BOOL CFtpSession::ExecuteCommand(LPCSTR cmd)
{
	pid_t pid;
	int status = -1;
	int input[2], output[2], err[2];	
	
  pipe(input);
	pipe(output);
	pipe(err);

	if ((pid = fork()) < 0)
	{
#ifdef FTPDEBUG
		printf("Unable to fork\n");	
#endif /* FTPDEBUG*/
		exit(-1);
	}
	else
	{
		if (pid == 0)
		{
			/* child */
			dup2(output[0], STDIN_FILENO);		/* stdin */
			dup2(input[1], STDOUT_FILENO);		/* stdout */		
			dup2(input[1], STDERR_FILENO);		/* stderr */		
			
			close(output[1]);
			close(err[0]);		
			close(input[0]);
			
			execl("/bin/sh", "sh", "-c", cmd,  NULL);

			exit(127);     /* execl error */
		}
		else
		{
			m_Pid = pid;
		}
	}

	close(input[1]);	
	close(output[0]);	
	close(err[1]);
	
	/* now just read from input[0] */
	/* and write to output[1] */

	m_In  = fdopen(input[0], "r");
	m_Out = fdopen(output[1], "w");
	m_Err = fdopen(err[1], "r");
	
	if (m_Out != NULL)
		setbuf(m_Out, NULL);
#ifdef FTPDEBUG
	else
		printf("Ooops, m_Out = NULL!\n");
#endif /* FTPDEBUG */
	
  if (m_In != NULL)
		setbuf(m_In, NULL);
#ifdef FTPDEBUG
	else
		printf("Ooops, m_In = NULL!\n");
#endif /* FTPDEBUG */

	return (short)status;
}

////////////////////////////////////////////////////////////////////////////

LPCSTR SkipWords[] =
{
	"*ftp>*",
	NULL
};

LPCSTR LoginWords_1[] =
{
	"Name *: ",
  "421*",
	"ftp:*",
	NULL
};
	
LPCSTR LoginWords_2[] =
{
	"Password:",
	"530 *",
	"*ftp>*",
	NULL
};

LPCSTR LoginWords_3[] =
{
	"530 *",
	"230 *",
	"Password:*",
	"421*",
	"*ftp>*",
	NULL
};

LPCSTR ChdirWords[] =
{
	"250 *", // success
	"550 *: No such file or directory.*", // 550 XXX: No such file or directory.
	"550*Permission denied*", // 550 XXX: Permission denied
	"550 *: Not a directory.*", // 550 XXX: Not a directory.
	"550 *", // other failure
	"421 *", // 421 Timeout (xx seconds): closing control connection.
	"*ftp>*", // some other failure
	NULL
};

LPCSTR TransferWords[] =
{
	"150 *", // 150 Opening BINRY data connection for XXX (YYYY bytes)
	"Bytes transferred:*",
	"550 *", // permission denied
	"553 *", // permission denied (Overwrite).
	"*ftp>*",
	NULL
};

BOOL BytesAvailable(FILE *f)
{
	fd_set rfds;
	FD_ZERO(&rfds);
	int fd = fileno(f);
	FD_SET(fd, &rfds);
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 200;
	return select(fd+1, &rfds, NULL, NULL, &tv);
}

void CFtpSession::ReadInput(LPSTR buf, int nMaxCount)
{
	int cnt;

  for (cnt=0; !feof(m_In) && BytesAvailable(m_In) && cnt < nMaxCount; cnt++)
	{
    buf[cnt] = fgetc(m_In);
			
		if (buf[cnt] == '\n')
			break;
	}

	buf[cnt] = '\0';
}

int CFtpSession::Receive(LPCSTR *Keywords)
{
	if (m_In == NULL)
		return -1;
	
	while (1)
	{
		if (!WaitWithMessageLoop(m_In))
			return -1; //keStoppedByUser;
		
		ReadInput(m_MsgBuf, sizeof(m_MsgBuf)-1);
    
#ifdef FTPDEBUG
    if (strlen(m_MsgBuf) > 0)
    {
      printf("-->%s\n", m_MsgBuf);
    }
#endif /*FTPDEBUG*/

		for (int i=0; Keywords[i] != NULL; i++)
		{
      if (Match(m_MsgBuf, Keywords[i]))
			  return i;
		}
	}
}

BOOL CFtpSession::RetryAuthentication()
{
	CPasswordDlg dlg(m_URL, 
					NULL, 
					gCredentials[m_nCredentialsIndex].m_UserName,
					FALSE);
	
	if (1 == dlg.exec())
	{
		int nIndex = m_URL.findRev("@");

		QString URL = (-1 == nIndex) ? m_URL : m_URL.mid(nIndex+1, m_URL.length());
		
		CCredentials cred(dlg.m_UserName,dlg.m_Password, (LPCSTR)URL);
		
		m_nCredentialsIndex = gCredentials.Find(cred);
			
		if (m_nCredentialsIndex == -1)
		  m_nCredentialsIndex = gCredentials.Add(cred);

		gCredentials[m_nCredentialsIndex].m_Password = dlg.m_Password;
		
#ifdef FTPDEBUG
		printf("user %s %s\n", 
			(LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[m_nCredentialsIndex].m_Password);
#endif
		
		fprintf(m_Out, "user %s %s\n", 
			(LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName,
			(LPCSTR)gCredentials[m_nCredentialsIndex].m_Password);

		return TRUE;
	}

	return FALSE;
}

CSMBErrorCode CFtpSession::Login()
{
	CWaitLogo WaitLogo;
  CSMBErrorCode retcode = keErrorAccessDenied;

	int nIndex = m_URL.findRev("@");

	QString URL = (-1 == nIndex) ? m_URL : m_URL.mid(nIndex+1, m_URL.length());

RetryLogin:;
  
  fprintf(m_Out, "open %s\n", (LPCSTR)URL);

#ifdef FTPDEBUG
	printf("open %s\n", (LPCSTR)URL);
#endif /* FTPDEBUG */

	switch (Receive(LoginWords_1))
	{
		case -1:
		{
Aborted:;			
      retcode =	keStoppedByUser;
#ifdef FTPDEBUG
      printf("FTP aborted by user\n");
#endif /* FTPDEBUG */
    }
		break;

		case 0:		// Name:
		{
#ifdef FTPDEBUG
			printf("%s\n", (LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName);
#endif			
      fprintf(m_Out, "%s\n", (LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName);
			
			switch (Receive(LoginWords_2))
			{
				case -1:
					goto Aborted;

				case 2: // login without a password
					m_Status = keFTP_Idle;
					Chdir("/");
					fprintf(m_Out, "binary\n");
					Receive(SkipWords);
				return keSuccess;

				case 1: // 530 User xxx access denied...
				{
					Receive(SkipWords);
							
					if (RetryAuthentication())
						goto DoAgain;
				}
				break;

				case 0:
				{
SubmitPassword:;
					fprintf(m_Out, "%s\n", (LPCSTR)gCredentials[m_nCredentialsIndex].m_Password);

DoAgain:;					
					switch (Receive(LoginWords_3))
					{
						case 0:  // 530 Login incorrect
						{
							Receive(SkipWords);
							
              if (RetryAuthentication())
								goto DoAgain;
						}
						break;

						case 1:	// 230 login OK, skip all
							Receive(SkipWords);
							m_Status = keFTP_Idle;
							Chdir("/");
							
              fprintf(m_Out, "binary\n");
							Receive(SkipWords);
							
							//----------- TEMPORARY CODE ------------
              /*
              fprintf(m_Out, "idle 30\n");
              Receive(SkipWords);
              */
              //----------- END OF TEMPORARY CODE ------------
							
            return keSuccess;

            case 2: // Password:
							goto SubmitPassword; 

						case 3: // Server closed connection
							goto ServerClosedConnection;
					}
				}
        break;
			}
		}
    break;

    case 1:     // 421 (Server closed connection)
    {
ServerClosedConnection:;      
			QString msg;
			
      msg.sprintf(LoadString(knSTR_UNABLE_TO_ESTABLISH_FTP_SESSION), 
				(LPCSTR)URL, 
				(LPCSTR)LoadString(knECONNREFUSED));
      
      if (strlen(m_MsgBuf) > 4)
      {
         msg += LoadString(knREASON_GIVEN_BY_SERVER);
         msg += &m_MsgBuf[4];
      }
      
      Receive(SkipWords);
          
      if (QMessageBox::Retry == 
          QMessageBox::critical(qApp->mainWidget(), 
                                "FTP", 
                                (LPCSTR)msg, 
                                QMessageBox::Retry | QMessageBox::Default, 
                                QMessageBox::Abort | QMessageBox::Escape))
        goto RetryLogin;
    }
    break;

    default:
		{
			QString msg;

			msg.sprintf(LoadString(knSTR_UNABLE_TO_ESTABLISH_FTP_SESSION), 
				(LPCSTR)URL, 
				Match(m_MsgBuf, "*Unknown host*") ? (LPCSTR)LoadString(knSTR_SERVER_NOT_FOUND) : m_MsgBuf);

			QMessageBox::critical(qApp->mainWidget(), "FTP", (LPCSTR)msg, LoadString(knOK));
		}
	}

  gFtpSessions.removeRef(this);
	
  return retcode;
}

////////////////////////////////////////////////////////////////////////////

CFtpSession::CFtpSession(CNetworkTreeItem *pItem, LPCSTR Url, int nCredentialsIndex)
{
	m_URL = Url;

	m_nCredentialsIndex = nCredentialsIndex;
	m_pItem = pItem;
  InitFtpAgent();	
}

////////////////////////////////////////////////////////////////////////////

void CFtpSession::InitFtpAgent()
{
  m_Status = keFTP_Undefined;

#ifdef FTPDEBUG
	printf("Creating FTP session to %s\n", (LPCSTR)m_URL);
#endif	
  m_Pid = 0;
	
#if (QT_VERSION >= 200)
    QString FtpAgentLocation = KGlobal::dirs()->findResource("exe", "FtpAgent");
#else    
    QString FtpAgentLocation = KApplication::kde_bindir();
		
		if (FtpAgentLocation.right(1) != "/")
			FtpAgentLocation += "/";

		FtpAgentLocation += "FtpAgent";
#endif
	
	ExecuteCommand(FtpAgentLocation);
	Receive(SkipWords);
#ifdef FTPDEBUG
	printf("tick\n");
#endif	
  fprintf(m_Out, "tick\n");
	Receive(SkipWords);
}

////////////////////////////////////////////////////////////////////////////

CFtpSession::~CFtpSession()
{
	fprintf(m_Out, "bye\n");
  kill(m_Pid, SIGQUIT);
}
	
////////////////////////////////////////////////////////////////////////////

BOOL CFtpSession::Chdir(LPCSTR Folder)
{
	BOOL bRet = FALSE;

TryAgain:;
#ifdef FTPDEBUG
	printf("cd \"%s\"\n", Folder);	
#endif	
  fprintf(m_Out, "cd \"%s\"\n", Folder);
	
  switch (Receive(ChdirWords))
  {
    case 0: // success
      bRet = TRUE;
      Receive(SkipWords);
    break;
  
		case 1: // 	550 XXX: No such file or directory.
			Receive(SkipWords);
			errno = ENOENT;
		break;

		case 2:	// 550 XXX: Permission denied.
			Receive(SkipWords);
			errno = EPERM;
		break;

		case 3: // 550 XXX: Not a directory.
			Receive(SkipWords);
			errno = ENOTDIR;
		break;

		case 4: // Some other failure
			Receive(SkipWords);
    break;

		default: // Some unknown failure
    break;
  
    case 5: // Timeout
      kill(m_Pid, SIGQUIT);
      InitFtpAgent();
      
      if (keSuccess == Login())
        goto TryAgain;
    break;
  }

	return bRet;
}

////////////////////////////////////////////////////////////////////////////

LPCSTR LsWords[] =
{
	"150 *", // 150 opening ASCII data connection for /bin/ls
	"421 *", // 421 Timeout (xx seconds): closing control connection.
  "*ftp>*", // some failure
	NULL
};

LPCSTR MkdirWords[] = 
{
	"257 *", // MKD command successful
	"553 *", // 553: Could not determine cwdir: No such file or directory
	"550 *: File exists.*", // 550 XXX: File exists.
  "550 *", // 550: XXX: Permission denied..
	"421 *", // 421 Timeout (xx seconds): closing control connection.
	"*ftp>*", // something else that we didn't catch
	NULL
};

LPCSTR SizeWords[] =
{
  "213 *", // 213 NNNN
  "550 *: Permission denied.*", // 550 XXX: Permission denied.
	"550 *: not a plain file.*", // 550 XXX: not a plain file.
	"550 *: not a regular file.*", // 550 XXX: not a regular file.
	"550 *: No such file or directory.*", // 550 XXX: No such file or directory.
	"*ftp>*", // something else that we didn't catch
  NULL
};

BOOL CFtpSession::CountFolderContents(
	LPCSTR Url, 
	unsigned long& dwNumFiles, 
	unsigned long& dwNumFolders, 
	double& dwTotalSize, 
	CFileJob *pResult,
	BOOL bRecursive,
	BOOL bContainersGoLast,
	int BaseNameLength)
{
	CWaitLogo WaitLogo;

	QString Hostname, Path;
	struct stat st;
	
	int nCredentialsIndex;
	
	ParseURL(Url, Hostname, Path, nCredentialsIndex);

	if (FTPStat(Path, &st) < 0)
	{
#ifdef FTPDEBUG
		printf("Unable to stat %s\n", (LPCSTR)Path);
#endif		
    return FALSE; // Unable to stat source...
	}
	
	if (BaseNameLength == -1)
	{
		LPCSTR x = Url + strlen(Url) - 1;

		if (((*x == '/') || (*x == '\\')) && x > Url) // ignore trailing slash...
			x--;

		while (*x != '/' && *x != '\\' && x > Url)
			x--;

		BaseNameLength = (x - Url);
	}
	
	if (S_ISDIR(st.st_mode))
	{
		int CountNow = 0;
		
		if (NULL != pResult)
		{
			pResult->append(new CFileJobElement(Url, st.st_mtime, st.st_size, st.st_mode, dwNumFiles, dwTotalSize, BaseNameLength, 0));
      CountNow = pResult->count();
		}

		CFileArray *pList = new CFileArray;
		
		if (keSuccess != GetFTPFileList(Url, pList, m_nCredentialsIndex, TRUE))
			return FALSE; // Oops, unable to get listing
	
		if (BaseNameLength == 0)
			BaseNameLength = strlen(Url);
		
		for (int i=0; i < pList->count(); i++)
		{
			CSMBFileInfo *pInfo = &((*pList)[i]);
	
			BOOL bIsFolder = (pInfo->IsFolder() > 0);
				
			dwNumFiles++;
	
			QString NewPath = Url;
			
			if (pInfo->m_FileName[0] == '/')
			{
				NewPath = NewPath.left(NewPath.findRev('/'));
	
				pInfo->m_FileName = pInfo->m_FileName.mid(pInfo->m_FileName.findRev('/')+1, pInfo->m_FileName.length());
			}
				
			if (NewPath.right(1) != "/")
				NewPath += "/";
			
			NewPath += pInfo->m_FileName;
			
#ifdef FTPDEBUG
			printf("NewPath = %s, FileName = %s\n",(LPCSTR)NewPath, (LPCSTR)(pInfo->m_FileName));
#endif			
			if (bIsFolder)
			{
				dwNumFolders++;
				
#ifdef FTPDEBUG
				printf("Found subfolder %s\n", (LPCSTR)pInfo->m_FileName);
#endif				
				CountFolderContents(NewPath, dwNumFiles, dwNumFolders, dwTotalSize, pResult, bRecursive, bContainersGoLast, BaseNameLength);
			}
			else
			{
#ifdef FTPDEBUG
				printf("Adding file %s (size %s)\n", (LPCSTR)pInfo->m_FileName, (LPCSTR)pInfo->m_FileSize);
#endif				
				size_t FileSize = (size_t)atol(pInfo->m_FileSize);
				dwTotalSize += FileSize;
				
				if (NULL != pResult)
					pResult->append(new CFileJobElement(NewPath, 
																							pInfo->m_FileDate, 
																							FileSize, 
																							ParseFileMode(pInfo->m_FileAttributes),
																							dwNumFiles, 
																							dwTotalSize, 
																							BaseNameLength, 1));
			}
		}
	
		delete pList;
		
		if (NULL != pResult && 
				bContainersGoLast && 
				CountNow < (int)pResult->count()) // only add if that directory was not empty
			pResult->append(new CFileJobElement(Url, st.st_mtime, st.st_size, st.st_mode, dwNumFiles, dwTotalSize, BaseNameLength, 2));
	}
	else
	{
#ifdef FTPDEBUG
		printf("%s is a file...\n", (LPCSTR)Url);
#endif		
		dwTotalSize += st.st_size;
		dwNumFiles++;
		
		if (NULL != pResult)
			pResult->append(new CFileJobElement(Url, 
																					st.st_mtime, 
																					st.st_size, 
																					st.st_mode,
																					dwNumFiles, 
																					dwTotalSize,
																					BaseNameLength, 1));
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFtpSession::GetFTPFileList(LPCSTR Url, 
								 CFileArray *pFileList, 
								 int /*nCredentialsIndex */, 
								 BOOL bWantRegularFiles)
{
	CWaitLogo WaitLogo;

	QString Hostname, Folder;
	int nCred;
	
	ParseURL(Url, Hostname, Folder, nCred);
	
	if (Folder.right(1) != "/")
		Folder += "/";

	Folder += ".";
	
	if (!Chdir(Folder))
		return keErrorAccessDenied;
	
TryAgain:;

#ifdef FTPDEBUG
	printf("ls\n");
#endif	
  fprintf(m_Out, "ls\n");

	switch (Receive(LsWords))
  {
    case 0:
      break;
    
    case 1: // Timeout
    {
      kill(m_Pid, SIGQUIT);
      InitFtpAgent();

      CSMBErrorCode code = Login();
      
      if (code != keSuccess)
        return code; //keTimeoutDetected could be another way of doing it...
    }
#ifdef FTPDEBUG
    printf("Restoring ftp session\n");
#endif    
    goto TryAgain;
          
    case 2:
      return keErrorAccessDenied;
  }

	while (1)
	{
		char buf[1024];
		ReadInput(buf, sizeof(buf)-1);
		
#ifdef FTPDEBUG
		printf("%s\n", buf);
#endif		
		if (!strncmp(buf, "ftp> ", 5))
			break;

		if (strncmp(buf, "total ", 6) && 
				strncmp(buf, "226 ", 4) && 
				strncmp(buf, "226-", 4) &&
				strlen(buf) > 30)
		{
			LPCSTR s = &buf[0];

			CSMBFileInfo sfi;
				
			sfi.m_FileAttributes = ExtractWord(s);
			ExtractWord(s); // number
			sfi.m_Owner = ExtractWord(s); // UID
			sfi.m_Group = ExtractWord(s); // GID
			
			// Sometimes GID is missing.
			
			while (*s == ' ' || *s == '\t')
				s++;
			
			if (isdigit(*s))
			{
				sfi.m_FileSize = ExtractWord(s);
			}
			else
			{
				sfi.m_FileSize = sfi.m_Group;
				sfi.m_Group = "";
			}
			
			if (sfi.m_FileAttributes[0] == 'c' || sfi.m_FileAttributes[0] == 'b')
				sfi.m_FileSize += " " + ExtractWord(s);
			
			while (*s==' ')
				s++;

			sfi.m_FileDate = ParseDate(QString(s).left(12));
			s += 12;
			sfi.m_FileName = ExtractTail(s);

			// Deal with symlinks
			int nIndex = sfi.m_FileName.find(" -> ");
			
			if (-1 != nIndex)
			{
				sfi.m_TargetName = sfi.m_FileName.mid(nIndex+4, sfi.m_FileName.length());
				sfi.m_FileName = sfi.m_FileName.left(nIndex);
			}
			
			if (sfi.m_FileName != "." && sfi.m_FileName != ".." &&
					sfi.m_TargetName != "." && sfi.m_TargetName != ".." &&
				(bWantRegularFiles || sfi.IsFolder() || sfi.IsLink()))
				pFileList->Add(sfi);
		}
	}

	if (pFileList->count() == 0)
	{
#ifdef FTPDEBUG
		printf("size \"%s\"\n", (LPCSTR)Folder);
#endif		
    fprintf(m_Out, "size \"%s\"\n", (LPCSTR)Folder);
		
		// 550 XXX: Permission denied.
		// 550 XXX: not a plain file.
		// 550 XXX: No such file or directory.
		// 213 NNNN
		
		do
		{
			ReadInput(m_MsgBuf, sizeof(m_MsgBuf)-1);
		}
		while (strlen(m_MsgBuf) == 0);
		
#ifdef FTPDEBUG
		printf("**%s**\n", m_MsgBuf);
#endif
		
    if (Match(m_MsgBuf, "550*Permission denied*"))
		{
			Receive(SkipWords);
			return keErrorAccessDenied;
		}

		if (Match(m_MsgBuf, "550*No such file or directory.*"))
		{
			StartIdle();
			
			QString msg(m_MsgBuf+4);
			
			Receive(SkipWords);
			
			QMessageBox::critical(qApp->mainWidget(), PopupCaption(), (LPCSTR)msg, LoadString(knOK));

			StopIdle();
		}
		else
			Receive(SkipWords);
	}
	else
		TryFindSymlinks(pFileList, Url, bWantRegularFiles);

	return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFtpSession::GetFile(LPCSTR Source, LPCSTR Destination, LPFNStatusCallback pStatusCallback, void *UserData)
{
RetryGet:;
  
  if (!strcmp(Destination,"/"))
    Destination = "/.";
  
	if (IsValidFolder(Destination))
	{
		LPCSTR p = Source + strlen(Source) - 1;

		while (p > Source && *p != '/')
			p--;

		if (*p == '/')
			p++;

		QString DestinationFolder(Destination);
		
		if (DestinationFolder.right(1) != "/")
			DestinationFolder += "/";

#ifdef FTPDEBUG
		printf("get %s %s%s\n", Source, (LPCSTR)DestinationFolder, p);
#endif		
    fprintf(m_Out, "get \"%s\" \"%s%s\"\n", Source, (LPCSTR)DestinationFolder, p);
	}
	else
	{
#ifdef FTPDEBUG
		printf("get %s %s\n", Source, Destination);
#endif		
    fprintf(m_Out, "get \"%s\" \"%s\"\n", Source, Destination);
	}

	unsigned long dwByteCount = 0;

KeepGoing:;
		  
	switch (Receive(TransferWords))
	{
		case 0:
		{
			/*
			LPCSTR p = &m_MsgBuf[0];

			while (*p != '\0' && *p != '(')
				p++;

			if (*p == '(')
				dwFileSize += atol(p+1);
			*/
			goto KeepGoing;
		}
		break;

		case 1:
		{
			if (pStatusCallback != NULL)
			{
				unsigned long dwNow = atol(m_MsgBuf + 19);
				
				(*pStatusCallback)(UserData, dwNow - dwByteCount); // report increment only
				
				dwByteCount = dwNow;
			}
		}
		goto KeepGoing;

		case 2: // 550 XXX: Permission denied.
		{
			QString msg(m_MsgBuf+4);
			
			Receive(SkipWords);

			StartIdle();
			
			int retcode = QMessageBox::critical(qApp->mainWidget(), PopupCaption(), (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL));

			StopIdle();

			if (!retcode)
				goto RetryGet;

			return keErrorAccessDenied;
		}
		break;
	}

	return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFtpSession::PutFile(LPCSTR Source, LPCSTR Destination, LPFNStatusCallback pStatusCallback, void *UserData)
{
RetryPut:;
#ifdef FTPDEBUG
	printf("put \"%s\" --> \"%s\"\n", Source, Destination);
#endif	
	if (IsFTPValidFolder(Destination))
	{
		LPCSTR p = Source + strlen(Source) - 1;

		while (p > Source && *p != '/')
			p--;

		if (*p == '/')
			p++;

		QString DestinationFolder(Destination);
		
		if (DestinationFolder.right(1) != "/")
			DestinationFolder += "/";

#ifdef FTPDEBUG
		printf("put %s %s%s\n", Source, (LPCSTR)DestinationFolder, p);
#endif		
    fprintf(m_Out, "put \"%s\" \"%s%s\"\n", Source, (LPCSTR)DestinationFolder, p);
	}
	else
	{
#ifdef FTPDEBUG
		printf("put %s %s\n", Source, Destination);
#endif		
    fprintf(m_Out, "put \"%s\" \"%s\"\n", Source, Destination);
	}

	unsigned long dwByteCount = 0;

KeepGoing:;
		  
	switch (Receive(TransferWords))
	{
		case 0:
		{
			/*
			LPCSTR p = &m_MsgBuf[0];

			while (*p != '\0' && *p != '(')
				p++;

			if (*p == '(')
				dwFileSize += atol(p+1);
			*/
			goto KeepGoing;
		}
		break;

		case 1:
		{
			if (pStatusCallback != NULL)
			{
				unsigned long dwNow = atol(m_MsgBuf + 19);
				
				(*pStatusCallback)(UserData, dwNow - dwByteCount); // report increment only
				
				dwByteCount = dwNow;
			}
		}
		goto KeepGoing;

		case 2: // 550 XXX: Permission denied.
		case 3: // 553 XXX: Permission denied. (Overwrite)
		{
			QString msg(m_MsgBuf+4);
			
			Receive(SkipWords);

			StartIdle();
			
			int retcode = QMessageBox::critical(qApp->mainWidget(), PopupCaption(), (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL));

			StopIdle();

			if (!retcode)
				goto RetryPut;

			return keErrorAccessDenied;
		}
		break;
	}

	return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

BOOL CFtpSession::IsFTPValidFolder(LPCSTR name)
{
	return (FTPFolderExists(name) == 0);
}

////////////////////////////////////////////////////////////////////////////

BOOL CFtpSession::IsFTPValidFile(LPCSTR name)
{
	CWaitLogo WaitLogo;
	BOOL retcode = FALSE;

#ifdef FTPDEBUG
	printf("size \"%s\"\n", (LPCSTR)name);
#endif	
  fprintf(m_Out, "size \"%s\"\n", (LPCSTR)name);
	
  switch (Receive(SizeWords))
  {
    case 0: // 213 NNNN
      retcode = TRUE;
    break;

    case 1: // 550 XXX: Permission denied.
      errno = EPERM;
    break;

    case 2: // 550 XXX: not a plain file.
    case 3: // 550 XXX: not a regular file.
      errno = EISDIR;
    break;

    case 4: // 550 XXX: No such file or directory.
      errno = ENOENT;
    break;

    case 5: // something else that we didn't catch, no skipwords
    return FALSE;
  }
	
  Receive(SkipWords);
	return retcode;
}

////////////////////////////////////////////////////////////////////////////

/* FTPFolderExists checks for the presence of folder.
   Returns:
	-3 - exists but is not a folder
	-2 - permission denied
	-1 - doesn't exist
	0 - exists and writable
	1 - exists and not writable
*/

int CFtpSession::FTPFolderExists(LPCSTR name)
{
	CWaitLogo WaitLogo;

	if (Chdir(name))
		return 0; // exists and accessible
	
	if (errno == ENOENT)
	{
		return -1;
	}

	if (errno == EPERM)
	{
		return -2;
	}
	
	if (errno == ENOTDIR)
	{
		return -3;
	}

	return -2; // Unknown error, play it safe...
}

////////////////////////////////////////////////////////////////////////////

int CFtpSession::FTPMakeDir(LPCSTR name)
{
	CWaitLogo WaitLogo;
	int retcode = -1;

TryAgain:;	
#ifdef FTPDEBUG
	printf("mkdir \"%s\"\n", name);  
#endif	
  fprintf(m_Out, "mkdir \"%s\"\n", name);
	
	switch (Receive(MkdirWords))
	{
		case 0:  // MKD command successful
			retcode = 0;
		break;

    case 1:  // 553: Could not determine cwdir: No such file or directory
      errno = ENOTDIR;
    break;

    case 2:  // 550 XXX: File exists.
      errno = EEXIST;
    break;

    case 3: // 550: XXX: Permission denied..
      errno = EACCES;
    break;

    case 4:  // 421 Timeout (xx seconds): closing control connection.
    {
      kill(m_Pid, SIGQUIT);
      InitFtpAgent();

      CSMBErrorCode code = Login();
      
      if (keSuccess == code)
        goto TryAgain;

      errno = (keStoppedByUser == code ? EPERM : ECONNREFUSED);
    }
    return retcode; // no skipwords here...

    case 5: // something else that we didn't catch
      errno = EIO;
		return -1; // don't receive skipwords as we're already there...
	}

	Receive(SkipWords);
	return retcode;
}

////////////////////////////////////////////////////////////////////////////

BOOL IsFTPValidFolder(LPCSTR Url)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->IsFTPValidFolder(Path);
	}

	return FALSE;  // unable to establish session...
}

////////////////////////////////////////////////////////////////////////////

BOOL IsFTPValidFile(LPCSTR Url)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->IsFTPValidFile(Path);
	}

	return FALSE;  // unable to establish session...
}

////////////////////////////////////////////////////////////////////////////
// FTP analog of lstat system call
// returns zero on success and -1 in case of an error

int CFtpSession::FTPStat(LPCSTR name, struct stat *st)
{
	CWaitLogo WaitLogo;

	if (IsFTPValidFile(name) || EISDIR == errno)
	{
		if (EISDIR == errno)
		{
			if (!Chdir(name))
			{
#ifdef FTPDEBUG
				printf("Stat returns -1\n");
#endif				
        return -1;                      
			}

			fprintf(m_Out, "ls\n");
#ifdef FTPDEBUG
			printf("ls\n");
#endif
		}
		else
		{
			fprintf(m_Out, "ls \"%s\"\n", (LPCSTR)name);
#ifdef FTPDEBUG
			printf("ls \"%s\"\n", (LPCSTR)name);
#endif
		}

		if (Receive(LsWords))
			return -1;

		while (1)
		{
			char buf[1024];
			
			do
			{
				ReadInput(buf, sizeof(buf)-1);
			}
			while (strlen(buf) == 0);

			if (!strncmp(buf, "ftp> ", 5))
				break;

			if (strncmp(buf, "total ", 6) && strncmp(buf, "226 ", 4) && strlen(buf) > 30)
			{
				LPCSTR s = &buf[0];

				QString a = ExtractWord(s); // File attributes
        
        if (EISDIR == errno)
          a[0] = 'd';
				
        st->st_mode = ParseFileMode(a);

				///////////////////////////////////////

				st->st_nlink = atoi(ExtractWord(s)); // number of links?
				ExtractWord(s); // UID
				a = ExtractWord(s); // GID
				
				// Sometimes GID is missing.

				while (*s == ' ' || *s == '\t')
					s++;

				if (isdigit(*s))
				{
					a = ExtractWord(s);
				}

				st->st_size = atol(a);
				
				while (*s==' ')
					s++;

        st->st_ctime = st->st_atime = st->st_mtime = ParseDate(QString(s).left(12));
				
				Receive(SkipWords);
				return 0;
			}
		}
	}
	
  return -1;
}

////////////////////////////////////////////////////////////////////////////
/* FTPFolderExists checks for the presence of folder.
   Returns:
	-3 - exists but is not a folder
	-2 - permission denied
	-1 - doesn't exist
	0 - exists and writable
	1 - exists and not writable
*/

int FTPFolderExists(LPCSTR Url)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->FTPFolderExists(Path);        
	}

	return -1;  // unable to establish session...
}

////////////////////////////////////////////////////////////////////////////
// FTP analog of mkdir system call
// returns zero on success and -1 in case of an error

int FTPMakeDir(LPCSTR Url)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->FTPMakeDir(Path);
	}

	return -1;  // unable to establish session...
}

////////////////////////////////////////////////////////////////////////////

LPCSTR CFtpSession::PopupCaption()
{
		LPCSTR Caption = NULL;

		if (qApp->mainWidget() != NULL)
		{
			Caption = qApp->mainWidget()->iconText();

			if (NULL == Caption)
				Caption = qApp->mainWidget()->caption();
		}
		
		if (NULL == Caption)
			Caption = "FTP";

		return Caption;
}

////////////////////////////////////////////////////////////////////////////

// FTP analog of lstat system call
// returns zero on success and -1 in case of an error

int FTPStat(LPCSTR Url, struct stat *st)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->FTPStat(Path, st);
	}

#ifdef FTPDEBUG
	printf("Unable to establish session!\n");
#endif	
  return -1;  // unable to establish session...
}

// FTP analog of unlink system call
// returns zero on success and -1 in case of an error

int FTPUnlink(LPCSTR Url)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url);

	if (NULL != pSession)
	{
		QString Hostname, Path;
		int nCredentialsIndex;

		ParseURL(Url, Hostname, Path, nCredentialsIndex);

		return pSession->DeleteFile(Path);
	}

	return -1;  // unable to establish session...
}

////////////////////////////////////////////////////////////////////////////


LPCSTR RenameWords[] =
{
	"250 *", // RNTO command successful
	"553 *", // 553 XXX: Permission denied. (rename)
	"550 *", // 550 XXX: No such file or directory.
	"421 *", // 421 Timeout (xx seconds): closing control connection.
	"*ftp>*", // something else that we didn't catch
	NULL
};


LPCSTR DeleteWords[] =
{
	"250 *", // RNTO command successful
	"553 *", // 553 XXX: Permission denied. (rename)
	"550 *: Permission denied.*", // 550 XXX: Permission denied.
	"550 *: Directory not empty.*", // 550 XXX: Directory not empty.
	"550 *", // 550 XXX: No such file or directory.
	"421 *", // 421 Timeout (xx seconds): closing control connection.
  "*ftp>*", // something else that we didn't catch
	NULL
};

CSMBErrorCode CFtpSession::RenameFile(LPCSTR Url, LPCSTR Destination)
{
	QString Hostname, Path;
	int nCred;

	ParseURL(Url, Hostname, Path, nCred);
	
	QString Parent, OldName;

	SplitPath(Path, Parent, OldName);
	
	if (Parent.right(1) != "/")
		Parent += "/";

	Parent += Destination;
	
TryAgain:;

  fprintf(m_Out, "ren \"%s\" \"%s\"\n", (LPCSTR)Path, (LPCSTR)Parent);
#ifdef FTPDEBUG
	printf("ren \"%s\" \"%s\"\n", (LPCSTR)Path, (LPCSTR)Parent);
#endif
	CSMBErrorCode retcode = keSuccess;

	switch (Receive(RenameWords))
	{
		case 0:
			// OK
			retcode = keSuccess;
		break;

		case 1:
			retcode = keErrorAccessDenied;
		break;

		case 2:
			retcode = keFileNotFound;
		break;

    case 3:
    {
      kill(m_Pid, SIGQUIT);
      InitFtpAgent();

      CSMBErrorCode code = Login();
      
      if (code != keSuccess)
        return code; //keTimeoutDetected could be another way of doing it...
    }
    
#ifdef FTPDEBUG
    printf("Restoring ftp session\n");
#endif    
    goto TryAgain;

    case 4:
			return keErrorAccessDenied;
	}

	Receive(SkipWords);
	return retcode;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFtpSession::DeleteFile(LPCSTR Url)
{
	QString Hostname, Path;
	int nCred;

	ParseURL(Url, Hostname, Path, nCred);
	
TryAgain:;  
  fprintf(m_Out, "del \"%s\"\n", (LPCSTR)Path);

#ifdef FTPDEBUG
	printf("del \"%s\"\n", (LPCSTR)Path);
#endif

	CSMBErrorCode retcode = keSuccess;

	switch (Receive(DeleteWords))
	{
 		case 0:
			// OK
			retcode = keSuccess;
		break;

		case 1:
		case 2:
			retcode = keErrorAccessDenied;
		break;

		case 3:
			retcode = keDirectoryNotEmpty;
		break;

		case 4:
			retcode = keFileNotFound;
		break;

    case 5:
    {
      kill(m_Pid, SIGQUIT);
      InitFtpAgent();

      CSMBErrorCode code = Login();

      if (code != keSuccess)
        return code; //keTimeoutDetected could be another way of doing it...
    }

#ifdef FTPDEBUG
    printf("Restoring ftp session\n");
#endif    
    goto TryAgain;

    case 6:
			return keErrorAccessDenied;
	}

	Receive(SkipWords);
	return retcode;
}


