/* Name: filejob.cpp

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

#include "common.h"
#include <unistd.h>
#include "qapplication.h"
#include <sys/types.h>
#include <sys/stat.h> // for S_IRUSR, S_IWUSR etc.
#include <fcntl.h> // for O_CREAT, O_RDONLY etc.
#include <utime.h> // for utime and struct utimbuf
#include "qmessagebox.h"
#include <sys/param.h>
#include <time.h>
#include "ftpsession.h"
#include "smbfile.h"
#include "smbutil.h"
#include "ftpfile.h"
#include "PasswordDlg.h"
#include "FileReplaceDialog.h"
#include <errno.h>
#include "MessageBoxFour.h"
#include <ctype.h>
#include "inifile.h"
#include <sys/wait.h>

int gIdleTime;
BOOL gbIdling=FALSE;
BOOL gbAutoOverwriteMode = FALSE;

QString CFileJob::m_NowAtFile;
QString CFileJob::m_UnfinishedFile;

int CFileJob::m_nCount;

static time_t tIdleBegin;
static BOOL CreateTrashEntry(QString& DestinationFile, LPCSTR Source);
QString RandomName();

///////////////////////////////////////////////////////////////////////////////

void StartIdle()
{
	tIdleBegin = time(NULL);
	gbIdling = TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void StopIdle()
{
	time_t tNow = time(NULL);

	if (tNow > tIdleBegin)
		gIdleTime += (tNow - tIdleBegin);
	
	gbIdling = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL DoMount(LPCSTR p)
{
  QString ServiceName = ExtractQuotedWord(p); // Get UNC path
	QString MountPoint = ExtractQuotedWord(p); // get the mount point
	
	CFileJob::m_NowAtFile = ServiceName;
  
  QSTRING_WITH_SIZE(msg, 50 + 50 + 1024);
		    
  if (access(MountPoint, 0))
  {
		msg.sprintf(LoadString(knUNABLE_TO_MOUNT_NOMOUNTPOINT), 
                (LPCSTR)SqueezeString(ServiceName, 50), 
                (LPCSTR)SqueezeString(MountPoint, 50));
      
		int nRet = QMessageBox::critical(qApp->mainWidget(), 
													LoadString(knERROR), 
                          msg,
                          LoadString(knYES),
                          LoadString(knNO));

    if (!nRet)
    {
      RemoveAutoMountEntry(MountPoint);
    }
		
    return FALSE;
  }
	
  if (!CanMountAt(MountPoint))
	{
		msg.sprintf(LoadString(knUNABLE_TO_MOUNT_NO_PRIVILEGES), 
                (LPCSTR)SqueezeString(ServiceName, 50), 
                (LPCSTR)SqueezeString(MountPoint, 50));
      
		int nRet = QMessageBox::critical(qApp->mainWidget(), 
													LoadString(knERROR), 
                          msg,
                          LoadString(knYES),
                          LoadString(knNO));
    
    if (!nRet)
    {
      RemoveAutoMountEntry(MountPoint);
    }
		
    return FALSE;
	}
		
	if (ServiceName.left(6) == "nfs://")
	{
		MountNFSShare(ServiceName, MountPoint, FALSE);
	}
	else
	{
		QString UserName = ExtractQuotedWord(p); // get the user name
		QString Domain = ExtractQuotedWord(p); // get the domain
		
		MountSMBShare(ServiceName, MountPoint, Domain, UserName, "$DEFAULTPASSWORD", FALSE);
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

CSMBErrorCode DoAutoMount()
{
	char buf[1024];

	sprintf(buf, "%s/.automount", getenv("HOME"));

	FILE *f = fopen(buf, "r");

	if (NULL != f)
	{
		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			buf[strlen(buf)-1] = '\0';

			if (strlen(buf) > 0)
			{
				DoMount(buf);
			}
		}
		
		fclose(f);
	}

	return keSuccess;
}

///////////////////////////////////////////////////////////////////////////////

CSMBErrorCode DoAutoUmount()
{
	char buf[1024];

	sprintf(buf, "%s/.automount", getenv("HOME"));

	FILE *f = fopen(buf, "r");

	if (NULL != f)
	{
		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			buf[strlen(buf)-1] = '\0';

			if (strlen(buf) > 0)
			{
				LPCSTR p = &buf[0];

				QString ServiceName = ExtractQuotedWord(p);
				QString MountPoint = ExtractQuotedWord(p);

				CFileJob::m_NowAtFile = ServiceName;

				if (ServiceName.left(6) == "nfs://")
				{
					QString ErrorDescription;
					UmountFilesystem(MountPoint, TRUE, ErrorDescription);
				}
				else
					UmountSMBShare(MountPoint);
			}
		}
		
		fclose(f);
	}
	
	return keSuccess;
}

///////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFileJob::Run()
{
  if (m_Type == keFileJobMount)
	{
    return DoAutoMount();
	}
  
  if (m_Type == keFileJobUmount)
	{
    return DoAutoUmount();
	}
  
	if (m_Type == keFileJobMkdir)
	{
    return EnsureDirectoryExists(m_Destination) ? keSuccess : keStoppedByUser;
	}
	
	if (m_Type == keFileJobPrint)
	{
		//gbIdling = TRUE;

		int nCredentialsIndex = 0;
		CFileJobElement *pElement = first();
		
		while (pElement->m_nBaseNameLength == -1)
			pElement = next();
    
DoAgainSMBPrint:;
		
		switch (SMBPrint(m_Destination, pElement->m_FileName, nCredentialsIndex))
		{
			case keSuccess:
				break;

			default:
				return keNetworkError; // unexpected error

			case keErrorAccessDenied:
			{
				CPasswordDlg dlg((LPCSTR)m_Destination, NULL);

				switch (dlg.exec())
				{
					case 1: 
					{
						CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

						nCredentialsIndex = gCredentials.Find(cred);

						if (nCredentialsIndex == -1)
							nCredentialsIndex = gCredentials.Add(cred);
						else
							if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
								gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
					}
					goto DoAgainSMBPrint;

					default: // Quit or Escape
						return keStoppedByUser;
				}
			}
		}

    return keSuccess;
	}
	
	if (m_Type == keFileJobList)
	{
		CFileArray list;
		int nCredentialsIndex;
		
		if (IsFTPUrl(m_Destination))
		{
			ExtractCredentialsFromURL(m_Destination, nCredentialsIndex);

DoAgainFTP:;

			switch (GetFTPFileList(m_Destination, &list, nCredentialsIndex, 1))
			{
				case keSuccess:
					break;

				default:
					return keNetworkError; // unexpected error
				
				case keErrorAccessDenied:
				{
					CPasswordDlg dlg((LPCSTR)m_Destination, NULL);
		
					switch (dlg.exec())
					{
						case 1: 
						{
							CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);
				
							nCredentialsIndex = gCredentials.Find(cred);
				
							if (nCredentialsIndex == -1)
								nCredentialsIndex = gCredentials.Add(cred);
							else
								if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
									gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
						}
						goto DoAgainFTP;
		
						default: // Quit or Escape
							return keStoppedByUser;
					}
				}
			}
		}
		else
			if (IsUNCPath(m_Destination))
			{
				nCredentialsIndex = 0;
DoAgainSMB:;
				switch (GetFileList(m_Destination, &list, nCredentialsIndex, 1))
				{
					case keSuccess:
						break;

					default:
						return keNetworkError; // unexpected error

					case keUnknownHost:
					{
						QString msg;
						QString UNC = MakeSlashesBackward(m_Destination);

						msg.sprintf(LoadString(knX_NETWORK_PATH_NOT_FOUND), (LPCSTR)SqueezeString(UNC, 50));

						QMessageBox::critical(qApp->mainWidget(), (LPCSTR)UNC, (LPCSTR)msg, LoadString(knOK));
					}
					return keFileNotFound;

					case keErrorAccessDenied:
					{
						CPasswordDlg dlg((LPCSTR)m_Destination, NULL);
			
						switch (dlg.exec())
						{
							case 1: 
							{
								CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);
					
								nCredentialsIndex = gCredentials.Find(cred);
					
								if (nCredentialsIndex == -1)
									nCredentialsIndex = gCredentials.Add(cred);
								else
									if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
										gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
							}
							goto DoAgainSMB;
			
							default: // Quit or Escape
								return keStoppedByUser;
						}
					}
				}
			}
			else
				GetLocalFileList(m_Destination, &list, TRUE);
			
		int i;

		for (i=0; i < list.count(); i++)
		{
			QString zzz = (LPCSTR)list[i].m_FileName;
			URLEncode(zzz);
			
			QString attr = list[i].m_FileAttributes;

			if (attr.length() < 9)
			{
				attr = (attr.contains('R') ? "r--r--r--" : "rw-r--r--");
				attr = (list[i].IsFolder() ? "d" : "-") + attr;
			}

			printf("{}%s %d %s %lx %s %s %s\n", 
				(LPCSTR)zzz, 
				(int)list[i].IsFolder(), 
				list[i].IsFolder() ? "0" : (LPCSTR)list[i].m_FileSize, 
				list[i].m_FileDate, 
				(LPCSTR)attr, 
				(LPCSTR)list[i].m_Owner,
				(LPCSTR)list[i].m_Group);
		}
		
		return keSuccess;
	}

	if (m_Type != keFileJobMove &&
		m_Type != keFileJobCopy &&
		m_Type != keFileJobDelete)
		return keWrongParameters;

	QString LocalDestination;

	if (IsUNCPath(m_Destination))
	{
    StartIdle();

    if (NetmapWithMessageLoop(LocalDestination, m_Destination))
    {
      StopIdle();
			return keErrorAccessDenied;
    }
    StopIdle();
  }
	else
		LocalDestination = m_Destination;
	
	CFileJobElement *pElement;

	gIdleTime = 0;
	m_nCount = count();
	CSMBErrorCode retcode = keSuccess;

	QString LastName;
	QString DestinationNow = LocalDestination;

  for (pElement=first(); pElement != NULL; pElement=next())
	{
		BOOL bThisIsLastItem = (NULL == next());
		prev(); // undo next() side effect...

		if (pElement->m_nBaseNameLength != -1)
			m_dwNowAtFile++;
    
    if (pElement->m_FileName == LastName)
      continue;

    if (m_Type == keFileJobDelete)
		{
			if (pElement->m_nBaseNameLength == -1)
				continue;

			if (!pElement->Delete(m_bNeedDeleteWarning))
				break;
		}
		else
		{
			if (m_Type == keFileJobMove && m_Ignore.count() > 0)
			{
				if (m_Ignore.contains(pElement->m_FileName))
					continue;
			}
			
			if (pElement->m_nBaseNameLength == -1)
			{
				if (IsTrashFolder(LocalDestination))
				{
					BOOL bIsDir = S_ISDIR(pElement->m_FileMode);
		
					if (m_bNeedDeleteWarning)
					{
						QSTRING_WITH_SIZE(msg, 50 + 256);
						
						msg.sprintf(LoadString(bIsDir ? knCONFIRM_FOLDER_TO_TRASH_QUESTION : knCONFIRM_FILE_TO_TRASH_QUESTION), 
                        (LPCSTR)SqueezeString(pElement->m_FileName, 50));
		
						QMessageBox mb(LoadString(bIsDir ? knCONFIRM_FOLDER_DELETE : knCONFIRM_FILE_DELETE), 
													 (LPCSTR)msg, 
													 QMessageBox::Warning, 
													 QMessageBox::Yes | QMessageBox::Default, 
													 QMessageBox::No, 
													 0,
													 qApp->mainWidget());
						
            mb.setIconPixmap(*LoadPixmap(keConfirmDeletePermanentIcon));
			
						gTreeExpansionNotifier.DoFileJobWarning(TRUE);
						int ret = mb.exec();
						gTreeExpansionNotifier.DoFileJobWarning(FALSE);
			
						if (ret != QMessageBox::Yes)
							return keStoppedByUser;
          
						if (bIsDir)
							m_bNeedDeleteWarning = FALSE;
					}
			
					DestinationNow = LocalDestination;
					CreateTrashEntry(DestinationNow, pElement->m_FileName);
					gbAutoOverwriteMode = TRUE;
				}
				continue;
			}
			
			CSMBErrorCode OpResult = pElement->CopyMove(DestinationNow, m_Destination, m_Type == keFileJobMove, m_bDestinationFinalized, m_dwTotalSize, m_dwNowAtByte, bThisIsLastItem);
      
			if (keStoppedByUser == OpResult ||
					keUnableToCreate == OpResult)
				break;
      
			if (m_Type == keFileJobMove && OpResult == keSuccess)
        LastName = pElement->m_FileName;
      else
        LastName = "";

      if (keDirectoryNotEmpty == OpResult)
			{
				retcode = keDirectoryNotEmpty;
				m_Outstanding.append(pElement->m_FileName);
			}
		}

		qApp->processEvents();
	}

	if (LocalDestination != m_Destination)
		gTreeExpansionNotifier.ScheduleUnmap(m_Destination); // UNC destination must be unmapped...
	
	return retcode;
}

///////////////////////////////////////////////////////////////////////////////

void FtpCallback(void *UserData, unsigned long Progress)
{
	double *pdwNowAtByte = (double *)UserData;

	*pdwNowAtByte += Progress;
}

///////////////////////////////////////////////////////////////////////////////

BOOL CFileJobElement::LocalDelete()
{
	BOOL bIsDir = S_ISDIR(m_FileMode);
TryAgain:;

  struct stat st;

  if (-1 == lstat(m_FileName, &st))
		return TRUE; // return if doesn't exist anymore. Couldn't use access() here because access follows symlinks.
	
	if (!bIsDir && st.st_size > 5*1024*1024)
	{
		pid_t pid;

		// We need to fork because our UI must remain alive

		if ((pid = fork()) < 0)
			goto NoFork;
		else
		{
			if (pid == 0)
			{
				/* child */
				if (!unlink(m_FileName))
					exit(0);
				else
					exit(errno);
			}
		}

		// parent

		int status;

		while (!waitpid(pid, &status, WNOHANG))
		{
			qApp->processEvents(500);
		}

		if (!status)
			return TRUE;
		else
			errno = status;
	}
	else
	{
NoFork:;		
		if (bIsDir ? !rmdir(m_FileName) : !unlink(m_FileName))
		{
			if (bIsDir)
				DeleteLocalShares(m_FileName);
		
			return TRUE;
		}
	}
		
	if (bIsDir && (errno == ENOTEMPTY || errno == EACCES))
		return TRUE;
	
	if (!ReportCommonFileError(m_FileName, errno,  CFileJob::m_nCount > 1, knSTR_DELETE))
		goto TryAgain;

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL CFileJobElement::FTPDelete()
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, m_FileName);

	if (NULL == pSession)
		return FALSE; // unable to establish FTP session

	CSMBErrorCode retcode;

