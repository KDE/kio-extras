/* Name: common.cpp

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
#include <qapplication.h>
#include <qobjcoll.h>
#include "smbworkgroup.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "unistd.h"
#include <time.h> // for mktime and strftime
#include <stdlib.h> // for atoi()
#include <qmessagebox.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <dirent.h> // opendir etc.
#include "ftpsession.h"
#include "smbutil.h"
#include <errno.h>
#include "kapp.h"
#include "exports.h"
#include "inifile.h"

#define CLI_PATH "/var/tmp/"
#define COREL_SERVERNAME "/var/tmp/corel.netserver"
#define CLI_PERM S_IRWXU

////////////////////////////////////////////////////////////////////////////

BOOL gbStopping = FALSE; // Set by the UI to indicate that aborting of the current operation is requested
BOOL gbShowHiddenFiles = -1; // -1 means "Not initialized"
int gnActiveTaskCount = 0;

/////////////////////////////////////////////////////
//
// We don't want libmwn to depend on "libaps".
// We only need DeletePrinter and PrintFile functionality.
// So we let it be a pointers to functions. It is a responsibility
// of the calling application to set those pointers properly
//
// TODO: move filejob.cpp out of libmwn entirely...

LPFN_DeletePrinter gpDeletePrinterHandler = NULL;
LPFN_PrintFileHandler gpPrintFileHandler = NULL;

/////////////////////////////////////////////////////
// Global TreeExpansionNotifier is application-wide
// notification engine.

CTreeExpansionNotifier gTreeExpansionNotifier;

/////////////////////////////////////////////////////

LPCSTR gSmbMountLocation = NULL;
BOOL gbUseSmbMount_2_2_x = FALSE;
BOOL gbUseSmbMount_2_1_x = FALSE;
LPCSTR gSmbUmountLocation = NULL;
LPCSTR gTrashID = "/Desktop/Trash/";

QString gSmbMountVersion;

/////////////////////////////////////////////////////////////////////////////

CWorkgroupArray gWorkgroupList; // Global workgroup list

/////////////////////////////////////////////////////////////////////////////
//Match() - regular expression (wildcard) matching function.

BOOL Match(LPCSTR s, LPCSTR p)
{
	register int scc;
	int c, yes;

	yes = 1;

	for (;;)
	{
		scc = *s++ & 0377;

		switch (c = *p++)
		{
			case '[':
			{
				int ok, lc, good;

				lc = 077777;
				good = 1;

				if (*p == '^')
				{
					good = 0;
					++p;
				}
				ok = ! good;

				for (;;)
				{
					int cc;

					cc = (unsigned char)*p++;

					if (cc == 0)
						return (! yes);         /* Missing ] */

					if (cc == ']')
						break;

					if (cc == '-')
					{
						if (lc <= scc && scc <= (unsigned char)*p++)
							ok = good;
					}
					else
						if (scc == (lc = cc))
							ok = good;
				}

				if (! ok)
					return (! yes);

				continue;
			}

			case '*':

				if (! *p)
					return (yes);

				for (--s; *s; ++s)
				{
					if (Match(s, p))
						return (yes);
				}
			return (! yes);

			case 0:
			return (scc==0 ? yes : !yes);

			case '\\':
				c = *p++;

			default:

				if ((c & 0377) != scc)
					return (! yes);

			continue;

			case '?':
				if (scc == 0)
					return (! yes);

			continue;
		} /* switch */
	}	/* for */
}

////////////////////////////////////////////////////////////////////////////

inline BOOL IsSpaceChar(char c, LPCSTR SpaceCharList)
{
    return strchr(SpaceCharList, c) != NULL;
}

////////////////////////////////////////////////////////////////////////////

QString ExtractQuotedWord(LPCSTR& x, LPCSTR SpaceCharList /* = "\t " */)
{
	if (*x == '\0')
		return QString("");

	LPCSTR p1, p2;

	// Skip opening spaces

	for (p1 = x; IsSpaceChar(*p1, SpaceCharList); p1++);

	BOOL bIsQuoted = (*p1 == '"');

	if (bIsQuoted)
		p1++;

	for (p2 = p1;
			 *p2 != '\0' &&
			 (bIsQuoted ? *p2 != '"' : !IsSpaceChar(*p2, SpaceCharList)) &&
			 *p2 != '\n';
			 p2++);

	x = (bIsQuoted && *p2 == '"') ? p2 + 1 : p2;

#ifdef QT_20
  return QString(p1).left(p2-p1);
#else
  return QString(p1, p2-p1 + 1); // +1 because QString counts trailing '\0'
#endif
}

QString ExtractWord(LPCSTR& x, LPCSTR SpaceCharList /* = "\t " */)
{
	if (*x == '\0')
		return QString("");

	LPCSTR p1, p2;

	// Skip opening spaces

	for (p1 = x; *p1 != '\0' && IsSpaceChar(*p1, SpaceCharList); p1++);

	for (p2 = p1; *p2 != '\0' && !IsSpaceChar(*p2, SpaceCharList) && *p2 != '\n'; p2++);

	x = p2;

#ifdef QT_20
  return QString(p1).left(p2-p1);
#else
  return QString(p1, p2-p1 + 1); // +1 because QString counts trailing '\0'
#endif
}

////////////////////////////////////////////////////////////////////////////

QString ExtractWordEscaped(LPCSTR& x, LPCSTR SpaceCharList /* = "\t " */)
{
	if (*x == '\0')
		return QString("");

	LPCSTR p1, p2;

  QString res;

	// Skip opening spaces

	for (p1 = x; IsSpaceChar(*p1, SpaceCharList); p1++);

	for (p2 = p1; *p2 != '\0' && !IsSpaceChar(*p2, SpaceCharList) && *p2 != '\n'; p2++)
  {
    if (*p2 == '\\')
      p2++;

    res += *p2;
  }

	x = p2;
  return res;
}

////////////////////////////////////////////////////////////////////////////

QString ExtractTail(LPCSTR& x, LPCSTR SpaceCharList /* = "\t " */)
{
	while (IsSpaceChar(*x, SpaceCharList))
		x++;

	LPCSTR p;

	for (p = x; *p != '\0' && *p != '\n'; p++);

#ifdef QT_20
  return QString(x).left(p-x);
#else
  return QString(x, p-x+1);
#endif
}

////////////////////////////////////////////////////////////////////////////

