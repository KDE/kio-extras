/* Name: fileUNC.cpp

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <utime.h>
#include <dirent.h>
#include "qstring.h"
#include "qlist.h"
#include "mapper.h"
#include "common.h"
#include "fileUNC.h"
#include "smbutil.h"
#include "syscall.h"
#include <stdarg.h>

#ifdef stat
#undef stat
#endif
#ifdef lstat
#undef lstat
#endif
#ifdef remove
#undef remove
#endif
#ifdef opendir
#undef opendir
#endif
#ifdef closedir
#undef closedir
#endif
#ifdef fopen
#undef fopen
#endif
#ifdef freopen
#undef freopen
#endif
#ifdef fclose
#undef fclose
#endif
#ifdef QFile
#undef QFile
#endif

#ifdef QFileInfo
#undef QFileInfo
#endif

#ifdef KURL
#undef KURL
#endif

QList <CFdUncListEntry> gUList;
char szPathBuffer[1024] = "\0";
bool bWorkDirMounted = FALSE;

#ifdef QT_20
void U_QFileInfo::setFile(const QString& UNC)
#else
void U_QFileInfo::setFile(const char *UNC)
#endif
{
	//printf("U_QFileInfo::setFile(%s)\n", UNC);
	if (IsUNCPath(UNC))
	{
		QString localname;

		if (netmap(localname, UNC))
		{
			QFileInfo::setFile(UNC);
		}
		else
		{
			QFileInfo::setFile(localname);
			m_UNCPath = UNC;
		}
	}
	else
		QFileInfo::setFile(UNC);
}

U_QFileInfo::~U_QFileInfo()
{
	if (!m_UNCPath.isEmpty())
		gTreeExpansionNotifier.ScheduleUnmap(m_UNCPath);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:  U_QFile
//
//////////////////////////////////////////////////////////////////////////////

void U_QFile::setName(const char *UNC)
{
	QString localname;

	if (IsUNCPath(UNC))
	{
		if (netmap(localname, UNC))
		{
			QFile::setName(UNC);
		}
		else
		{
			QFile::setName(localname);
			m_UNCPath = UNC;
		}
	}
	else
		QFile::setName(UNC);
}

U_QFile::~U_QFile()
{
	if (!m_UNCPath.isEmpty())
		gTreeExpansionNotifier.ScheduleUnmap(m_UNCPath);
}

bool U_QFile::U_Open(int mode)
{
	return QFile::open(mode);
}

bool U_QFile::U_Open(int mode, FILE *fd)
{
	return QFile::open(mode, fd);
}

bool U_QFile::U_Open(int p1, int p2)
{
	return QFile::open(p1, p2);
}

void U_QFile::U_Close()
{
	QFile::close();
}

bool U_QFile::exists() const
{
	return QFile::exists();
}

bool U_QFile::exists(const char *fileName)
{
	int nRet = SMBFolderExists(fileName);

	return (nRet != -2 && nRet != -1);
}

bool U_QFile::remove(const char *fileName)
{
	return !unlink(fileName);
}

#if (QT_VERSION < 200)
///////////////////////////////////////////////////////////////////////////////
//
//	Class:	CCorelURL
//
///////////////////////////////////////////////////////////////////////////////

bool CCorelURL::isLocalFile()
{
	if (strcmp(protocol(), "file"))
		return false;

#if (QT_VERSION < 200)
  if (hasSubProtocol())
		return false;
#endif
	return true;
}

#endif
///////////////////////////////////////////////////////////////////////////////
//
//	Class:	CFdUncList
//
///////////////////////////////////////////////////////////////////////////////

CFdUncListEntry::CFdUncListEntry(unsigned long ulKey, const char* szUNC) : m_ulKey(ulKey), m_szUNC(szUNC)
{
}

CFdUncListEntry::~CFdUncListEntry()
{
}

unsigned long CFdUncListEntry::GetKey()
{
	return m_ulKey;
}

const char *CFdUncListEntry::GetUNC()
{
	return m_szUNC;
}


///////////////////////////////////////////////////////////////////////////////
//
//	Method:	int U_Open()
//
//	Purpose:	This is a form of the system call 'open' that provides UNC support
//
///////////////////////////////////////////////////////////////////////////////

extern "C"
{
	int open(const char *pathname, int oflag, ...)
	{
		va_list ap;
		mode_t mode;
		va_start(ap, oflag);

		mode = va_arg(ap, mode_t);
		va_end(ap);

		QString szBuffer;
		int nFd;

		//printf("UNCopen(%s)\n", pathname);

		if (IsUNCPath(pathname))
		{
			if (netmap(szBuffer, pathname))
				return -1;
			else
			{
				nFd = syscall(SYS_open, (const char*)szBuffer, oflag, mode);

				if (-1 == nFd)
					gTreeExpansionNotifier.ScheduleUnmap(pathname);
				else
				{
					CFdUncListEntry *pTemp = new CFdUncListEntry((unsigned long)nFd, pathname);
					gUList.append(pTemp);
					pTemp = 0;
				}

				return nFd;
			}
		}
		else
			return syscall(SYS_open, pathname, oflag, mode);
	}

	int creat(const char *pathname, mode_t mode)
	{
		//printf("UNCcreat(%s, %d)\n", pathname, mode);

		return open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode);
	}

	int close(int filedes)
	{
		QString szBuffer;
		
		if (!syscall(SYS_close, filedes))
		{
			if (gUList.count() > 0)
			{
				QListIterator <CFdUncListEntry> UListIterator(gUList);
				gUList.setAutoDelete(TRUE);
				UListIterator.toFirst();
	
				for (; UListIterator.current() != NULL; ++UListIterator)
				{
					if ((int)UListIterator.current()->GetKey() == filedes)
					{
						gTreeExpansionNotifier.ScheduleUnmap(UListIterator.current()->GetUNC());
						gUList.remove(UListIterator.current());
						break;
					}
				}
			}

			return 0;
		}
		else
			return -1;
	}

	int access(const char *pathname, int mode)
	{
		//printf("UNCAccess(%s, %d)\n", pathname, mode);

		if (IsUNCPath(pathname))
		{
			QString szBuffer;
			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_access, (const char*)szBuffer, mode);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_access, pathname, mode);
	}

	int truncate(const char *pathname, off_t length)
	{
		//printf("UNCtruncate(%s, %ld)\n", pathname, length);

		if (IsUNCPath(pathname))
		{
			QString szBuffer;
			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_truncate, (const char*)szBuffer, length);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_truncate, pathname, length);
	}

	int mkdir(const char *pathname, mode_t mode)
	{
		//printf("UNCMkdir(%s, 0%o)\n", pathname, mode);

		if (IsUNCPath(pathname))
		{
			QString szBuffer;
			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_mkdir, (const char*)szBuffer, mode);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_mkdir, pathname, mode);
	}

	int rmdir(const char *pathname)
	{
		//printf("UNCRmdir(%s)\n", pathname);

		if (IsUNCPath(pathname))
		{
			QString szBuffer;
			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_rmdir, (const char*)szBuffer);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_rmdir, pathname);
	}

	int unlink(const char *pathname)
	{
		//printf("UNCUnlink(%s)\n", pathname);

		if (IsUNCPath(pathname))
		{
			QString szBuffer;

			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_unlink, (const char*)szBuffer);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_unlink, pathname);
	}

	int rename(const char *oldname, const char *newname)
	{
		QString szBuffer1;
		QString szBuffer2;
		bool dUncCount[2] = {FALSE, FALSE};
		int nResult;

		//printf("UNCrename(%s, %s)!\n", oldname, newname);

		if (IsUNCPath(oldname))
		{
			nResult = netmap(szBuffer1, oldname);

			if (nResult)
				return -1;

			dUncCount[0] = TRUE;
		}
		else
			szBuffer1 = oldname;

		if (IsUNCPath(newname))
		{
			nResult = netmap(szBuffer2, newname);

			if (nResult)
				return -1;

			dUncCount[1] = TRUE;
		}
		else
			szBuffer2 = newname;

		nResult = syscall(SYS_rename, (const char*)szBuffer1, (const char*)szBuffer2);

		if (dUncCount[0])
		{
			gTreeExpansionNotifier.ScheduleUnmap(oldname);
		}

		if (dUncCount[1])
		{
			gTreeExpansionNotifier.ScheduleUnmap(newname);
		}

		return nResult;
	}

	int chdir(const char *pathname)
	{
		QString szBuffer;
		bool bUnMap;
		char szUnMapDir[1024];
		int nResult;

		bUnMap = bWorkDirMounted;
		strcpy(szUnMapDir, szPathBuffer);

		//printf("UNCChdir!\n");

		if (IsUNCPath(pathname))
		{
			nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_chdir, (const char*)szBuffer);

				if (!nResult)
				{
					strcpy(szPathBuffer, pathname);
					bWorkDirMounted = TRUE;
				}
				else
					gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			if (bUnMap && !nResult)
				gTreeExpansionNotifier.ScheduleUnmap((const char*)szUnMapDir);

			return nResult;
		}
		else
		{
			nResult = syscall(SYS_chdir, pathname);

			if ((bUnMap) && !nResult)
			{
				gTreeExpansionNotifier.ScheduleUnmap((const char*)szUnMapDir);

				bWorkDirMounted = FALSE;
			}

			return nResult;
		}
	}

	int utime(const char *pathname, const struct utimbuf *times)
	{
		//printf("UNCUtime\n");

		if (IsUNCPath(pathname))
		{
			QString szBuffer;
			int nResult = netmap(szBuffer, pathname);

			if (!nResult)
			{
				nResult = syscall(SYS_utime, (const char*)szBuffer, times);
				gTreeExpansionNotifier.ScheduleUnmap(pathname);
			}
			else
				nResult = -1;

			return nResult;
		}
		else
			return syscall(SYS_utime, pathname, times);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//	Method:	int U_Stat()
//
//	Purpose:	This is a form of the system call 'stat' that provides UNC support
//
///////////////////////////////////////////////////////////////////////////////

int U_Stat1(const char *pathname, struct stat *buf)
{
	printf("\nU_Stat1(%s)\n", pathname);

	if (IsUNCPath(pathname))
	{
		QString szBuffer;
		int nResult = netmap(szBuffer, pathname);

		if (!nResult)
	  {
	    nResult = stat((const char*)szBuffer, buf);

			gTreeExpansionNotifier.ScheduleUnmap(pathname);
	  }
	  else
	    nResult = -1;

		return nResult;
	}
	else
	  return stat(pathname, buf);
}

int U_Stat2(int /*ver*/, const char *pathname, struct stat *buf)
{
  int nResult;
  QString szBuffer;
  //printf("\nStat2\n");
  if (((pathname[0] == '/') && (pathname[1] == '/')) ||
       ((pathname[1] == '\\') && (pathname[1] == '\\')))
  {
    nResult = netmap(szBuffer, pathname);
    //printf("Netmap result is: %d\n", nResult);
    if (nResult == 0)
    {
      nResult = stat((const char*)szBuffer, buf);
      //printf("Stat result is: %d\n", nResult);
      netunmap(szBuffer, pathname);
    }
    else
    {
      nResult = -1;
    }
    return(nResult);
  }
  else
  {
    return(stat(pathname, buf));
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//	Method:	int U_Lstat()
//
//	Purpose:	This is a form of the system call 'lstat' that provides UNC support
//
///////////////////////////////////////////////////////////////////////////////

int U_Lstat1(const char *pathname, struct stat *buf)
{
	QString szBuffer;
  int nResult;

	//printf("\nLStat1\n");
	if (((pathname[0] == '/') && (pathname[1] == '/')) ||
	     ((pathname[1] == '\\') && (pathname[1] == '\\')))
	{
	  nResult = netmap(szBuffer, pathname);
		//printf("Netmap result is: %d\n", nResult);
	  if (nResult == 0)
	  {
	    nResult = lstat((const char*)szBuffer, buf);
			//printf("LStat result is: %d\n", nResult);
	    netunmap(szBuffer, pathname);
	  }
	  else
	  {
	    nResult = -1;
	  }
	  return(nResult);
	}
  else
  {
    return(lstat(pathname, buf));
  }
}

int U_Lstat2(int /*ver*/, const char *pathname, struct stat *buf)
{
  QString szBuffer;
	int nResult;

	if (IsUNCPath(pathname))
	{
	  nResult = netmap(szBuffer, pathname);
	  if (nResult == 0)
	  {
	    nResult = lstat((const char*)szBuffer, buf);
	    netunmap(szBuffer, pathname);
	  }
	  else
	  {
	    nResult = -1;
	  }
	  return nResult;
	}
	else
	{
	  return lstat(pathname, buf);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Remove()
//
//  Purpose:  This is a form of the system call 'remove' that provides UNC
//            support
//
///////////////////////////////////////////////////////////////////////////////

int U_Remove(const char *pathname)
{
	QString szBuffer;
	int nResult;

	//printf("\nRemove\n");
	if (((pathname[0] == '/') && (pathname[1] == '/')) ||
	     ((pathname[1] == '\\') && (pathname[1] == '\\')))
  {
    nResult = netmap(szBuffer, pathname);
		//printf("Netmap result is: %d\n path is: %s\n", nResult, (const char*)szBuffer);
    if (nResult == 0)
    {
      nResult = remove((const char*)szBuffer);
			//printf("Remove result is: %d\n", nResult);
      netunmap(szBuffer, pathname);
    }
    else
    {
      nResult = -1;
    }
    return(nResult);
  }
  else
  {
    return(remove(pathname));
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Opendir()
//
//  Purpose:  This is a form of the system call 'opendir' that provides UNC
//            support
//
///////////////////////////////////////////////////////////////////////////////

DIR *U_Opendir(const char *pathname)
{
  QString szBuffer;
	int nResult;
  DIR *pDp;

	//printf("\nOpendir\n");
  if (((pathname[0] == '/') && (pathname[1] == '/')) ||
       ((pathname[1] == '\\') && (pathname[1] == '\\')))
  {
    nResult = netmap(szBuffer, pathname);
		//printf("Netmap result is: %d, %s\n", nResult, (const char*)szBuffer);
    if (nResult == 0)
    {
      pDp = opendir((const char*)szBuffer);
			if (pDp == 0)
			{
				//printf("Null directory pointer\n");
				netunmap(szBuffer, pathname);
			}
			else
			{
      	CFdUncListEntry *pTemp = new CFdUncListEntry((unsigned long)pDp, pathname);
				gUList.append(pTemp);
				pTemp = 0;
			}
			return(pDp);
		}
   	else
   	{
			return(0);
   	}
	}
  else
  {
  	return(opendir(pathname));
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Closedir()
//
//  Purpose:  This is a form of the system call 'closedir' that
//  provides UNC support
//
///////////////////////////////////////////////////////////////////////////////

int U_Closedir(DIR *dp)
{
  QString szBuffer;
	QListIterator <CFdUncListEntry> UListIterator(gUList);
	gUList.setAutoDelete(TRUE);

	//printf("\nUclosedir\n");
	if (closedir(dp) == 0)
	{
	  UListIterator.toFirst();
	  for (; UListIterator.current() != NULL; ++UListIterator)
	  {
	    if (UListIterator.current()->GetKey() ==  (unsigned long)dp)
	    {
	      netunmap(szBuffer, UListIterator.current()->GetUNC());
	      gUList.remove(UListIterator.current());
	      return(0);
	    }
	  }
	  return(0);
  }
  else
  {
    return(-1);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Fopen()
//
//  Purpose:  This is a form of the system call 'fopen' that provides UNC
//            support
//
///////////////////////////////////////////////////////////////////////////////

FILE *U_Fopen(const char *pathname, const char *type)
{
  int nResult;
	QString szBuffer;
  FILE *pFp;

  //printf("\nFopen\n");
	if (((pathname[0] == '/') && (pathname[1] == '/')) ||
       ((pathname[1] == '\\') && (pathname[1] == '\\')))
  {
    nResult = netmap(szBuffer, pathname);
    if (nResult == 0)
    {
      pFp = fopen((const char*)szBuffer, type);
			if (pFp == 0)
			{
				netunmap(szBuffer, pathname);
			}
			else
			{
      	CFdUncListEntry *pTemp = new CFdUncListEntry((unsigned long)pFp, pathname);
      	gUList.append(pTemp);
				pTemp = 0;
			}
      return(pFp);
    }
    else
    {
      return(0);
    }
  }
  else
  {
    return(fopen(pathname, type));
  }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Freopen()
//
//  Purpose:  This is a form of the system call 'freopen' that provides UNC
//            support
//
///////////////////////////////////////////////////////////////////////////////

FILE *U_Freopen(const char *pathname, const char *type, FILE *fp)
{
  QString szBuffer;
	int nResult;
	FILE *pFp;

	if (IsUNCPath(pathname))
	{
    nResult = netmap(szBuffer, pathname);
    
		if (!nResult)
    {
			pFp = freopen((const char*)szBuffer, type, fp);
			
			if (pFp == 0)
			{
				netunmap(szBuffer, pathname);
			}
			else
			{
				if (gUList.count() > 0)
				{
					QListIterator <CFdUncListEntry> UListIterator(gUList);
					gUList.setAutoDelete(TRUE);
					UListIterator.toFirst();
					
					for (; UListIterator.current() != NULL; ++UListIterator)
					{
						if (UListIterator.current()->GetKey() ==  (unsigned long)fp)
						{
							netunmap(szBuffer, UListIterator.current()->GetUNC());
							gUList.remove(UListIterator.current());
							break;
						}
					}
				}
      	
				CFdUncListEntry *pTemp = new CFdUncListEntry((unsigned long)pFp, pathname);
      	gUList.append(pTemp);
				pTemp = 0;
      	return pFp;
			}
    }
    else
    {
      return 0;
    }
  }
  else
  {
    pFp = freopen(pathname, type, fp);
		
		if (gUList.count() > 0)
		{
			QListIterator <CFdUncListEntry> UListIterator(gUList);
			gUList.setAutoDelete(TRUE);
			UListIterator.toFirst();

			for (; UListIterator.current() != NULL; ++UListIterator)
			{
				if (UListIterator.current()->GetKey() ==  (unsigned long)fp)
				{
					netunmap(szBuffer, UListIterator.current()->GetUNC());
					gUList.remove(UListIterator.current());
					break;
				}
			}
		}

		return pFp;
  }

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method: int U_Fclose()
//
//  Purpose:  This is a form of the system call 'fclose' that provides UNC
//						support
//
///////////////////////////////////////////////////////////////////////////////

int U_Fclose(FILE *fp)
{
  QString szBuffer;

	if (fclose(fp) == 0)
  {
		if (gUList.count() > 0)
		{
			QListIterator <CFdUncListEntry> UListIterator(gUList);
			gUList.setAutoDelete(TRUE);
			UListIterator.toFirst();
			
			for (; UListIterator.current() != NULL; ++UListIterator)
			{
				if (UListIterator.current()->GetKey() ==  (unsigned long)fp)
				{
					netunmap(szBuffer, UListIterator.current()->GetUNC());
					gUList.remove(UListIterator.current());
					
					return 0;
				}
			}
		}

    return 0;
  }
  else
  {
    return -1;
  }
}