TryAgain:;
	retcode = pSession->DeleteFile(m_FileName);

	if (keSuccess == retcode ||
			keFileNotFound == retcode)
		return TRUE; // deleted OK... or already gone....

	if (keDirectoryNotEmpty == retcode)
		return TRUE; // expected and OK, because we will then delete it later...

	QString Hostname, Path;
	int nCred;

	ParseURL(m_FileName, Hostname, Path, nCred);

	StartIdle();
	int n = ReportCommonFileError(m_FileName, retcode == keErrorAccessDenied ? EACCES : knENOENT,  CFileJob::m_nCount > 1, knSTR_DELETE, knUNABLE_TO_DELETE);
	StopIdle();

	if (!n)
		goto TryAgain;
	
	return n == 2; // Ignore
}

///////////////////////////////////////////////////////////////////////////////

BOOL CFileJobElement::Delete(BOOL& bNeedWarning)
{
	BOOL bIsDir = S_ISDIR(m_FileMode);

	if (bNeedWarning)
	{
		QSTRING_WITH_SIZE(msg, 50 + 256);
		int nCaptionID;

		if (IsPrinterUrl(m_FileName))
		{
			nCaptionID = knCONFIRM_PRINTER_DELETE;
			msg.sprintf(LoadString(knCONFIRM_PRINTER_DELETE_QUESTION), (LPCSTR)m_FileName + 10);
		}
    else
		{
			nCaptionID = bIsDir ? knCONFIRM_FOLDER_DELETE : knCONFIRM_FILE_DELETE;
			msg.sprintf(LoadString(bIsDir ? knCONFIRM_FOLDER_DELETE_QUESTION : knCONFIRM_FILE_DELETE_QUESTION), (LPCSTR)SqueezeString(m_FileName, 50));
		}

		QMessageBox mb(LoadString(nCaptionID), 
									 (LPCSTR)msg, 
									 QMessageBox::Warning, 
									 QMessageBox::Yes | QMessageBox::Default, 
									 QMessageBox::No, 
									 0,
									 qApp->mainWidget());
		
    mb.setIconPixmap(*LoadPixmap(keConfirmDeletePermanentIcon));
    
    gTreeExpansionNotifier.DoFileJobWarning(TRUE);
    int ret = mb.exec();
    gTreeExpansionNotifier.DoFileJobWarning(FALSE);

		if (ret != QMessageBox::Yes)
			exit(-1);

		bNeedWarning = FALSE;
	}
	
	if (IsFTPUrl(m_FileName))
		return FTPDelete();
	else
		if (IsPrinterUrl(m_FileName) && NULL != gpDeletePrinterHandler)
		{
			return (*gpDeletePrinterHandler)((LPCSTR)m_FileName+10);
		}
		else
			return LocalDelete();

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void CFileJobElement::SetParentModTime()
{
	QString ParentFolder = GetParentURL((LPCSTR)m_FileName);
	
	if (!ParentFolder.isEmpty())
	{
		if (utime(ParentFolder, NULL))
		{
			printf("Unable to change time for %s\n", (LPCSTR)ParentFolder);
		}
	}
}

static void TouchFile(LPCSTR FileName, time_t at, time_t mt)
{
	// Change file time...

	struct utimbuf FileTimes;

	FileTimes.actime = at; 
	FileTimes.modtime= mt;

	utime(FileName, &FileTimes);
}

///////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFileJobElement::CopyMove(LPCSTR Destination,
															 LPCSTR DestinationDisplay,
															 BOOL bMove, 
															 BOOL bDestinationFinalized, 
															 double& dwTotalSize, 
															 double& dwNowAtByte,
															 BOOL bThisIsLastFile)
{
  int nCaptionID;
	LPCSTR Caption;
  QSTRING_WITH_SIZE(msg, strlen(Destination)+m_FileName.length()+1024);
	struct stat st;
	int ifd;
	int ofd;
	static BOOL bYesToAll = FALSE;
	static BOOL bNoToAll = FALSE;
	static BOOL bLastWasYes = TRUE;
	static BOOL bYesToAllFolders = FALSE;
	static BOOL bNoToAllFolders = FALSE;
	static BOOL bLastWasYesForFolders = TRUE;
	QString DestinationFile;
	QString DestinationDisplayFolder;
	QString DestinationFolder;
	LPCSTR p;

	if (S_ISFIFO(m_FileMode))
    return keSuccess;
  
  if (IsPrinterUrl(Destination)) // Special case for printing
	{
		if (NULL == gpPrintFileHandler)
			return keUnableToCreate; // calling application did not provide printing function

		nCaptionID = knSTR_FILE_PRINT;
		Caption = LoadString(nCaptionID);
		
		goto TrySourceStat;
	}
	
	nCaptionID = (bMove ? knSTR_FILE_MOVE : knSTR_FILE_COPY);
	Caption = LoadString(nCaptionID);
	
	if (gbAutoOverwriteMode)
	{
		bYesToAll = TRUE;
	}

	// Compose destination file name...

	if (strlen(Destination) > 7 && !strnicmp(Destination, "file://", 7))
		Destination += 7;

	DestinationFile = Destination;
  
	CFileJob::m_NowAtFile = DestinationDisplay;
	p = NULL;
	
	if (!bDestinationFinalized)
	{
Retry:;
		switch (FolderExists(Destination))
		{
			case -3:	// file exists, just continue
			break;

			case -2:	// permission denied
			case 1:		// exists and not writable
				msg.sprintf(LoadString(knSTR_NO_PERMISSIONS), (LPCSTR)SqueezeString(Destination, 50));
				
				if (!QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL)))
					goto Retry;

			return keErrorAccessDenied;

			case -1:	// doesn't exist
			case 0:		// exists and writable
        if (DestinationFile[DestinationFile.length()-1] != '/')
					DestinationFile += "/";

				if (CFileJob::m_NowAtFile[CFileJob::m_NowAtFile.length()-1] != '/')
					CFileJob::m_NowAtFile += "/";
				
				if ((unsigned long)m_nBaseNameLength < strlen(m_FileName))
				{
					p = (LPCSTR)m_FileName + m_nBaseNameLength;

					if (*p == '/')
						p++;

					DestinationFile += p;

          if (!bMove && DestinationFile == m_FileName)
          {
            DestinationFile = DestinationFile.left(DestinationFile.length() - strlen(p));
            
            QString Name;
            int nCount=0;
            
            do
            {
              if (0 == nCount++)
                Name = DestinationFile + LoadString(knCOPY_OF_SPACE) + p;
              else
                Name.sprintf(LoadString(knXCOPY_N_OF_Y), (LPCSTR)DestinationFile, nCount, p);
            } while (!FileStat(Name, &st));

            DestinationFile = Name;
          }
				}
				
				if (DestinationFile.right(1) == "/" || DestinationFile.right(1) == "\\") // remove trailing slash
				{
					DestinationFile = DestinationFile.left(DestinationFile.length()-1);
				}
				
				CFileJob::m_NowAtFile += p;

				if (IsUNCPath(CFileJob::m_NowAtFile))
					CFileJob::m_NowAtFile = MakeSlashesBackward(CFileJob::m_NowAtFile);
			break;

			default:
			return keErrorAccessDenied; // oops, unexpected return code
		}
	}

  // Handle Trash
  
	// Check destination folder
	p = (LPCSTR)DestinationFile + strlen(DestinationFile) - 1;

	while (p > (LPCSTR)DestinationFile && *p != '/')
		p--;

	if (*p == '/' && p == (LPCSTR)DestinationFile)
		p++;

	DestinationFolder = QString((LPCSTR)DestinationFile).left(p - (LPCSTR)DestinationFile);