void ExtractFromTail(LPCSTR x, int nExtract, QStringArray& list)
{
	LPCSTR s = x + strlen(x);
	LPCSTR p = s;

	while (s > x && nExtract > list.count())
	{
		for (p = s-1; *p == ' ' && p > x; p--);

		while (*p != ' ' && p > x)
			p--;

		if (*p == ' ')
			p++;

#ifdef QT_20
    QString a = QString(p).left(s-p);
#else
    QString a(p, s-p+1);
#endif

		a = a.stripWhiteSpace();
		list.Add(a);
		s = p;
	}

	if (s > x)
	{
#ifdef QT_20
    QString a = QString(x).left(s-x);
#else
    QString a(x, s-x+1);
#endif
		a = a.stripWhiteSpace();
		list.Add(a);
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL ParseUNCPath(LPCSTR UNCPath, QString& Server, QString& Share, QString& Path)
{
	if ((UNCPath[0] != '/' && UNCPath[0] != '\\') ||
		UNCPath[1] != UNCPath[0])
		return FALSE;

	LPCSTR p = UNCPath+2;

  if (NULL != p && *p != '\0')
  	Server = ExtractWord(p, "\\/");
	else
		Server = "";

	if (NULL != p && *p != '\0')
    Share = ExtractWord(p, "\\/");
	else
		Share = "";

  Path = p;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

void GetShareAndDirectory(LPCSTR UNCPath, QString& ShareName, QString& Directory)
{
	if ((UNCPath[0] != '/' && UNCPath[0] != '\\') ||
		UNCPath[1] != UNCPath[0])
		return;

	LPCSTR s = UNCPath+2;

	if (UNCPath[0] == '\\')
	    s += 2;

	while (*s != '\0' && *s != UNCPath[0])
		s++;

	if (*s != '\0')
	    s++;

	if (*s == '\\')
	    s++;

	while (*s != '\0' && *s != UNCPath[0])
	        s++;

#ifdef QT_20
  ShareName = QString(UNCPath).left(s - UNCPath);
#else
  ShareName = QString(UNCPath, s - UNCPath + 1);
#endif

  if (*s != '\0')
		s++;

	if (*s == '\\')
	    s++;

	Directory = QString(s);
}

////////////////////////////////////////////////////////////////////////////

#define keDlgUnitsX 1024
#define keDlgUnitsY 768

void ConvertDlgUnits(QWidget *w)
{
	QRect r = w->geometry();
	QWidget *d  = QApplication::desktop();

	int width=d->width(); // returns screen width
	int height=d->height(); // returns screen height

	r.setCoords((r.left() * width) / keDlgUnitsX, (r.top() * height) / keDlgUnitsY,
		(r.right() * width) / keDlgUnitsX, (r.bottom() * height) / keDlgUnitsY);

	w->setGeometry(r);

	QObjectList *list = (QObjectList*)(w->children());

	if (list != NULL)
	{
		QObject *p;

		for (p=list->first(); p != NULL; p = list->next())
		{
			if (p->isWidgetType())
				ConvertDlgUnits((QWidget*)p);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

QObject *GetComboEdit(QObject *pCombo)
{
	QObjectList *list = (QObjectList*)(pCombo->children());

	if (list != NULL)
	{
		QObject *p;

		for (p=list->first(); p != NULL; p = list->next())
		{
		  	if (!strcmp(p->name(), "combo edit"))
				return p;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////
// This function is a very important one.
// It is used to organize asynchronous operation.
// If we are waiting for external process to send us some
// data through the pipe (for instance, smbclient),
// we still want the UI to react to the user input.
// By various reasons, it is extremely hard to implement
// real multitasking and do thread synchronization.
// As Qt uses event-driven application model, we can
// do something similar to what was happening in Windows 3.11:
// enter message loop whenever we are waiting for something
// to happen.
// So here we periodically check for something to arrive from
// the pipe (FILE *f) using select(...) and doing message loop
// using processEvents(...).

BOOL WaitWithMessageLoop(FILE *f)
{
	int retval;

	gnActiveTaskCount++;

	do
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		int fd = fileno(f);
		FD_SET(fd, &rfds);
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 400;
		retval = select(fd+1, &rfds, NULL, NULL, &tv);

		if (!retval)
		   qApp->processEvents(500);
	}
	while (!retval && !feof(f) && !ferror(f) && !gbStopping);

	gnActiveTaskCount--;

	int retcode = !gbStopping;

	if (gnActiveTaskCount == 0 && gbStopping)
		gbStopping = FALSE;

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

QString NumToCommaString(unsigned long num)
{
	QString Buffer,Temp;
	int i,Length;

	Buffer.sprintf("%lu", num);

	Length = Buffer.length();

	for (i = 0; i < (Length-1)/3; i++)
	{
		Temp = "," + Buffer.right(3*(i+1) + i);
		Buffer = Buffer.left(Buffer.length() - 3*(i+1) - i) + Temp;
	}

	return Buffer;
}

////////////////////////////////////////////////////////////////////////////

QString SizeInKilobytes(unsigned long num)
{
	return NumToCommaString(((num+1023)/1024)) + "KB";
}

////////////////////////////////////////////////////////////////////////////

QString SizeBytesFormat(double num)
{
	QString c;
	QString Buffer;

	if (num < 1024.)
		c = " bytes";
	else
		if (num < 1024.*1024.)
		{
			c = "KB";
			num = num/1024.;
		}
		else
			if (num < 1024.*1024.*1024.)
			{
				c = "MB";
				num = num/(1024.*1024.);
			}
			else
			{
				c = "GB";
				num = num/(1024.*1024.*1024.);
			}

	Buffer.sprintf("%-15.2f", num);
	Buffer = Buffer.stripWhiteSpace();

	if (Buffer.right(3) == ".00")
		Buffer.truncate(Buffer.length()-3);

	return Buffer+c;
}

////////////////////////////////////////////////////////////////////////////

time_t ParseDate(LPCSTR s)
{
 	static char Months[12][10];
	static BOOL bFirstTime = TRUE;
	int i;
	struct tm Tm;

	time_t tt = time(NULL);
	Tm = *localtime(&tt);

	if (bFirstTime)
	{
		for(i=0; i<12; i++)
		{
		    Tm.tm_mon  = i;
		    strftime(Months[i], 10, "%b" ,&Tm);
		}

		bFirstTime = FALSE;
	}

	if (strlen(s) == 12)
	{
		QString Month = ExtractWord(s);
		for (i=0; i < 12; i++)
		{
			if (Months[i] == Month)
			{
				Tm.tm_mon = i; // month in the range 0-11
				break;
			}
		}
		Tm.tm_mday = atoi(ExtractWord(s));

		if (s[3] == ':')
		{
			char x[3];
			x[0] = s[1];
			x[1] = s[2];
			x[2] = '\0';

			Tm.tm_hour = atoi(x);

			x[0] = s[4];
			x[1] = s[5];

			Tm.tm_min = atoi(x);

			if (mktime(&Tm) > tt)
				Tm.tm_year--;
		}
		else
		{
			Tm.tm_hour = 12;
			Tm.tm_min = 0;
			Tm.tm_year = atoi(ExtractWord(s)) - 1900;
		}

		Tm.tm_sec = 0;
	}
	else
	{
		ExtractWord(s); // Skip weekday name

		QString Month = ExtractWord(s);

		for (i=0; i < 12; i++)
		{
			if (Months[i] == Month)
			{
				Tm.tm_mon = i; // month in the range 0-11
				break;
			}
		}

		Tm.tm_mday = atoi(ExtractWord(s));
		Tm.tm_hour = atoi(ExtractWord(s, " :")); // Get hour (in 24-hour clock)
		Tm.tm_min = atoi(ExtractWord(s, " :"));
		Tm.tm_sec = atoi(ExtractWord(s, " :"));
		Tm.tm_year = atoi(ExtractWord(s)) - 1900;
		Tm.tm_isdst = -1; // information is not available
	}

	return mktime(&Tm);
}

////////////////////////////////////////////////////////////////////////////
/* Check whether user is allowed to mount on the specified mount point */

BOOL CanMountAt(LPCSTR Path)
{
	struct stat st;

    return
		stat(Path, &st) == 0 &&
		S_ISDIR(st.st_mode) &&
		(getuid() == 0 || getgid() == 0 || getuid() == st.st_uid) &&
		((st.st_mode & S_IRWXU) == S_IRWXU);
}

////////////////////////////////////////////////////////////////////////////

QString MakeSlashesBackwardDouble(LPCSTR s)
{
	QString ret;

	while (*s != '\0')
	{
	   if (*s == '/' || *s == '\\')
		   ret += "\\\\";
	   else
		   ret += *s;

	   s++;
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////

QString MakeSlashesForward(LPCSTR s)
{
	int nLen = strlen(s);

	QString ret;

	ret.fill(' ', nLen);

	for (int i=0; i < nLen; i++)
		ret[i] = (s[i] == '\\') ? '/' : s[i];

	return ret;
}

////////////////////////////////////////////////////////////////////////////

QString EscapeString(LPCSTR s)
{
	int nLen = strlen(s);

	QString ret;

	ret.fill(' ', nLen*2 + 1);

	int j=0;

	for (int i=0; i < nLen; i++)
	{
		char c = s[i];

		if (c == '\\' ||
			c == '"' ||
			c == '\'' ||
			c == '`' ||
			c == '~' ||
			c == '!' ||
			c == '$')
			ret[j++] = '\\';
		ret[j++] = c;
	}

	ret[j] = '\0';
	ret.truncate(j);

	return ret;
}

////////////////////////////////////////////////////////////////////////////

QString MakeSlashesBackward(LPCSTR s)
{
	int nLen = strlen(s);

	QString ret;

	ret.fill(' ', nLen);

	for (int i=0; i < nLen; i++)
		ret[i] = (s[i] == '/') ? '\\' : s[i];

	return ret;
}

////////////////////////////////////////////////////////////////////////////

BOOL IsValidFolder(LPCSTR name)
{
	struct stat st;

	if (IsFTPUrl(name))
		return IsFTPValidFolder(name);

	return (!stat(name, &st) && (st.st_mode & S_IFDIR) == S_IFDIR);
}

////////////////////////////////////////////////////////////////////////////
/* FolderExists checks for the presence of folder.
   Returns:
	-3 - exists but is not a folder
	-2 - permission denied
	-1 - doesn't exist
	0 - exists and writable
	1 - exists and not writable
*/

int FolderExists(LPCSTR name)
{
	if (IsFTPUrl(name))
		return FTPFolderExists(name);

	if (IsUNCPath(name))
		return SMBFolderExists(name);

	if (access(name, F_OK))
		return -1;

	if (!IsValidFolder(name))
		return -3;

	return (access(name, W_OK) ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode MakeDir(LPCSTR name)
{
	if (IsUNCPath(name))
			return SMBMkdir(name);

	if (IsFTPUrl(name))
		return FTPMakeDir(name) ? keErrorAccessDenied : keSuccess;

	return mkdir(name, 0777) ?  keErrorAccessDenied : keSuccess;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CreateDir(LPCSTR name)
{
	if (FolderExists(name) != -1)
		return keSuccess; // return if already exists

	LPCSTR s = name;
	s += strlen(s) - 1;

	while (*s != '\\' && *s != '/' && s != name)
		s--;

	CSMBErrorCode retcode;

	if (s == name)
		retcode = MakeDir(name);
	else
	{
		int len = s - name + 1;
		LPSTR x = new char[len];
		strncpy(x, name, len-1);
		x[len-1] = '\0';

		retcode = CreateDir(x);

		if (keSuccess == retcode)
			retcode = MakeDir(name);

		delete []x;
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

BOOL EnsureDirectoryExists(LPCSTR name, BOOL bPromptToCreate /*= TRUE */, BOOL bMustBeWritable /* = TRUE */)
{
  QSTRING_WITH_SIZE(msg, 256 + strlen(name));

	LPCSTR Caption = LoadString(knSTR_DIRECTORY);

Retry:;
	switch (FolderExists(name))
	{
		case 0:
			return TRUE; // exists and writable, OK

		case 1:
      if (!bMustBeWritable)
        return TRUE;

		case -2:
		{
      msg.sprintf(LoadString(knSTR_NO_PERMISSIONS), name);

			if (!QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knNO)))
				goto Retry;

			return FALSE;
		}
		break;

		case -3: // plain file instead of folder exists, abort
		{
			msg.sprintf(LoadString(knSTR_UNABLE_TO_CREATE), name);
			QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg);

			return FALSE;
		}
		break;

		case -1:
		{
			if (bPromptToCreate)
			{
				msg.sprintf(LoadString(knSTR_CONFIRM_CREATE_DIRECTORY), name);

				if (0 != QMessageBox::warning(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knYES), LoadString(knNO)))
					return FALSE;
			}

TryCreateDir:;
			if (keSuccess != CreateDir(name))
			{
				msg.sprintf(LoadString(knSTR_UNABLE_TO_CREATE), name);

				if (!QMessageBox::critical(qApp->mainWidget(), Caption, (LPCSTR)msg, LoadString(knSTR_RETRY), LoadString(knCANCEL)))
					goto TryCreateDir;

				return FALSE;
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// The function below will add a new entry into .automount file
// in the default home directory for the currently logged in user

BOOL AddAutoMountEntry(LPCSTR Entry)
{
	QString s((LPCSTR)gDefaultHomeDirectory);

	struct stat st;

	if (stat(s, &st) < 0)
		return FALSE; // Home dir does not exist

	s += "/.automount";

	BOOL bFileExisted = (access(s,0) == 0);

	FILE *f = fopen(s, "a");

	if (NULL == f)
	{
		//printf("Unable to write %s\n", (LPCSTR)s);
		return FALSE;
	}

	// If we're now working as someone else (for example, after 'su'),
	// we will need to change the owner of the file

	if (!bFileExisted && (getuid() != st.st_uid || getgid() != st.st_gid))
		fchown(fileno(f), st.st_uid, st.st_gid);

	fprintf(f, "%s\n", Entry);
  fclose(f);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// This function is used to remove an automount entry from the .automount file
// in the currently logged in user's home directory

void RemoveAutoMountEntry(LPCSTR MountPath)
{
	void GetDefaultCredentials();

  if (gDefaultHomeDirectory.isEmpty())
    GetDefaultCredentials();

  QString FileName((LPCSTR)gDefaultHomeDirectory);

	FileName += "/.automount";

	struct stat st;

  if (stat(FileName, &st) < 0)
		return; // no file found

	QString FileBak((LPCSTR)FileName);

	FileBak += ".bak";

	if (rename(FileName, FileBak))
		return; // Unable to rename

	FILE *fi = fopen(FileBak, "r");

	if (NULL != fi)
	{
		unlink(FileBak);

		FILE *fo = fopen(FileName, "w");

		if (getuid() != st.st_uid ||
			getgid() != st.st_gid)
		{
			fchown(fileno(fo), st.st_uid, st.st_gid);
		}

		if (NULL != fo)
		{
			char buf[1024];

			while (!feof(fi))
			{
				fgets(buf, sizeof(buf)-1, fi);

				if (feof(fi))
					break;

				LPCSTR p = &buf[0];

				ExtractQuotedWord(p);

        QString MountPoint = ExtractQuotedWord(p);

        if (MountPoint != MountPath)
					fwrite(buf, 1, strlen(buf), fo);
			}

			fclose(fo);
		}

		fclose(fi);
	}
}

///////////////////////////////////////////////////////////////////////////////

BOOL FindAutoMountEntry(LPCSTR UNCPath, LPCSTR MountPath)
{
	QString ForwardUNCPath;

	if (UNCPath != NULL)
		ForwardUNCPath = MakeSlashesForward(UNCPath);

	//printf("FindAutoMountEntry([%s], [%s])\n", UNCPath, MountPath);

	QString FileName((LPCSTR)gDefaultHomeDirectory);

	FileName += "/.automount";

	BOOL retcode = FALSE;

	FILE *f = fopen((LPCSTR)FileName, "r");

	if (NULL != f)
	{
    char buf[1024];

		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			LPCSTR p = &buf[0];

			QString ThisUNC((LPCSTR)ExtractQuotedWord(p));
			QString ThisPath((LPCSTR)ExtractQuotedWord(p));

      if ((ForwardUNCPath.isEmpty() || !stricmp(ThisUNC, ForwardUNCPath)) &&
          (MountPath == NULL || ThisPath == MountPath))
			{
				retcode = TRUE;
				break;
			}
		}

		fclose(f);
	}
	else
		printf("Unable to open %s\n", (LPCSTR)FileName);

	return retcode;
}

///////////////////////////////////////////////////////////////////////////////

QString GetHostName()
{
	char hostname[MAXHOSTNAMELEN+1];
	memset(hostname, 0, sizeof(hostname));
	gethostname(hostname, MAXHOSTNAMELEN);

	return QString(hostname);
}

///////////////////////////////////////////////////////////////////////////////

QString GetIPAddress()
{
	struct hostent *h = gethostbyname((LPCSTR)GetHostName());

	return QString(NULL == h ? "" : inet_ntoa(*(struct in_addr *)(h->h_addr)));
}

///////////////////////////////////////////////////////////////////////////////

QString GetHomeDir(LPCSTR UserName)
{
	struct passwd *pw = getpwnam(UserName);

	return QString(NULL == pw ? "" : pw->pw_dir);
}

///////////////////////////////////////////////////////////////////////////////

QString GetGroupName(LPCSTR UserName)
{
	struct passwd *pw = getpwnam(UserName);

	if (NULL != pw)
	{
		struct group *gr;

		while ((gr = getgrent()) != NULL)
		{
			if (gr->gr_gid == pw->pw_gid)
				return QString(gr->gr_name);
		}
	}

	return QString("");
}

///////////////////////////////////////////////////////////////////////////////

BOOL IsUNCPath(LPCSTR s)
{
	return (NULL != s && (s[0] == '\\' || s[0] == '/') && s[0] == s[1] && s[2] != '\0');
}

///////////////////////////////////////////////////////////////////////////////

void SplitPath(LPCSTR Path, QString& Parent, QString& FileName)
{
	char Slash = Path[0] == '\\' ? '\\' : '/';

	LPCSTR p = Path;
	int nLen = strlen(p);

	if (nLen > 0)
		p += nLen - 1;

	while (*p != Slash && p > Path)
		p--;

	if (p > Path)
	{
#ifdef QT_20
    Parent = (LPCSTR)QString(Path).left(p - Path);
#else
    Parent = (LPCSTR)QString(Path, p - Path + 1);
#endif
		FileName = p+1;
	}
	else
	{
		FileName = p;
		char buf[2];
		buf[0] = Slash;
		buf[1] = '\0';
		Parent = buf;
	}
}

///////////////////////////////////////////////////////////////////////////
//
// Functions below are used to connect to our network services server
// (corel netserv)

int GetServerOpenHandle(LPCSTR ServerCommand)
{
  int fd;
  struct sockaddr_un unix_addr;
  static int ServerCount = 0;

  if ((fd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
    //printf("Unable to create socket!\n");
		return -1;
	}

  memset(&unix_addr, 0, sizeof(unix_addr));
  unix_addr.sun_family = AF_UNIX;
  sprintf(unix_addr.sun_path, "%s%05d.%d", CLI_PATH, getpid(), ServerCount++);
  unlink(unix_addr.sun_path);

  if (bind(fd, (struct sockaddr *) &unix_addr, SUN_LEN(&unix_addr)) < 0)
	{
    //printf("Unable to bind!\n");
		return -2;
	}

  if (chmod(unix_addr.sun_path, CLI_PERM) < 0)
	{
    //printf("Unable to chmod!\n");
		return -3;
	}

  memset(&unix_addr, 0, sizeof(unix_addr));

  unix_addr.sun_family = AF_UNIX;
  strcpy(unix_addr.sun_path, COREL_SERVERNAME);

  if (connect(fd, (struct sockaddr *) &unix_addr, SUN_LEN(&unix_addr)) < 0)
	{
    //printf("Unable to connect!\n");
		return -4;
	}

  write(fd, ServerCommand, strlen(ServerCommand)+1);

  return fd;
}

///////////////////////////////////////////////////////////////////////////////

FILE *ServerOpen(LPCSTR ServerCommand)
{
  int fd = GetServerOpenHandle(ServerCommand);

  return fd < 0 ? NULL : fdopen(fd, "r");
}

///////////////////////////////////////////////////////////////////////////////

QString GetServerVariable(LPCSTR Variable)
{
	QString s;

	s.sprintf("getenv %s", Variable);

	int fd = GetServerOpenHandle(s);

	s = "";

	if (fd >= 0)
	{
		char buf[1024];
		int n = read(fd, buf, sizeof(buf)-1);

		buf[n] = '\0';
		s = buf;

		close(fd);
	}

	return s;
}

///////////////////////////////////////////////////////////////////////////////

int ServerExecute(LPCSTR Command)
{
	int fd = GetServerOpenHandle(Command);

	int retcode = -1;

	if (fd >= 0)
	{
		char buf[1024];
		int n = read(fd, buf, sizeof(buf)-1);

		buf[n] = '\0';
		retcode = atoi(buf);

		close(fd);
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

void WriteLog(LPCSTR s)
{
	FILE *f = fopen("/var/log/Corel.netserv.log", "a");

	if (NULL != f)
	{
		time_t x = time(NULL);
		fprintf(f, "%24.24s: %s\n", ctime(&x), s);
		fclose(f);
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL LocateSmbMount()
{
	int i;

	LPCSTR PossibleLocationsMount[] =
	{
		"/usr/bin/smbmount-2.2.x",
		"/usr/bin/smbmount-2.1.x",
		"/usr/sbin/smbmount",
		"/usr/bin/smbmount",
		"/sbin/smbmount",
		"/bin/smbmount",
		"/usr/local/samba/bin/smbmount"
	};

	LPCSTR PossibleLocationsUmount[] =
	{
		"/usr/bin/smbumount-2.2.x",
		"/usr/bin/smbumount-2.1.x",
		"/usr/sbin/smbumount",
		"/usr/bin/smbumount",
		"/sbin/smbumount",
		"/bin/smbumount",
		"/usr/local/samba/bin/smbumount"
	};

	for (i=0; i < (int)(sizeof(PossibleLocationsMount)/sizeof(LPCSTR)); i++)
	{
		if (!access(PossibleLocationsMount[i], 0))
		{
			gSmbMountLocation = PossibleLocationsMount[i];

			if (!i)
      {
        gbUseSmbMount_2_2_x = TRUE;

        FILE *f = popen(gSmbMountLocation, "r");

        if (NULL != f)
        {
          while (!feof(f))
          {
            char buf[256];
            fgets(buf, sizeof(buf), f);

            buf[strlen(buf)-1] = '\0';

            if (strlen(buf) > 8 && !strncmp(buf, "Version ", 8))
            {
              gSmbMountVersion = buf+8;
              break;
            }
          }

          pclose(f);
        }
        //printf("%s\n", (LPCSTR)gSmbMountVersion);
      }
			else
				if (1 == i)
					gbUseSmbMount_2_1_x = TRUE;

			break;
		}
	}

	if (gSmbMountLocation == NULL)
	{
		WriteLog("Unable to locate smbmount");
		return FALSE;
	}

	for (i=0; i < (int)(sizeof(PossibleLocationsUmount)/sizeof(LPCSTR)); i++)
	{
		if (!access(PossibleLocationsUmount[i], 0))
		{
			gSmbUmountLocation = PossibleLocationsUmount[i];
			break;
		}
	}

	if (gSmbUmountLocation == NULL)
	{
		WriteLog("Unable to locate smbumount");
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void ParseURL(LPCSTR Url, QString& Hostname, QString& SiteRelativePath, int& nCredentialsIndex)
{
	QString URL(Url);
	ExtractCredentialsFromURL(URL, nCredentialsIndex);

	Url = (LPCSTR)URL;

	if (IsFTPUrl(Url))
		Url += 6;

	LPCSTR z = Url;

	while (*z != '\0' && *z != '\\' && *z != '/')
		z++;

#ifdef QT_20
  Hostname = QString(Url).left(z - Url);
#else
  Hostname = QString(Url, z - Url + 1);
#endif

	SiteRelativePath = MakeSlashesForward(z);

	if (SiteRelativePath.isEmpty())
		SiteRelativePath = "/";
}

///////////////////////////////////////////////////////////////////////////////

QString GetParentURL(LPCSTR Url)
{
	LPCSTR p = Url;

	while (*p != '\0' && *p != ':')
		p++;

	char Slash = (*p != '\0') ? *++p : *Url;

	p = Url + strlen(Url) - 1;

	if (p > Url && *p == Slash)
		p--;

	while (p > Url && *p != Slash)
		p--;

	if (p > Url+2 && p[-1] == '/' && p[-2] == '/' && p[-3] == ':')
		p++;

#ifdef QT_20
  return QString(Url).left(p - Url);
#else
  return QString(Url, p - Url + 1);
#endif
}

///////////////////////////////////////////////////////////////////////////////

void MakeURL(LPCSTR Parent, LPCSTR File, QString& Result)
{
	LPCSTR SlashChar = "/";

	if (Parent == NULL || *Parent == '\0' || *Parent == '/' || *Parent == '\\')
	{
		if (*Parent == '\\')
			SlashChar = "\\";

		Result = "file://";
	}
	else
		Result = "";

	QString Part2;

	if (NULL != Parent)
		Part2 += Parent;

	if (NULL != File)
	{
		if (Part2[Part2.length()-1] != SlashChar[0])
			Part2 += SlashChar;

		Part2 += File;
	}

	URLEncode(Part2);
	Result += Part2;
}

///////////////////////////////////////////////////////////////////////////////

BOOL CountFolderContents(
	LPCSTR Path,
	unsigned long& dwNumFiles,
	unsigned long& dwNumFolders,
	double& dwTotalSize,
	CFileJob *pResult,
	BOOL bRecursive,
	BOOL bContainersGoLast,
	int BaseNameLength)
{
	if (IsFTPUrl(Path))
	{
		CFtpSession *pSession = gFtpSessions.GetSession(NULL, Path);

		if (NULL != pSession)
			return pSession->CountFolderContents(Path, dwNumFiles, dwNumFolders, dwTotalSize, pResult, bRecursive, bContainersGoLast, BaseNameLength);

		return FALSE;  // unable to establish session...
	}

	int BaseNameExtra = 0;

	if (strlen(Path) > 7 && !strnicmp(Path, "file://", 7))
	{
		Path += 7;
	}

	LPCSTR UNCPath = NULL;

	if (IsUNCPath(Path))
	{
		static QString szBuffer;

		if (netmap(szBuffer, Path))
			return FALSE; // unable to netmap

		UNCPath = Path;

		Path = (LPCSTR)szBuffer;
	}

	if (BaseNameLength == -1)
	{
		LPCSTR x = Path + strlen(Path) - 1;

		if (((*x == '/') || (*x == '\\')) && x > Path) // ignore trailing slash...
		{
			x--;
		}

		while (*x != '/' && *x != '\\' && x > Path)
			x--;

		BaseNameLength = (x - Path) + BaseNameExtra;
	}

	struct stat st;

	if (lstat(Path, &st))
		return FALSE; // unable to stat, may be not there or permission denied

	if (S_ISDIR(st.st_mode))
	{
		if (NULL != pResult) // && !bContainersGoLast)
			pResult->append(new CFileJobElement(Path, st.st_mtime, st.st_size, st.st_mode, dwNumFiles, dwTotalSize, BaseNameLength, 0));

		char *buf = new char[strlen(Path)+2];
		strcpy(buf, Path);

		if (buf[strlen(buf)-1] != '/')
			strcat(buf, "/");

		DIR *thisDir = opendir(Path);

		if (thisDir == NULL)
			return FALSE;

		struct dirent *p;

		while ((p = readdir(thisDir)) != NULL)
		{
			if (!strcmp(p->d_name, ".") ||
				!strcmp(p->d_name, ".."))
				continue;

			char *filename = new char[strlen(buf)+strlen(p->d_name)+1];

			strcpy(filename, buf);
			strcat(filename, p->d_name);

			struct stat st2;

			if (!lstat(filename, &st2))
			{
				dwTotalSize += st2.st_size;

				if (S_ISDIR(st2.st_mode))
				{
					dwNumFolders++;

					if (bRecursive)
						CountFolderContents(filename, dwNumFiles, dwNumFolders, dwTotalSize, pResult, TRUE, bContainersGoLast, BaseNameLength);
				}
				else
				{
					if (NULL != pResult)
					{
						/*CFileJobElement el(filename, dwNumFiles, dwTotalSize);*/
						/*pResult->Add(el);*/

						pResult->append(new CFileJobElement(filename, st2.st_mtime, st2.st_size, st2.st_mode,
							dwNumFiles, dwTotalSize, BaseNameLength, 1));
					}

					dwNumFiles++;
				}
			}

			delete []filename;
			qApp->processEvents();
		}

		closedir(thisDir);

		if (NULL != pResult && bContainersGoLast)
			pResult->append(new CFileJobElement(Path, st.st_mtime, st.st_size, st.st_mode, dwNumFiles, dwTotalSize, BaseNameLength, 2));

		delete []buf;
	}
	else
	{
		dwTotalSize += st.st_size;
		dwNumFiles++;

		if (NULL != pResult)
			pResult->append(new CFileJobElement(Path, st.st_mtime, st.st_size, st.st_mode, dwNumFiles, dwTotalSize, BaseNameLength, 1));
	}

	if (NULL != UNCPath)
	{
		gTreeExpansionNotifier.ScheduleUnmap(UNCPath);
	}

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

int FileStat(LPCSTR Url, struct stat *st)
{
	return IsFTPUrl(Url) ? FTPStat(Url, st) : lstat(Url, st);
}

///////////////////////////////////////////////////////////////////////////////

int FileUnlink(LPCSTR Url)
{
	return IsFTPUrl(Url) ? FTPUnlink(Url) : unlink(Url);
}

///////////////////////////////////////////////////////////////////////////////

BOOL IsValidFileName(LPCSTR s)
{
	QString InvalidChars = "\\/:*?\"<>|";

	while (*s != '\0')
	{
		if (InvalidChars.contains(*s++))
			return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

QPixmap *GetFilePixmap(const QString& s, BOOL bIsLink, BOOL bIsExecutable, BOOL bIsBig)
{
	struct __Associations
	{
		LPCSTR m_Extension;
		CPixmapType m_Type;
		CPixmapType m_TypeBig;
	} Associations[] =
		{
			{ "exe", keProgIcon, keProgBigIcon },
			{ "dll", keProgIcon, keProgBigIcon },
			{ "zip", keZipFileIcon, keZipFileBigIcon },
			{ "gz", keZipFileIcon, keZipFileBigIcon },
			{ "z", keZipFileIcon, keZipFileBigIcon },
			{ "tgz", keZipFileIcon, keZipFileBigIcon },
			{ "tar", keZipFileIcon, keZipFileBigIcon },
			{ "wpd", keWordPerfectFileIcon, keWordPerfectFileBigIcon },
			{ "doc", keWordPerfectFileIcon, keWordPerfectFileBigIcon },
			{ "rtf", keWordPerfectFileIcon, keWordPerfectFileBigIcon },
			{ "wpt", keWordPerfectFileIcon, keWordPerfectFileBigIcon },
			{ "wcm", keWordPerfectFileIcon, keWordPerfectFileBigIcon },
			{ "cpp", keCXXFileIcon, keCXXFileBigIcon },
			{ "c", keCXXFileIcon, keCXXFileBigIcon },
			{ "cxx", keCXXFileIcon, keCXXFileBigIcon },
			{ "h", keHeaderFileIcon, keHeaderFileBigIcon },
			{ "hpp", keHeaderFileIcon, keHeaderFileBigIcon },
			{ "htm", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "html", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "asp", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "htx", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "shtml", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "alx", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "stm", keHTMLFileIcon, keHTMLFileBigIcon },
			{ "rm", keAudioFileIcon, keAudioFileBigIcon },
			{ "ra", keAudioFileIcon, keAudioFileBigIcon },
			{ "wav", keAudioFileIcon, keAudioFileBigIcon },
			{ "au", keAudioFileIcon, keAudioFileBigIcon },
			{ "mp3", keAudioFileIcon, keAudioFileBigIcon },
			{ "ram", keAudioFileIcon, keAudioFileBigIcon },
			{ "bmp", keImageFileIcon, keImageFileBigIcon },
			{ "xpm", keImageFileIcon, keImageFileBigIcon },
			{ "dib", keImageFileIcon, keImageFileBigIcon },
			{ "gif", keImageFileIcon, keImageFileBigIcon },
			{ "jpg", keImageFileIcon, keImageFileBigIcon },
			{ "jpeg", keImageFileIcon, keImageFileBigIcon },
			{ "pcx", keImageFileIcon, keImageFileBigIcon },
			{ "pic", keImageFileIcon, keImageFileBigIcon },
			{ "cpt", keImageFileIcon, keImageFileBigIcon },
			{ "tif", keImageFileIcon, keImageFileBigIcon },
			{ "tiff", keImageFileIcon, keImageFileBigIcon },
			{ "cdr", keDrawFileIcon,  keDrawFileIconBig },
			{ "cdt", keDrawFileIcon,  keDrawFileIconBig },
			{ "cgm", keDrawFileIcon,  keDrawFileIconBig },
			{ "cmx", keDrawFileIcon,  keDrawFileIconBig },
			{ "emf", keDrawFileIcon,  keDrawFileIconBig },
			{ "cpt", keDrawFileIcon,  keDrawFileIconBig }
		};

	const int NumAssociations = sizeof(Associations) / sizeof(struct __Associations);

	int nIndex = s.findRev(".");

	if (nIndex != 0)
	{
		QString Extension = s.mid(nIndex+1, s.length()).lower();

		for (int i=0; i < NumAssociations; i++)
		{
			if (Associations[i].m_Extension == Extension)
				return LoadPixmap(bIsBig ? Associations[i].m_TypeBig : Associations[i].m_Type, bIsLink);
		}
	}

	return LoadPixmap(bIsExecutable ? (bIsBig ? keProgBigIcon : keProgIcon) : (bIsBig ? keRegFileIconBig : keRegFileIcon), bIsLink);
}

///////////////////////////////////////////////////////////////////////////////

QString SplitString(LPCSTR s, int nMaxChar)
{
	LPCSTR p;
	QString ret;

	while (*s != '\0')
	{
		for (p=s; *p != '\0' && p < s+nMaxChar; p++);

		if (*p != '\0')
		{
			while (p > s+1 && *p != ' ' && *p != '\t')
				p--;
		}

		if (!ret.isEmpty())
			ret += "\n";

		while (*s == ' ' || *s == '\t')
			s++;

#ifdef QT_20
    ret += QString(s).left(p-s);
#else
    ret += QString(s, p-s+1);
#endif
		s = p;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

void URLEncode(QString& _url)
{
	int old_length = _url.length();

	if (!old_length)
		return;

	// a worst case approximation
	char *new_url = new char[ old_length * 3 + 1 ];
	int new_length = 0;

	int nLeaveAsIs = 0;

	if (old_length > 5)
	{
		if (!strnicmp(_url, "ftp://", 6))
			nLeaveAsIs = 6;
		else
			if (!strnicmp(_url, "http://", 7))
				nLeaveAsIs = 7;
	}

	for (int i = 0; i < old_length; i++)
	{
		static const char *safe = "$-._!*(),/"; /* RFC 1738 */
		// '/' added by David, fix found by Michael Reiher

#ifdef QT_20
    char t = _url[i].latin1();
#else
    char t = _url[i];
#endif

		if (i < nLeaveAsIs || ((t >= 'A') && (t <= 'Z')) || ((t >= 'a') && (t <= 'z')) || ((t >= '0') && (t <= '9')) || (strchr(safe, t)))
#ifdef QT_20
      new_url[ new_length++ ] = _url[i].latin1();
#else
      new_url[ new_length++ ] = _url[i];
#endif
		else
		{
	    new_url[ new_length++ ] = '%';

	    unsigned char c = ((unsigned char)_url[ i ]
#ifdef QT_20
      .latin1()
#endif
                         ) / 16;
	    c += (c > 9) ? ('A' - 10) : '0';
	    new_url[ new_length++ ] = c;

	    c = ((unsigned char)_url[ i ]
#ifdef QT_20
      .latin1()
#endif
           ) % 16;
	    c += (c > 9) ? ('A' - 10) : '0';
	    new_url[ new_length++ ] = c;

		}
	}

	new_url[new_length]=0;
	_url = new_url;

	delete []new_url;
}

///////////////////////////////////////////////////////////////////////////////

static uchar hex2int( char _char ) {
    if ( _char >= 'A' && _char <='F')
	return _char - 'A' + 10;
    if ( _char >= 'a' && _char <='f')
	return _char - 'a' + 10;
    if ( _char >= '0' && _char <='9')
	return _char - '0';
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void URLDecode(QString& _url)
{
	int old_length = _url.length();

	if (!old_length)
		return;

	int new_length = 0;

	// make a copy of the old one

	char *new_url = new char[old_length + 1];

	for (int i = 0; i < old_length; i++)
	{
		uchar character = _url[i]
#ifdef QT_20
      .latin1()
#endif
      ;

		if (character == '%')
		{
			character = hex2int(_url[i+1]
#ifdef QT_20
      .latin1()
#endif
                           ) * 16 + hex2int(_url[i+2]
#ifdef QT_20
      .latin1()
#endif
                                            );
			i += 2;
		}

		new_url[new_length++] = character;
	}

	new_url[new_length] = 0;
	_url = new_url;

	delete []new_url;
}

///////////////////////////////////////////////////////////////////////////////

void URLDecodeSmart(QString &s)
{
	URLDecode(s);

	int nLen = s.length();

	if (nLen < 6)
		return;

	// file:/XXX/YYY --> /XXX/YYY
	// file:///XXX/YYY --> /XXX/YYY
	// file://\\XXX\YYY --> \\XXX\YYY

	if (!strnicmp((LPCSTR)s, "file:", 5))
	{
		int nStart = 5;

		if (s[5] == '/' &&
			  s[6] == '/' &&
				((nLen > 7 && s[7] == '/') || (nLen > 8 && s[7] == '\\' && s[8] == '\\')))
			nStart+=2;

		s = (LPCSTR)s + nStart;
	}
}

///////////////////////////////////////////////////////////////////////////////

void ExtractCredentialsFromURL(QString &URL, int& nCredentialsIndex)
{
	//printf("ExtractCredentialsFromURL: %s on start\n", (LPCSTR)URL);
  QString x;

  int n1 = URL.find("://");
  x = (-1 == n1) ? URL : URL.mid(n1+3, URL.length());

  int n2 = x.find('/');

  if (-1 != n2)
     x = x.left(n2);

  int n3 = x.findRev('@');

  if (-1 == n3)
    nCredentialsIndex = 1;
  else
  {
    QString cr = x.left(n3);
    int n4 = cr.find(':');
    QString UserName, Password;

    if (-1 == n4)
  	  UserName = cr;
	  else
	  {
  		UserName = cr.left(n4);
		  Password = cr.mid(n4+1, cr.length());
	  }

    QString Host = x.mid(n3+1, x.length());

    n2 = URL.find(cr);
    URL = URL.left(n2) + UserName + URL.mid(n2+cr.length(), URL.length());

		//printf("Username=[%s], Password = [%s], Host = [%s]\n", (LPCSTR)UserName, (LPCSTR)Password, (LPCSTR)Host);
    CCredentials cred(UserName, Password, Host);

		nCredentialsIndex = gCredentials.Find(cred);

		if (nCredentialsIndex == -1)
		  nCredentialsIndex = gCredentials.Add(cred);
		else
			if (!cred.m_Password.isEmpty() &&
					gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
			{
				//printf("Password %s --> %s\n", (LPCSTR)gCredentials[nCredentialsIndex].m_Password, (LPCSTR)cred.m_Password);
				gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
			}
	}

  //printf("On End: %s\n, returning %d", (LPCSTR)URL, nCredentialsIndex);
}

///////////////////////////////////////////////////////////////////////////////

void FileModeToString(QString& s, mode_t mode)
{
  s = "";
	if (S_ISDIR(mode))
		s += "d";
	else
		if (S_ISLNK(mode))
			s += "l";
		else
			if (S_IFCHR == (mode & S_IFMT))
				s += "c";
			else
				if (S_IFBLK == (mode & S_IFMT))
					s += "b";
				else
					if (S_ISSOCK(mode))
						s += "s";
					else
						if (S_ISFIFO(mode))
							s += "p";
						else
							s += "-";

	s += ((mode & S_IRUSR) == S_IRUSR) ? "r" : "-";
	s += ((mode & S_IWUSR) == S_IWUSR) ? "w" : "-";
	s += ((mode & S_IXUSR) == S_IXUSR) ? (((mode & S_ISUID) == S_ISUID) ? "s" : "x") : (((mode & S_ISUID) == S_ISUID) ? "S" : "-");
	s += ((mode & S_IRGRP) == S_IRGRP) ? "r" : "-";
	s += ((mode & S_IWGRP) == S_IWGRP) ? "w" : "-";
	s += ((mode & S_IXGRP) == S_IXGRP) ? (((mode & S_ISGID) == S_ISGID) ? "s" : "x") : (((mode & S_ISGID) == S_ISGID) ? "S" : "-");
	s += ((mode & S_IROTH) == S_IROTH) ? "r" : "-";
	s += ((mode & S_IWOTH) == S_IWOTH) ? "w" : "-";
	s += ((mode & S_IXOTH) == S_IXOTH) ? (((mode & S_ISVTX) == S_ISVTX) ? "t" : "x") : (((mode & S_ISVTX) == S_ISVTX) ? "T" : "-");
}

///////////////////////////////////////////////////////////////////////////////

QString GetHomeDir()
{
	LPCSTR s = getenv("HOME");

	if (NULL != s)
		return QString(s);

	struct passwd *pwd = getpwuid(getuid());

	if (NULL != pwd)
		return QString(pwd->pw_dir);

	return QString("~");
}

///////////////////////////////////////////////////////////////////////////////
/*
	 returns:
		0 - retry
		1 - abort
		2 - ignore
*/

int ReportCommonFileError(LPCSTR FileName,
                          int nError,
                          BOOL bNeedIgnore,
                          int nCaptionID,
                          int nPrefix /* = -1 */,
                          int nIgnoreID /* = -1 */)
{
	QString Caption;

  if (-1 != nCaptionID)
    Caption = LoadString(nCaptionID);
  else
    Caption = qApp->mainWidget()->caption();

  if (nIgnoreID == -1)
    nIgnoreID = knIGNORE;

	QSTRING_WITH_SIZE(msg, 256 + strlen(FileName));

	switch (nError)
	{
		case EFAULT:
			msg.sprintf(LoadString(knEFAULT), FileName);
		break;

		case EACCES:
		case EPERM:
			msg.sprintf(LoadString(knEACCES), FileName);
		break;

		case ENAMETOOLONG:
		{
			// Retry won't help... so do special case here.

			msg.sprintf(LoadString(knENAMETOOLONG), FileName);

			if (-1 != nPrefix)
				msg = LoadString(nPrefix) + QString("\n") + msg;

			if (bNeedIgnore)
			{
				return QMessageBox::critical(qApp->mainWidget(),
																				Caption,
																				(LPCSTR)msg,
																				LoadString(knIGNORE),
																				LoadString(knCANCEL),
	 																			NULL,
																				0,
																				1) ? 1:2;
			}
			else
			{
				QMessageBox::critical(qApp->mainWidget(),
															Caption,
															(LPCSTR)msg,
															LoadString(knOK),
															NULL,
															NULL,
															0);
				return 1;
			}
		}
		break;

		case ENOENT:
			msg.sprintf(LoadString(knENOENT), FileName);
		break;

		case ENOTDIR:
			msg.sprintf(LoadString(knENOTDIR), FileName);
		break;

		case EISDIR:
			msg.sprintf(LoadString(knEISDIR), FileName);
		break;

		case ENOMEM:
			msg.sprintf(LoadString(knENOMEM), FileName);
		break;

		case EROFS:
			msg.sprintf(LoadString(knEROFS), FileName);
		break;

		case ELOOP:
			msg.sprintf(LoadString(knELOOP), FileName);
		break;

		case EIO:
			msg.sprintf(LoadString(knEIO), FileName);
		break;

		case ENOTEMPTY:
			msg.sprintf(LoadString(knENOTEMPTY), FileName);
		break;

		case ETXTBSY:
      msg.sprintf(LoadString(knETXTBSY), FileName);
    break;

    case ECONNREFUSED:
      msg = LoadString(knECONNREFUSED);
    break;

		case ENOSPC:
			msg.sprintf(LoadString(knENOSPC), FileName);
		break;

		case EEXIST:
			msg.sprintf(LoadString(knEEXIST), FileName);
		break;
	}

	if (-1 != nPrefix)
	{
		msg = LoadString(nPrefix) + QString("\n") + msg;
	}

	if (bNeedIgnore)
	{
    int ret = QMessageBox::critical(qApp->mainWidget(),
                                    Caption,
                                    (LPCSTR)msg,
                                    LoadString(knSTR_RETRY),
                                    LoadString(nIgnoreID),
                                    LoadString(knABORT));

		if (!ret)
			return 0;

		if (1 == ret)
			return 2;

		return 1;
	}
	else
		return (!QMessageBox::critical(qApp->mainWidget(),
                                   Caption,
                                   (LPCSTR)msg,
                                   LoadString(knSTR_RETRY),
                                   LoadString(knABORT))) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

QPixmap *GetIconFromKDEFile(LPCSTR FileName, BOOL bIsBig)
{
	FILE *f = fopen(FileName, "r");

	if (NULL != f)
	{
		char buf[1024];
		QString IconName, MiniIconName;
    BOOL bIsTrash = FALSE;

		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			buf[strlen(buf)-1] = '\0';

			if (!strncmp(buf, "Icon=", 5))
				IconName = &buf[5];
			else
				if (!strncmp(buf, "MiniIcon=", 9))
					MiniIconName = &buf[9];
        else
          if (!strcmp(buf, "Name=Trash") || !strcmp(buf, "Name=Dumpster"))
            bIsTrash = TRUE;

			if (!IconName.isEmpty() && !MiniIconName.isEmpty())
				break;
		}

		fclose(f);

    //printf("FileName = %s, %d %s \n", FileName, bIsTrash, (LPCSTR)IconName);

    if (bIsTrash && (IconName.isEmpty() || MiniIconName.isEmpty()))
    {
      return LoadPixmap(bIsBig ? keDumpsterIconBig : keDumpsterIcon);
    }

		if (!IconName.isEmpty() || !MiniIconName.isEmpty())
		{
#ifdef QT_20
      QString FileName("/usr/X11R6/share/icons");
#else
      QString FileName = KApplication::kde_icondir();
#endif

			if (FileName.right(1) != "/")
				FileName += "/";

			if (bIsBig)
				FileName += IconName.isEmpty() ? MiniIconName : IconName;
			else
			{
				FileName += "mini/";
				FileName += MiniIconName.isEmpty() ? IconName : MiniIconName;
			}

			QPixmap *pIcon = LoadCachePixmap(FileName);

			if (NULL != pIcon && pIcon->width() != 0)
				return pIcon;
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

BOOL CopyFile(LPCSTR FileIn, LPCSTR FileOut)
{
	FILE *fi = fopen(FileIn, "r");

	if (NULL == fi)
		return FALSE; // source is unreadable

	FILE *fo = fopen(FileOut, "w");

	if (NULL == fo)
		return FALSE; // destination is non-writable

	BOOL retcode = TRUE;

	fseek(fi, 0L, 2);
	long TotalSize = ftell(fi);

	fseek(fi, 0L, 0);

	int bufsize = (TotalSize > 8192) ? 8192 : TotalSize;

	unsigned char *buf = new unsigned char[bufsize];

	while (TotalSize > 0)
	{
		size_t ReadNow = (TotalSize > bufsize) ? bufsize : TotalSize;

		if (ReadNow != fread(buf, 1, ReadNow, fi) ||
				ReadNow != fwrite(buf, 1, ReadNow, fo))
		{
			retcode = FALSE;
			break;
		}

		TotalSize -= ReadNow;
	}

	fclose(fi);
	fclose(fo);
	return retcode;
}

///////////////////////////////////////////////////////////////////////////////

void CTreeExpansionNotifier::DoStartWorking()
{
	m_nWorkingLevel++;
	//printf("START: Working Level = %d\n", m_nWorkingLevel);

	if (m_nWorkingLevel == 1)
		emit StartWorking();
}

///////////////////////////////////////////////////////////////////////////////

void CTreeExpansionNotifier::DoEndWorking()
{
	if (m_nWorkingLevel > 0)
		m_nWorkingLevel--;

	//printf("END: Working Level = %d\n", m_nWorkingLevel);

	if (!m_nWorkingLevel)
		emit EndWorking();
}

///////////////////////////////////////////////////////////////////////////////

// checks if s2 is in subtree of s1
//
// returns: 0: not in subtree and s1 != s2
//					1: s1 == s2
//          2: is in subtree

int IsInSubtree(LPCSTR s1, LPCSTR s2)
{
	QString p1(s1);
	QString p2(s2);

	if (p1.right(1) != "/")
		p1 += "/";

	if (p2.right(1) != "/")
		p2 += "/";

	if (p1.length() > p2.length())
		return 0;

  if (p1 == p2)
		return 1;

  if (p2.left(p1.length()) == p1)
		return 2;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

BOOL IsTrashFolder(LPCSTR Name)
{
  QString FileName(Name);

  if (FileName.right(1) != "/")
    FileName += "/";

  int nTrashNameLen = strlen(gTrashID);
  int nFileNameLen = FileName.length();

  if (nFileNameLen < nTrashNameLen)
    return FALSE;

  if (strcmp((LPCSTR)FileName+nFileNameLen-nTrashNameLen, gTrashID))
    return FALSE;

  FILE *f = fopen(FileName + ".directory", "r");

  if (NULL == f)
    return FALSE;

  char buf[80];
  fgets(buf, sizeof(buf)-1, f);

	if (strcmp(buf, "[KDE Desktop Entry]\n") &&
			strcmp(buf, "[Desktop Entry]\n"))
  {
    fclose(f);
    return FALSE;
  }

  BOOL retcode = FALSE;

	while (!feof(f))
	{
		fgets(buf, sizeof(buf)-1, f);
		
		if (!strcmp(buf, "Name=Trash\n") || 
				!strcmp(buf, "Name=Dumpster\n"))
			retcode = TRUE;
	}
  
	fclose(f);

  return retcode;
}

///////////////////////////////////////////////////////////////////////////////

BOOL IsMyTrashFolder(LPCSTR FileName)
{
  if (!IsTrashFolder(FileName))
    return FALSE;

  QString TrashPath = GetHomeDir();

  if (TrashPath.right(1) != "/")
    TrashPath += "/";

  TrashPath += "Desktop/Trash";

  if (FileName[strlen(FileName)-1] == '/')
    TrashPath += "/";

  return TrashPath == FileName;
}

///////////////////////////////////////////////////////////////////////////////

typedef LPCSTR (*LPFORMATDATETIME)(time_t datetime);

LPFORMATDATETIME gLPFNFormatDateTime = NULL;

///////////////////////////////////////////////////////////////////////////////
/* USA/Canada */

LPCSTR FormatDateTime_US(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	char AMPM[100];
	strftime(AMPM, sizeof(AMPM), "%I:%M %P" , &Tm);
	QString AMPMString(AMPM[0] == '0' ? &AMPM[1] : AMPM);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%d/%d/%d %s",
		Tm.tm_mon+1,
		Tm.tm_mday,
		Tm.tm_year > 99 ? Tm.tm_year + 1900 : Tm.tm_year,
		(LPCSTR)AMPMString.upper());

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////
/* French standard */

LPCSTR FormatDateTime_fr(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%.2d/%.2d/%.2d %.2d:%.2d",
		Tm.tm_mday,
		Tm.tm_mon+1,
		Tm.tm_year,
		Tm.tm_hour,
    Tm.tm_min);

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////
/* German */

LPCSTR FormatDateTime_de(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%.2d.%.2d.%.2d %.2d:%.2d",
		Tm.tm_mday,
		Tm.tm_mon+1,
		Tm.tm_year,
		Tm.tm_hour,
    Tm.tm_min);

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////
/* Spanish */

LPCSTR FormatDateTime_es(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%d.%.2d.%.2d %.2d:%.2d",
		Tm.tm_mday,
		Tm.tm_mon+1,
		Tm.tm_year,
		Tm.tm_hour,
    Tm.tm_min);

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////
/* Dutch */

LPCSTR FormatDateTime_nl(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%d-%d-%.2d %d:%.2d",
		Tm.tm_mday,
		Tm.tm_mon+1,
		Tm.tm_year,
		Tm.tm_hour,
    Tm.tm_min);

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////
/* Czech */

LPCSTR FormatDateTime_cs(time_t datetime)
{
  struct tm Tm = *localtime(&datetime);

	static char TimeBuf[100];

	sprintf(
		TimeBuf,
		"%d.%d.%.4d %.2d:%.2d",
		Tm.tm_mday,
		Tm.tm_mon+1,
		Tm.tm_year + 1900,
		Tm.tm_hour,
    Tm.tm_min);

  return &TimeBuf[0];
}

///////////////////////////////////////////////////////////////////////////////

LPCSTR FormatDateTime(time_t datetime)
{
  if (NULL == gLPFNFormatDateTime)
  {
#ifdef QT_20
    QString language = "en";
#else
    QString language = KApplication::getKApplication()->getLocale()->language();
#endif

    //printf("LANGUAGE = %s\n", (LPCSTR)language);

    if (language == "fr" ||
        language == "en_UK" ||
        language == "it")
      gLPFNFormatDateTime = FormatDateTime_fr;
    else
      if (language == "es")
        gLPFNFormatDateTime = FormatDateTime_es;
      else
        if (language == "de")
          gLPFNFormatDateTime = FormatDateTime_de;
        else
          if (language == "nl")
            gLPFNFormatDateTime = FormatDateTime_nl;
          else
            if (language == "cs")
              gLPFNFormatDateTime = FormatDateTime_cs;
            else
              gLPFNFormatDateTime = FormatDateTime_US;
  }

  return (*gLPFNFormatDateTime)(datetime);
}

///////////////////////////////////////////////////////////////////////////////
//
// Quick check if a local directory has any subfolders.
//

BOOL LocalHasSubfolders(LPCSTR Path)
{
  QString FullName(Path);

	if (FullName.right(1) != "/")
		FullName += "/";

  if (access(Path, X_OK))
		return FALSE; // no access!

	DIR *thisDir = opendir(Path);

	if (NULL == thisDir)
		return FALSE;

	struct dirent *p;
  BOOL retcode = FALSE;

	while ((p = readdir(thisDir)) != NULL)
	{
    if (!strcmp(p->d_name, ".") ||
      !strcmp(p->d_name, ".."))
      continue;

    struct stat st;

    if (!stat(FullName + p->d_name, &st))
    {
      if (S_ISDIR(st.st_mode))
      {
        retcode = TRUE;
        break;
      }
    }
  }

  closedir(thisDir);

  return retcode;
}

///////////////////////////////////////////////////////////////////////////////
//
// Check if the current process is owned by Super-User
// or has effective UID/GID of one.
//
// Note that UID is not necessarily 0, it is sufficient
// to have 0 GID to be super user...
//

BOOL IsSuperUser()
{
  return
		!getuid() ||
		!getgid() ||
		!geteuid() ||
		!getegid();
}

///////////////////////////////////////////////////////////////////////////////
//
// Check is NFS server is installed
// We just look for some typical files...
//

BOOL HasNFSSharing()
{
	return
		!access("/etc/init.d/nfs-server", 0) &&
		!access("/usr/sbin/rpc.nfsd", 0);
}

///////////////////////////////////////////////////////////////////////////////

bool IsShared(LPCSTR pPath)
{
	return
		IsNFSShared(pPath) ||
		NULL != gSambaConfiguration.FindShare(pPath);
}

///////////////////////////////////////////////////////////////////////////////

static const char *HiddenPrefixes[] =
{
  "printers://",
  "printer://",
  "workgroups://",
  "workgroup://",
  "nfsservers://"
};

static unsigned int nHiddenPrefixesCount = (sizeof(HiddenPrefixes)/sizeof(const char *));

///////////////////////////////////////////////////////////////////////////////

LPCSTR GetHiddenPrefix(const unsigned int nIndex)
{
  return (nIndex < nHiddenPrefixesCount) ? HiddenPrefixes[nIndex] : "";
}

///////////////////////////////////////////////////////////////////////////////

QString AttachHiddenPrefix(const char *text, int nHiddenPrefix)
{
  if (-1 == nHiddenPrefix)
    return QString(text);

  return QString(GetHiddenPrefix(nHiddenPrefix)) + text;
}

///////////////////////////////////////////////////////////////////////////////

LPCSTR DetachHiddenPrefix(LPCSTR text, int& nHiddenPrefix)
{
  nHiddenPrefix = -1;

  for (int i=0; i < (int)nHiddenPrefixesCount; i++)
  {
    unsigned int nLen = strlen(GetHiddenPrefix(i));

    if (strlen(text) > nLen &&
        !strnicmp(text, GetHiddenPrefix(i), nLen))
    {
      nHiddenPrefix = i;
      text += nLen;
      break;
    }
  }

  return text;
}

///////////////////////////////////////////////////////////////////////////////

QObject *FindChildByName(QWidget *w, LPCSTR Name)
{
	if (NULL == w)
		return NULL;

	QObjectList *list = (QObjectList*)(w->children());

	if (NULL != list)
	{
		QObject *p;

		for (p=list->first(); p != NULL; p = list->next())
		{
			if (!strcmp(p->name(), Name))
					return p;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

QString GetLocalFileTip(int nType, BOOL bIsLink, BOOL bIsFolder, const QString& ShortName, const QString& FullName, const QString& TargetName)
{
  QString Path = FullName;

  int maxwidth = (nType ? 65 : 40);

	if (bIsLink)
  {
    QString ret;

    if (nType)
    {
      ret = ShortName;
      ret += "\n";
    }

    ret += "-> ";
    ret += TargetName;
    return ret;
  }

	if (bIsFolder)
	{
		QString DirFile = Path;

    if (DirFile.right(1) != "/")
			DirFile += "/";

    DirFile += ".directory";

		FILE *f = fopen(DirFile, "r");

		if (NULL != f)
		{
			char buf[1024];
			while (!feof(f))
			{
				fgets(buf, sizeof(buf)-1, f);

				if (feof(f))
					break;

				if (!strncmp(buf, "Comment=", 8))
				{
					if (nType)
						Path = ShortName;

					int nLen = Path.length();
					int maxlen = nLen > maxwidth ? nLen : maxwidth;

					Path += "\n";
					buf[strlen(buf)-1] = '\0';
					Path += SplitString(&buf[8], maxlen);
					break;
				}
			}

			fclose(f);
		}

		return Path;
	}

	FILE *f = popen("file \"" + Path + "\"", "r");

	if (NULL != f)
	{
		char buf[1024];

		while (!feof(f))
		{
			fgets(buf, sizeof(buf)-1, f);

			if (feof(f))
				break;

			buf[strlen(buf)-1] = '\0';

			int nLen = Path.length();
			int maxlen = nLen > maxwidth ? nLen : maxwidth;

			QString Description;

			if (!strncmp(buf, (LPCSTR)Path, nLen)) // replace ':' with newline, that looks better...
				Description = SplitString(&buf[nLen+1], maxlen);
			else
				Description = SplitString(buf, maxlen);

			if (nType)
				Path = ShortName;

			Path += "\n" + Description;
		}

		if (Path.right(1) == ",")
			Path = Path.left(Path.length()-1);

		pclose(f);
	}

	return Path;
}

////////////////////////////////////////////////////////////////////////////

QPixmap *DefaultFilePixmap(LPCSTR FullName,
                           LPCSTR ShortName,
                           BOOL bIsBig,
                           LPCSTR FileAttributes,
                           mode_t TargetMode,
                           LPCSTR TargetName,
                           BOOL bIsLink,
                           BOOL bIsFolder)
{
  if (bIsFolder)
	{
		QString DirFile = FullName;

		if (DirFile.right(1) != "/")
			DirFile += "/";

		DirFile += ".directory";

		QPixmap *pIcon = GetIconFromKDEFile(DirFile, bIsBig);

		if (NULL != pIcon)
			return pIcon;

		return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, bIsLink, IsShared(FullName));
	}

	switch (FileAttributes[0])
	{
 		case 's':
			return LoadPixmap(keMSRootIcon);

		case 'c':
		case 'b':
			return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon);

		case 'p':
			return LoadPixmap(keFIFOIcon);

		case 'l':
		{
			if (TargetMode == 0xffffffff)
				return LoadPixmap(bIsBig ? keBrokenLinkBigIcon : keBrokenLinkIcon);

			if (S_ISDIR(TargetMode))
				return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, 1);
			else
				if (S_IFCHR == (TargetMode & S_IFMT) || S_IFBLK == (TargetMode & S_IFMT))
					return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon, 1);
				else
					if (S_ISFIFO(TargetMode))
						return LoadPixmap(keFIFOIcon, 1);
					else
						return GetFilePixmap(TargetName,
                                 TRUE,
                                 (TargetMode & S_IXUSR) == S_IXUSR ||	(TargetMode & S_IXGRP) == S_IXGRP || (TargetMode & S_IXOTH) == S_IXOTH,
                                 bIsBig);
		}
	}

  if (strlen(ShortName) > 7 && !strcmp(ShortName + strlen(ShortName) - 7, ".kdelnk"))
	{
		QPixmap *pIcon = GetIconFromKDEFile(FullName, bIsBig);

		if (NULL != pIcon)
			return pIcon;
	}

	return GetFilePixmap(ShortName, FALSE, NULL != strchr(FileAttributes,'x'), bIsBig);
}

////////////////////////////////////////////////////////////////////////////

QString SqueezeString(LPCSTR str, unsigned int maxlen)
{
  QString s(str);

  if (s.length() > maxlen)
  {
    int part = (maxlen - 3) / 2;
    return QString(s.left(part) + "..." + s.right(part));
  }

  return s;
}

////////////////////////////////////////////////////////////////////////////

BOOL IsScreenSaverRunning()
{
	static QString LockFilePath;

	if (LockFilePath.isEmpty())
	{
		LockFilePath = getenv("HOME");
    LockFilePath += "/.kss-install.pid.";
    char ksshostname[200];
		gethostname(ksshostname, 200);
		LockFilePath += ksshostname;
	}

	BOOL bFound = FALSE;

	FILE *fp;

	if ((fp = fopen(LockFilePath, "r")) != NULL)
	{
		int pid;
		struct stat st;

		fscanf(fp, "%d", &pid);
		fclose(fp);

		if (pid > 0 &&
        stat(LockFilePath
#ifdef QT_20
  .latin1()
#endif
                          , &st) >=0 &&
				(st.st_mode & S_IXOTH) == S_IXOTH)
			bFound = TRUE;
	}

	return bFound;
}

////////////////////////////////////////////////////////////////////////////

BOOL IsSamePath(LPCSTR Path1, LPCSTR Path2)
{
	if (!strcmp(Path1, Path2))
		return TRUE;

	int l1 = strlen(Path1);
	int l2 = strlen(Path2);

	if (l2 > 0 && l1 == l2+1 && Path1[l1-1] == '/' && Path2[l2-1] != '/')
		return TRUE;

	if (l1 > 0 && l2 == l1+1 && Path2[l2-1] == '/' && Path1[l1-1] != '/')
		return TRUE;

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