//#else  
//  DestinationFolder ((LPCSTR)DestinationFile, p - (LPCSTR)DestinationFile + 1);
//#endif
  
  DestinationDisplayFolder = QString((LPCSTR)CFileJob::m_NowAtFile).left(CFileJob::m_NowAtFile.length() - strlen(p));

	StartIdle();

	if (!EnsureDirectoryExists(DestinationFolder, FALSE))  // and do not prompt for creation...
		return keStoppedByUser;

	StopIdle();

TryStat:;

	// Warn if destination file exists
	
	if (!FileStat(DestinationFile, &st))
	{
		int ret;
		
		if (*p == '/') // Prepare file name without path
			p++;

		if (S_ISDIR(st.st_mode))
		{
			if (S_ISDIR(m_FileMode) ||
					(S_ISLNK(m_FileMode) && !stat(m_FileName, &st) && S_ISDIR(st.st_mode)))
			{
				if (bNoToAllFolders)
					return keStoppedByUser;

				if (!bYesToAllFolders && m_nChainStatus != 2)
				{
					QString Button2 = LoadString(bLastWasYesForFolders ? knSTR_YES_ALL : knNO);
					QString Button3 = LoadString(bLastWasYesForFolders ? knNO : knSTR_NO_ALL);
				
					msg.sprintf(LoadString(knSTR_DESTINATION_FOLDER_EXISTS), 
								(LPCSTR)SqueezeString(DestinationDisplayFolder, 50),
								(LPCSTR)SqueezeString(p, 50));
					
					QMessageBox mb(LoadString(knCONFIRM_FOLDER_REPLACE), (LPCSTR)msg, QMessageBox::Warning, QMessageBox::Yes | QMessageBox::Default, QMessageBox::Ok, QMessageBox::No);
					
					mb.setButtonText(QMessageBox::Ok, Button2);
					mb.setButtonText(QMessageBox::No, Button3);
					
					mb.setIconPixmap(*LoadPixmap(keReplaceFolderIcon));

					StartIdle();
					ret = mb.exec();
					StopIdle();
					
					switch (ret)
					{
						case QMessageBox::Yes:
							bLastWasYesForFolders = TRUE;
							bYesToAll = TRUE;
						break;

						case QMessageBox::Ok:
							if (bLastWasYesForFolders)  // Yes to all
							{
								bYesToAllFolders = TRUE;
								bYesToAll = TRUE;
								bNoToAll = FALSE;
							}
							else	// No
							{
								bLastWasYesForFolders = FALSE;
								bYesToAll = FALSE;
								bNoToAll = TRUE;
								return keStoppedByUser;
							}
						break;

						case QMessageBox::No:
							
							bYesToAll = FALSE;
							bNoToAll = TRUE;
							
							if (bLastWasYesForFolders) // No
							{
								bLastWasYesForFolders = FALSE;
							}
							else	// No to all
							{
								bNoToAllFolders = TRUE;
							}
						return keStoppedByUser;
					}
				}
			}
			else
			{
			}
		}
		else
		{
			if (bNoToAll)
				return keStoppedByUser;

			if (!bYesToAll)
			{
				QString Button2 = LoadString(bLastWasYes ? knSTR_YES_ALL : knNO);
				QString Button3 = LoadString(bLastWasYes ? knNO : knSTR_NO_ALL);
				
				msg.sprintf(LoadString(knSTR_DESTINATION_FILE_EXISTS), 
							(LPCSTR)SqueezeString(DestinationDisplayFolder, 50),
							(LPCSTR)SqueezeString(p, 50));
				
				QString Modified1, Modified2;
				
				Modified1.sprintf(LoadString(knMODIFIED_ON_XXX), ctime(&st.st_mtime));
				Modified1 = Modified1.left(Modified1.length()-1);
	
				Modified2.sprintf(LoadString(knMODIFIED_ON_XXX), ctime(&m_FileTime));
				Modified2 = Modified2.left(Modified2.length()-1);
	
				CFileReplaceDialog dlg(qApp->mainWidget(), msg, (LPCSTR)Button2, (LPCSTR)Button3, 
															 SizeBytesFormat(st.st_size),
															 Modified1, 
															 SizeBytesFormat(m_FileSize),
															 Modified2, 
															 GetFilePixmap(DestinationFile, FALSE, FALSE, TRUE),
															 GetFilePixmap(DestinationFile, FALSE, FALSE, TRUE));
				
				StartIdle();
				ret = dlg.exec();
				StopIdle();

				switch (ret)
				{
					case 0: // Cancel
						exit(-1);//return FALSE;

					case 1:
						bLastWasYes = TRUE;
					break;

					case 2:
						if (bLastWasYes)  // Yes to all
							bYesToAll = TRUE;
						else	// No
						{
							bLastWasYes = FALSE;
							return keStoppedByUser;
						}
					break;

					case 3:
						if (bLastWasYes) // No
							bLastWasYes = FALSE;
						else	// No to all
							bNoToAll = TRUE;
					return keStoppedByUser;
				}
			}
		
      chmod(DestinationFile, 0777); // Do this because unlink() fails on read-only files :-(

TryAgain1:;		
		
			if (FileUnlink(DestinationFile))
			{
				StartIdle();
				int n = ReportCommonFileError(DestinationFile, errno,  CFileJob::m_nCount > 1, knSTR_DELETE);
				StopIdle();
				
				if (!n)
					goto TryAgain1;
	
				if (2 != n)
					return keStoppedByUser;	 // Abort
			}
		}
	}
  else
  {
    if (errno == ETXTBSY)
    {
      StartIdle();
      int n = ReportCommonFileError(DestinationFile, errno,  FALSE, nCaptionID);
      StopIdle();

      if (!n)
        goto TryStat;

      return keErrorAccessDenied;	 // Abort
    }
  }
	
  // Handle ftp source case

	if (IsFTPUrl(m_FileName))
	{
		// Remove source file 

		if (S_ISDIR(m_FileMode))
		{
			if (!EnsureDirectoryExists(DestinationFile, FALSE))
				return keStoppedByUser;

			TouchFile(DestinationFile, m_FileTime, m_FileTime);

			if (bMove && !FTPDelete())
				return keNetworkError;
		}
		else // regular file
		{
			if (S_ISREG(m_FileMode))
			{
				CFtpSession *pSession = gFtpSessions.GetSession(NULL, m_FileName);
		
				QString Hostname;
				QString SiteRelativePath;
				int nCred;
		
				ParseURL(m_FileName, Hostname, SiteRelativePath, nCred);
		
				pSession->GetFile(SiteRelativePath, DestinationFolder, FtpCallback, &dwNowAtByte);

				TouchFile(DestinationFile, m_FileTime, m_FileTime);

				if (bMove && !FTPDelete())
					return keNetworkError;
			}
		}

		return keSuccess;
	}

	// Handle FTP destination case

	if (IsFTPUrl(Destination))
	{
		if (S_ISDIR(m_FileMode))
		{
			if (!EnsureDirectoryExists(DestinationFile, FALSE))
				return keStoppedByUser;
    }
		else
		{
			CFtpSession *pSession = gFtpSessions.GetSession(NULL, DestinationFile);
			
			QString Hostname;
			QString SiteRelativePath;
			int nCred;
	
			ParseURL(DestinationFile, Hostname, SiteRelativePath, nCred);
	
			CSMBErrorCode code = pSession->PutFile(m_FileName, SiteRelativePath, FtpCallback, &dwNowAtByte);

			if (keSuccess != code)
				return code;
		}

		goto CopyDone;
	}

	// Simply rename if that is possible
	
	if (bMove)
	{
		if (!rename(m_FileName, DestinationFile))
		{
			SetParentModTime();
			
			DeleteLocalShares(m_FileName);
			return keSuccess;
		}
	}
	
	// Obtain source file stats
TrySourceStat:;
	if (lstat(m_FileName, &st))
  {
    if (errno == ETXTBSY)
    {
      StartIdle();
      int n = ReportCommonFileError(DestinationFile, errno,  FALSE, nCaptionID);
      StopIdle();

      if (!n)
        goto TrySourceStat;

      return keErrorAccessDenied;	 // Abort
    }
    
		msg.sprintf(LoadString(knSTR_UNABLE_TO_OPEN_FOR_READING), 
					(LPCSTR)SqueezeString(m_FileName, 50));

		StartIdle();

		if (!QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL)))
			goto TrySourceStat;

		StopIdle();

		return keFileReadError;
  }

	// Handle printing
	
	if (IsPrinterUrl(Destination))
	{
		if (S_ISDIR(st.st_mode)) // Skip directories in case we're printing
			return keSuccess; 

		return (*gpPrintFileHandler)(Destination, m_FileName);
	}
	
	// Deal with symbolic links

	if (S_ISLNK(st.st_mode))
	{
		char buf[MAXPATHLEN];
		
		int nLength = readlink(m_FileName, buf, sizeof(buf));
		
		if (nLength > 0)
		{
			buf[nLength] = '\0';
		
			static BOOL bFollowSymlinks = FALSE;
			static BOOL bIgnoreSymlinkErrors = FALSE;

      if (symlink(buf, DestinationFile))
			{
				if (bIgnoreSymlinkErrors)
					return keSuccess; 

				if (EEXIST == errno && 
						!stat(DestinationFile, &st) && 
						S_ISDIR(st.st_mode))
				{
					return keSuccess;
				}
				
				if (EPERM == errno)
				{
					msg.sprintf(LoadString(knUNABLE_TO_CREATE_SYMBOLIC_LINK_X), 
                      (LPCSTR)SqueezeString(DestinationFile, 50));

					off_t oldsize = st.st_size;

					if (stat(m_FileName, &st)) // Source link is broken
					{
						if (bThisIsLastFile) // no point to show "Ignore" buttons if this is the last file.
						{
							StartIdle();
							
							QMessageBox::critical(qApp->mainWidget(),
																		Caption,
																		(LPCSTR)msg,
																		LoadString(knOK));
							StopIdle();

							return keUnableToCreate;
						}
						
						StartIdle();
						
						int nRet = QMessageBox::critical(qApp->mainWidget(),
																						 Caption,
																						 (LPCSTR)msg,
																						 LoadString(knIGNORE),
																						 LoadString(knIGNORE_ALL),
																						 LoadString(knABORT),
																						 0,
																						 1);
						StopIdle();

						if (nRet == 0)
							return keSuccess;

						if (nRet == 1)
						{
							bIgnoreSymlinkErrors = TRUE;
							return keSuccess;
						}
						
						return keUnableToCreate;
					}
					else
					{
						msg += "\n\n";
						msg += LoadString(knCOPY_INSTEAD_QUESTION);
						
						int nRet;
						
						if (bFollowSymlinks)
							nRet = 1;
						else
						{
							if (bThisIsLastFile)
							{ // no point to show "Ignore" buttons if this is the last file.
								StartIdle();
								
								nRet = QMessageBox::warning(qApp->mainWidget(),
																						Caption,
																						(LPCSTR)msg,
																						LoadString(knYES),
																						LoadString(knSTR_YES_ALL),
																						LoadString(knABORT),
																						0,
																						2) + 1;
								
								StopIdle();
								
								if (nRet == 0 || nRet == 1)
									nRet++;
								else
									if (nRet == 2)
										nRet=4; 
							}
							else
							{
								CMessageBoxFour dlg(msg,
																		knYES,
																		knSTR_YES_ALL,
																		knIGNORE,
																		knABORT);
			
								StartIdle();
								nRet = dlg.exec();
								StopIdle();
							}
						}

						switch (nRet)
						{
							case 2: // Yes All
								bFollowSymlinks = TRUE;
							
							case 1: // Yes
								// Size of total job changes then...
								dwTotalSize += (st.st_size - oldsize);
								goto ProceedFurther;
	
							case 3: // Ignore
								return keSuccess;
	
							case 0:	// Cancel
							case 4: // Abort
							default: // default never happens, here just to comply with coding standard
								return keUnableToCreate;
						}
					}
				}
				else
				{
					StartIdle();
					ReportCommonFileError(DestinationFile, errno, FALSE, nCaptionID);
					StopIdle();
					
					return keUnableToCreate;
				}
			}
		}

    if (!S_ISLNK(st.st_mode))
      chmod(DestinationFile, st.st_mode);

		LCHOWN(DestinationFile, st.st_uid, st.st_gid);
	}
	else
	{
ProceedFurther:;
		if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) // device
		{
TryCreateDevice:;			
      if (mknod(DestinationFile, st.st_mode, st.st_rdev))
			{
				StartIdle();
				int n = ReportCommonFileError(DestinationFile, 
																			errno, 
																			!bThisIsLastFile, 
																			nCaptionID);
				StopIdle();									  

				if (!n)
					goto TryCreateDevice;
				
				if (2 == n) // Ignore
					return keSuccess;
				
				return keUnableToCreate;
			}
			
			LCHOWN(DestinationFile, st.st_uid, st.st_gid);
		}
		else
		{
			if (S_ISDIR(st.st_mode))
			{
				if (!EnsureDirectoryExists(DestinationFile, FALSE))
					return keStoppedByUser;
			}
			else // regular file
			{
				// Open source
TryOpenSource:;	
				ifd = open(m_FileName, O_RDONLY);
	
				if (ifd < 0)
				{
          StartIdle();

          int nCode = ReportCommonFileError(m_FileName, errno, TRUE, nCaptionID, knUNABLE_TO_READ_FILE, knSKIP);

          StopIdle();

          if (!nCode)
            goto TryOpenSource; // Retry
	
          if (nCode == 2)
            return keFileReadError; // Skip (ignore)

          return keStoppedByUser;	 // Abort
				}
				
        bool bReadingAsManyAsWeCan = false;

        if (st.st_size == 0)
        {
          char zbuf;

          if (1 == read(ifd, &zbuf, 1))
          {
            bReadingAsManyAsWeCan = true;
            
            st.st_size = 0x7fffffff;
            lseek(ifd, 0L, 0);
          }
        }

				// Open destination
	OpenDestination:;
				
				ofd = open(
						DestinationFile, 
						O_WRONLY | O_CREAT | O_TRUNC, 
						S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				
				if (ofd < 0)
				{
					msg.sprintf(LoadString(knSTR_UNABLE_TO_OPEN_FOR_WRITING), 
                      (LPCSTR)SqueezeString(CFileJob::m_NowAtFile, 50));
					
					StartIdle();
					int nRet = QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL));
					StopIdle();

					if (!nRet)
						goto OpenDestination;
					
					close(ifd);
          
					return keUnableToCreate;
				}
	
				CFileJob::m_UnfinishedFile = DestinationFile;

				// Allocate copy buffer
				
				off_t sz = st.st_size;
				size_t PacketSize = sz > 65536 ? 65536 : sz;
	
				char *PacketBuf = new char[PacketSize];
				
				size_t now = PacketSize;
				
				CSMBErrorCode HadErrors = keSuccess;

				// Copy data
	
				while (sz > 0)
				{
					if (sz < (off_t)now)
						now = sz;
							
					size_t bytesread = read(ifd, PacketBuf, now);
	
					if (bytesread > 0 &&
						bytesread != (size_t)write(ofd, PacketBuf, bytesread))
					{
						QString s;
						s.sprintf(LoadString(knSTR_ERROR_WRITING_FILE), 
								(LPCSTR)SqueezeString(CFileJob::m_NowAtFile, 50));
	
						StartIdle();
						QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)s);
						StopIdle();
						
            HadErrors = keUnableToCreate;
						break;
					}
          
          if (bReadingAsManyAsWeCan && (now != bytesread))
	          break;

					if (now != bytesread)
					{
            int nCode;
            static bool bIgnoreAllReadErrors = false;

            if (bIgnoreAllReadErrors)
            {
              nCode = 0;
            }
            else
            {
              msg.sprintf(LoadString(knSTR_ERROR_READING_FILE), 
                (LPCSTR)SqueezeString(m_FileName, 50));
              
              StartIdle();
              nCode = QMessageBox::critical(qApp->mainWidget(), 
                                  Caption, 
                                  (LPCSTR)msg,
                                  LoadString(knIGNORE),
                                  LoadString(knIGNORE_ALL),
                                  LoadString(knABORT));
						
              StopIdle();
            }
						
            switch (nCode)
            {
              case 0: // Ignore
                HadErrors = keFileReadError;
              break;

              case 1: // Ignore All
                HadErrors = keFileReadError;
                bIgnoreAllReadErrors = true;
              break;

              case 2:
              default:
                HadErrors = keStoppedByUser;
              break;
            }
            
						break;
					}
	
					sz -= now;
					
					dwNowAtByte += now;
	
					qApp->processEvents();
				}
						
				delete []PacketBuf;
	
				close(ifd);
				
        fchmod(ofd, st.st_mode);
				fchown(ofd, st.st_uid, st.st_gid);
				
				close(ofd);

				CFileJob::m_UnfinishedFile = "";

				if (keSuccess != HadErrors)
				{
					if (keUnableToCreate == HadErrors)
            unlink(DestinationFile);
					
          return HadErrors;
				}
			}
		}
	}

	
	// Change destination file times
	// Unfortunately, for symbolic links this won't work :-(
	TouchFile(DestinationFile, st.st_atime, st.st_mtime);

	// Remove source file if necessary

CopyDone:;

	if (bMove)
	{
TryAgain:;
		if (S_ISDIR(m_FileMode) ? !rmdir(m_FileName) : !unlink(m_FileName))
		{
			if (S_ISDIR(m_FileMode))
				DeleteLocalShares(m_FileName);

			SetParentModTime();
		}
		else
		{
			if (ENOTEMPTY == errno)
			{
				return keDirectoryNotEmpty;
			}

			StartIdle();
			int n = ReportCommonFileError(m_FileName, errno, CFileJob::m_nCount>1, nCaptionID);
			StopIdle();									  

			if (!n)
				goto TryAgain;
			
			if (n != 2)
				return keStoppedByUser; // 2 means 'ignore', so we'll return TRUE then...
		}
	}

	return keSuccess;
}

///////////////////////////////////////////////////////////////////////////////

QString RandomName()
{
  char ValidLetters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrtsuvwxyz_";
  
  static BOOL bSeedDone = FALSE;

  if (!bSeedDone)
  {
    srand(time(NULL));
    bSeedDone = TRUE;
  }
  
  char buf[10];
  int i;
  
  buf[0] = '.';

  for (i=1; i < (int)sizeof(buf)-1; i++)
  {
    buf[i] = ValidLetters[rand() % ((sizeof(ValidLetters)/sizeof(char))-1)];
  }
  
  buf[i] = '\0';
  return QString(buf);
}

///////////////////////////////////////////////////////////////////////////////

static BOOL CreateTrashEntry(QString& DestinationFile, LPCSTR Source)
{
  QString s;
	
	if (DestinationFile.right(1) != "/")
			DestinationFile += "/";
  
	while (!access(DestinationFile + (s = RandomName()), 0));

  DestinationFile += s;

  FILE *f = fopen(DestinationFile + ".info", "w");

	DestinationFile += "/";

  if (NULL == f)
    return FALSE;

  struct stat st;

  stat(Source, &st);
  
  fprintf(f, "%s\n%lx\n%lx\n%lx\n%lx\n%lx", 
          Source, 
          (unsigned long)time(NULL), 
          (unsigned long)st.st_size,
          (unsigned long)st.st_mode, 
          (unsigned long)st.st_mtime, 
          (unsigned long)st.st_ctime);
  
  fclose(f);
  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

