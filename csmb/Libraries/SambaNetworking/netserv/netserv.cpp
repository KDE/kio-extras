/* Name: netserv.cpp

   Description: This file is a part of the netserv application.

   Authors:  Oleg Noskov
             Chris Ellison

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stddef.h>
#include <time.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h> // for waitpid
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include "PasswordDlg.h"
#include "qstring.h"
#include "qlist.h"
#include <qvaluelist.h>
#include "table.h"
#include "common.h"
#include <sys/param.h> // for MAXPATHLEN
#include <sys/sysmacros.h> // for major()
#include <ctype.h>
#include "inifile.h"
#include "usermang.h"
#include <iostream.h>
#include <qfile.h>
#include <qtextstream.h>

void ExtractWord(LPSTR Result, int nResultSize, LPCSTR& x, const char *SpaceCharList = "\t ");
char LogBuf[1024];

int gArgc;
char **gArgv;
QApplication *gpApp;

FILE *PopenUID(LPCSTR cmd, uid_t uid, gid_t gid);
void PcloseUID(FILE *f);

//added by Chris Ellison
bool gMapperDisabled = FALSE;
char gTempMntDir[1024];
bool gDoGarbageCleanup = FALSE;
QList <CProcessEntry> gProcessTable;
QList <CMountEntry> gMountTable;
#define CORELMNTDIR "/CorelMntDir"
#define CORELTMPDIR "/CorelTmpDir"
#define UNCMOUNTNAME "uncm"
#define TAGNAME "/uncmounttagfile"
#define PROCPATH "/proc"
#define GARBAGETIME 60


#define CS_OPEN "/var/tmp/corel.netserver"
#define CL_OPEN "open"
#define MAXLINE 2048
#define STALE 30

typedef struct
{
    int fd;
    uid_t uid;
    gid_t gid;
    FILE *f;
    int peerfd;
} Client;



extern Client *client;
extern int client_size; /* # of entries in client[] array */

int cli_agrs(int, char **);
int client_add(int, uid_t, gid_t, FILE *, int);
void client_del(int);
void loop(void);
FILE *request(char *, int, int, uid_t, gid_t, BOOL&, int&);

void netmap(int cliFd, pid_t PID, uid_t UID, gid_t GID, const char* UNC);
int netunmap(pid_t PID, uid_t UID, gid_t GID, const char* UNC);
void notify(int cliFd, const char *MountPoint);
bool DoGarbageCleanup();
bool RemoveMountDir(LPCSTR szDirectory);
void getcred(LPCSTR Name);


#define NALLOC 10
Client *client = NULL;
int client_size;


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


class CCredentialsCacheEntry
{
public:
	CCredentialsCacheEntry()
	{
	}

	CCredentialsCacheEntry(const CCredentialsCacheEntry &other)
	{
		*this = other;
	}

	CCredentialsCacheEntry& operator=(const CCredentialsCacheEntry& other)
  {
		m_ShareName = other.m_ShareName;
		m_UserName = other.m_UserName;
		m_Password = other.m_Password;
		m_Workgroup = other.m_Workgroup;

		return *this;
	}

	QString m_ShareName;
	QString m_UserName;
	QString m_Password;
	QString m_Workgroup;
};

typedef QList<CCredentialsCacheEntry> CCredentialsCache;

CCredentialsCache gCredentialsCache;


////////////////////////////////////////////////////////////////////////////

BOOL CanMountAt(uid_t uid, LPCSTR Path)
{
	struct stat st;
	return
		stat(Path, &st) == 0 &&
		S_ISDIR(st.st_mode) &&
		(uid == 0 || uid == st.st_uid) &&
		((st.st_mode & S_IRWXU) == S_IRWXU);
}

////////////////////////////////////////////////////////////////////////////

short ExecuteCommand(LPCSTR cmd, uid_t uid, gid_t gid)
{
	pid_t pid;
	int status = -1;

	if ((pid = fork()) < 0)
	{
		WriteLog("Unable to fork()");
		exit(-1);
	}
	else
	{
		if (pid == 0)
		{
			/* child */
			close(1); // close stdout
			close(2); // close stderr
      setgid(gid);
			setuid(uid);
			execl("/bin/sh", "sh", "-c", cmd, NULL);
			exit(127);     /* execl error */
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
	return (short)status;
}



////////////////////////////////////////////////////////////////////////////

void PopupMessageBox(LPCSTR s)
{
	pid_t pid = fork();
	if (pid == 0)
	{
		QApplication a(gArgc, gArgv);
		QMessageBox::critical(NULL, "Error", s);
		exit(0);
	}
}



////////////////////////////////////////////////////////////////////////////

//
// This is a simplified version of GetIP from smbutil.cpp.
//
QString GetIP(LPCSTR Service)
{
	QString s;
	QString Server = ExtractWord(Service, "/\\");
	FILE *f;
	BOOL bRFlag = FALSE;

TryAgain:;
	s.sprintf("nmblookup %s", (LPCSTR)Server);
	f = popen(s,"r");
	s = "";
	if (NULL != f)
	{
		while (!feof(f))
		{
			char buf[2048];
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

void DoMount(LPCSTR szUNC,
						 LPCSTR szMountPoint,
						 pid_t PID,
						 uid_t UID,
						 gid_t GID,
						 int cliFd)
{
	if (fork())
		return;

	QApplication *pA = NULL;
  char szStatusBuf[1024];
	QString UserId, GroupId, ServiceName, MountPoint, UserName, Domain, cmd;
	bool bCancel = FALSE;
	bool bMountFail = FALSE;
	struct stat statbuf1;
	struct stat statbuf2;
	char *ARGV[] = { "netserv", "netserv", NULL };
	int ARGC = 2;
	BOOL bTriedEmpty = FALSE;

	UserId.sprintf("%u", UID);
	GroupId.sprintf("%u", GID);

	short status;
	QString Password(getenv("DEFAULTPASSWORD"));

	QString Server;
	QString Share;
	QString Path;

	//parse the UNC pathname

	if (!ParseUNCPath(szUNC, Server, Share, Path))
	{
		WriteLog("DoMount: Invalid UNC path");
	}
	else
	{
		ServiceName.sprintf("//%s/%s", (LPCSTR)Server, (LPCSTR)Share);
		MountPoint = szMountPoint;

		QString DoubleSlashName;
		DoubleSlashName.sprintf("\\\\\\\\%s\\\\%s", (LPCSTR)Server, (LPCSTR)Share);
		getcred(DoubleSlashName);
/*    QListIterator<CCredentialsCacheEntry> it(gCredentialsCache);
    for (it.toFirst(); NULL != it.current(); ++it)
    {
      cout<<"it.current().m_Workgroup="<<endl;
    }
*/
		UserName = getenv("CURRENTUSER");
		Domain = getenv("CURRENTWORKGROUP");
	}

	int nRetryCount=5;

DoAgain:;
	if (gbUseSmbMount_2_2_x)
	{
		//cmd.sprintf("%s%%%s", (LPCSTR)UserName, (LPCSTR)Password);
		//setenv("USER", cmd, 1);
    extern QString gSmbMountVersion;
    cout<<"Samba= 2_2_x"<<endl;
    cout<<"USER= "<<getenv("USER")<<endl;
    cout<<"CURRENTUSER="<<getenv("CURRENTUSER")<<endl;
    cout<<"CURRENTWORKGROUP="<<getenv("CURRENTWORKGROUP")<<endl;
		cmd.sprintf(gSmbMountVersion.left(5) == "2.0.5" ?
                "\"%s\" \"%s\" \"%s\" -W \"%s\"" :
                "\"%s\" \"%s\" \"%s\" -o workgroup=\"%s\"%s",
				gSmbMountLocation,
				(LPCSTR)ServiceName,
				(LPCSTR)MountPoint,
				(LPCSTR)Domain,
				(NULL == getenv("USER") || strcmp(getenv("USER"), "%")) ? "" : ",username=,password=");
	}
	else
	{
		if (gbUseSmbMount_2_1_x)
		{
			//cmd.sprintf("%s%%%s", (LPCSTR)UserName, (LPCSTR)Password);
			//setenv("USER", cmd, 1);
      cout<<"Samba= 2_1_x"<<endl;
			cmd.sprintf("\"%s\" \"%s\" -c \"mount %s -u %s -g %s\" -W \"%s\"",
				gSmbMountLocation,
				(LPCSTR)ServiceName,
				(LPCSTR)MountPoint,
				(LPCSTR)UserId,
				(LPCSTR)GroupId,
				(LPCSTR)Domain);
		}
		else
		{
      cout<<"Samba= I don't know"<<endl;
			cmd.sprintf("\"%s\" \"%s\" \"%s\" -U %s -D \"%s\" -P \"%s\" -u %s -g %s",
				gSmbMountLocation,
				(LPCSTR)ServiceName,
				(LPCSTR)MountPoint,
				(LPCSTR)UserName,
				(LPCSTR)Domain,
				(LPCSTR)Password,
				(LPCSTR)UserId,
				(LPCSTR)GroupId);

			QString IP = GetIP(ServiceName);

			if (!IP.isEmpty())
			{
				cmd.append(" -I ");
				cmd.append(IP);
			}
		}
	}

  cout<<"cmd="<<cmd<<endl;
	status = ExecuteCommand(cmd, UID, GID);
  cout<<"After ExecuteCommand in DoMount"<<endl;

	struct stat st;

	if (stat(MountPoint, &st) && (errno == EIO || errno == EACCES))
	{
		WriteLog("I/O Error detected");
		cmd.sprintf("%s \"%s\"", (LPCSTR)gSmbUmountLocation, (LPCSTR)MountPoint);
		//WriteLog(cmd);
		ExecuteCommand(cmd, UID, GID);
		status = 256;
	}

  cout<<"After stat in DoMount"<<endl;
	sprintf(szStatusBuf, "%d", status);

	//WriteLog(szStatusBuf);

	if (status == -256 || status == 256)
	{
    cout<<"in check status in DoMount"<<endl;
		if (!bTriedEmpty)
		{
			setenv("USER", "%", 1);
			bTriedEmpty = TRUE;
			goto DoAgain;
		}

		//if (status == -256 && nRetryCount--)	// on Debian, we may need to retry several times...
		//	goto DoAgain;

		int nRet;

		if (NULL == pA)
			pA = new QApplication(ARGC, ARGV);

		CPasswordDlg dlg(ServiceName, Domain, UserName);

		nRet = dlg.exec();

		if (1 == nRet)
		{
			UserName = dlg.m_UserName;
			Domain = dlg.m_Workgroup;
			Password = dlg.m_Password;
      QString s;
      s.sprintf(" \"%s\" \"%s\"  \"%s\"  \"%s\"",
				szUNC,
				(LPCSTR)UserName,
        (LPCSTR)Password,
				(LPCSTR)Domain
				);
      cout<<"s= "<<s<<endl;
      BOOL tmp;
      int tmp2;
      FILE *CommandSetcred(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD);
      CommandSetcred((LPCSTR)s.latin1(), NULL, tmp, 0, 0, 0, tmp2);
/*
      CCredentials cred(UserName,Password,Domain);

			int nCredentialsIndex = gCredentials.Find(cred);

      if (nCredentialsIndex == -1)
				nCredentialsIndex = gCredentials.Add(cred);
			else
				if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
					gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
*/
			cmd.sprintf("%s%%%s", (LPCSTR)UserName, (LPCSTR)Password);
			setenv("USER", cmd, 1);
			goto DoAgain;
		}
		else
			bCancel = TRUE;
	}
	else
	{
    cout<<"in else of check status in DoMount"<<endl;
		time_t timenow = time(NULL);
WaitLoop:;

		if (stat((const char*)gTempMntDir, &statbuf1) < 0)
		{
			WriteLog("DoMount: unable to stat the parent mount directory");
			bMountFail = TRUE;
		}
		if (stat((const char*)MountPoint, &statbuf2) < 0)
		{
			WriteLog("DoMount:	unable to stat the mountpoint");
			bMountFail = TRUE;
		}

		if (statbuf1.st_dev == statbuf2.st_dev)
		{
			if (!status && time(NULL) < timenow+2)
			{
				goto WaitLoop;
			}

			bMountFail = TRUE;
		}
    cout<<"in the end of else of check status in DoMount"<<endl;
	}

	if (!bCancel && !bMountFail)
	{
    cout<<"in if of !bCancel && !bMountFail in DoMount"<<endl;
		char szCommand[1024];
		QString EncodedMountPoint(MountPoint);
		QString EncodedUNC(szUNC);

		URLEncode(EncodedMountPoint);
		URLEncode(EncodedUNC);

		sprintf(szCommand,
						"notify %d %s %s %d %u %u %d",
						knNetmapSuccess,
						(LPCSTR)EncodedMountPoint,
						(LPCSTR)EncodedUNC,
						PID,
						UID,
						GID,
						cliFd);

		int fd = GetServerOpenHandle(szCommand);

		if (fd >= 0)
			close(fd);
    cout<<"in the end of if of !bCancel && !bMountFail in DoMount"<<endl;
	}
	else
	{
		if (bCancel)
		{
			char szMessage[1024];
			sprintf(szMessage, "%d The request was cancelled by the client", knNetmapCancelled);
			notify(cliFd, (const char*)szMessage);

			if (!RemoveMountDir(MountPoint))
			{
				sprintf(szMessage, "DoMount, cancelled request: Unable to remove the mount directory %s", (LPCSTR)MountPoint);
				WriteLog(szMessage);
			}
		}
		else
		{
			if (bMountFail)
			{
				char szMessage[1024];
				sprintf(szMessage, "%d Failed to mount the specified location", knNetmapError);
				notify(cliFd, (const char*)szMessage);

				if (!RemoveMountDir(MountPoint))
				{
					sprintf(szMessage, "DoMount, mount failed: Unable to remove the mount directory %s", (LPCSTR)MountPoint);
					WriteLog(szMessage);
				}
			}
		}
	}

	// We have to notify our parent process that we are about to
	// terminate. Parent will waitpid(...) and this will prevent us going into zombie state.
	ServiceName.sprintf("waitpid %d", getpid());

  cout<<"Before GetServerOpenHandle in DoMount"<<endl;
	int fd = GetServerOpenHandle(ServiceName);
  cout<<"After GetServerOpenHandle in DoMount"<<endl;

	if (fd >= 0)
		close(fd);

  cout<<"In the end of DoMount"<<endl;
	exit(0);
}

////////////////////////////////////////////////////////////////////////////

int IsSpaceChar(char c, const char *SpaceCharList)
{
    return strchr(SpaceCharList, c) != NULL;
}

////////////////////////////////////////////////////////////////////////////

void ExtractWord(LPSTR Result, int nResultSize, LPCSTR& x, const char *SpaceCharList /* = "\t " */)
{
    LPCSTR p1, p2;
    // Skip opening spaces
    for (p1 = x; *p1 != '\0' && IsSpaceChar(*p1, SpaceCharList); p1++);
	  for (p2 = p1; *p2 != '\0' && !IsSpaceChar(*p2, SpaceCharList) && *p2 != '\n'; p2++)
  	{
			if (*p2 == '\\')
		    	p2++;
   	}
    x = p2;
    int n = p2 - p1;

    if (n > nResultSize-1)
			n = nResultSize-1;

    strncpy(Result, p1, n);
    Result[n] = '\0';
}



////////////////////////////////////////////////////////////////////////////

int serv_listen(const char *name)
{
    int fd;
    int len;
    struct sockaddr_un unix_addr;

	if ((fd=socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -1;

    unlink(name);

    memset(&unix_addr, 0, sizeof(unix_addr));

    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, name);

	if (bind(fd, (struct sockaddr *) &unix_addr, SUN_LEN(&unix_addr)) < 0)

		return -2;

    chmod(unix_addr.sun_path, 0777);

	if (listen(fd, 5) < 0)
		return -3;


	return fd;
}

////////////////////////////////////////////////////////////////////////////

int serv_accept(int listenfd, uid_t *uidptr, gid_t *gidptr)
{
  int clifd;
  unsigned int len;
  time_t staletime;
  struct sockaddr_un unix_addr;
  struct stat statbuf;

  len = sizeof(unix_addr);

	if ((clifd = accept(listenfd, (struct sockaddr *) &unix_addr, &len)) < 0)
		return -1;

  unix_addr.sun_path[SUN_LEN(&unix_addr)] = '\0';

	if (stat(unix_addr.sun_path, &statbuf) < 0)
		return -2;

#ifdef S_ISSOCK
  if (S_ISSOCK(statbuf.st_mode) == 0)
  	return -3;
#endif /* S_ISSOCK */

	if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
		(statbuf.st_mode & S_IRWXU) != S_IRWXU)
		return -4; /* is not rwx------ */

  staletime = time(NULL) - STALE;

  if (statbuf.st_atime < staletime || statbuf.st_ctime < staletime ||
		  statbuf.st_mtime < staletime)
		return -5; /* too old i-node */

	if (uidptr != NULL)
		*uidptr = statbuf.st_uid;

	if (gidptr != NULL)
		*gidptr = statbuf.st_gid;

  unlink(unix_addr.sun_path);

  return clifd;
}

////////////////////////////////////////////////////////////////////////////

static void client_alloc(void)
{
	int i;

	if (client == NULL)
		client = (Client*)malloc(NALLOC * sizeof(Client));
	else
		client = (Client*)realloc(client, (client_size + NALLOC) * sizeof(Client));

	if (NULL == client)
		WriteLog("malloc/realloc failure");

  for (i=client_size; i < client_size + NALLOC; i++)
  {
		client[i].fd = -1;
		client[i].f = NULL;
  }

	client_size += NALLOC;
}

////////////////////////////////////////////////////////////////////////////

int client_add(int fd, uid_t uid, gid_t gid, FILE *f, int peerfd)
{
  int i;

  if (NULL == client)
	client_alloc();

  while (1)
  {
		for (i=0; i < client_size; i++)
		{
			if (client[i].fd == -1) /* find an available entry */
			{
				client[i].fd = fd;
				client[i].uid = uid;
        client[i].gid = gid;
				client[i].f = f;
				client[i].peerfd = peerfd;

				return i;
			}
		}

		/* client array full, time to realloc for more */
		client_alloc();
	}
}

////////////////////////////////////////////////////////////////////////////

void client_del(int fd)
{
	int i;

	for (i=0; i < client_size; i++)
	{
		if (client[i].fd == fd)
		{
			client[i].fd = -1;

			if (client[i].f != NULL)
			{
				PcloseUID(client[i].f);
				client[i].f = NULL;
			}
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void loop(void)
{
	int i, n, maxfd, maxi, listenfd, clifd, nread;
	char buf[MAXLINE];

  uid_t uid;
  gid_t gid;

	fd_set rset, allset;

	FD_ZERO(&allset);

	if ((listenfd = serv_listen(CS_OPEN)) < 0)
	{
		WriteLog("serv_listen error");
		exit(-1);
  }

  FD_SET(listenfd, &allset);

  maxfd = listenfd;
  maxi = -1;

  while (1)
  {
		rset = allset; /* rset gets modified each time around */
Alarm:;
		if ((n = select(maxfd+1, &rset, NULL, NULL, NULL)) < 0)
		{
			if (gDoGarbageCleanup)
				goto Alarm;
			WriteLog("select error");
			exit(-1);
		}

		if (FD_ISSET(listenfd, &rset)) /* Accept new client request */
		{
			if ((clifd = serv_accept(listenfd, &uid, &gid)) < 0)
			{
				sprintf(LogBuf, "serv_accept error: %d", clifd);
				WriteLog(LogBuf);
			}

			i = client_add(clifd, uid, gid, NULL, 0);

			FD_SET(clifd, &allset);

			if (clifd > maxfd)
				maxfd = clifd;

			if (i > maxi)
				maxi = i;

			continue;
		}

		for (i=0; i <= maxi; i++)
		{
			if ((clifd = client[i].fd) < 0)
				continue;

			if (FD_ISSET(clifd, &rset))
			{
				if (client[i].f != NULL)
				{
					int peerfd = client[i].peerfd;

					if (feof(client[i].f))
					{
PipeEof:;
						client_del(clifd);
						FD_CLR(clifd, &allset);
						client_del(peerfd);
						FD_CLR(peerfd, &allset);
						close(peerfd);
					}
					else
					{
ReadAgain:;
            int nCount = read(client[i].fd, buf, sizeof(buf)-1);

            if (-1 == nCount)
            {
              if (EAGAIN == errno)
                goto ReadAgain;
              if (EIO == errno)
                goto PipeEof;
            }

            if (!nCount)
              goto PipeEof;

						write(peerfd, buf, nCount);
					}
				}
				else
				{
ReadAgain2:;
          if ((nread = read(clifd, buf, MAXLINE)) < 0)
					{
            if (-1 == nread && EAGAIN == errno)
              goto ReadAgain2;

            sprintf(LogBuf, "read error on fd %d", clifd);
            WriteLog(LogBuf);
					}
					else
					{
						if (nread == 0)
						{
CloseClient:;
							client_del(clifd);
							FD_CLR(clifd, &allset);
							close(clifd);
						}
						else
						{
							BOOL bCloseConnection;
							int CloseFD = -1;
							FILE *f = request(buf, nread, clifd, client[i].uid, client[i].gid, bCloseConnection, CloseFD);

							if (NULL != f)
							{
								int pipefd = fileno(f);
								int ix = client_add(pipefd, 0, 0, f, clifd);
								FD_SET(pipefd, &allset);

								if (pipefd > maxfd)
									maxfd = pipefd;

								if (ix > maxi)
									maxi = ix;
							}

							if (CloseFD != -1)
							{
								client_del(CloseFD);
								FD_CLR(CloseFD, &allset);
								close(CloseFD);
							}

							if (bCloseConnection)
								goto CloseClient;
						}
					}
				}
			}
		}
    }
}

////////////////////////////////////////////////////////////////////////////

void ShellExpand(char *s)
{
	char *save = s;
	char buf[1024];
	char *p = &buf[0];

	do
	{
		while (*s != '$' && *s != '\0')
			*p++ = *s++;

		if (*s == '$')
		{
			char *r = ++s;

			while (*s != '\0' &&
				   *s != ' ' &&
				   *s != '.' &&
				   *s != ';' &&
				   *s != '\'' &&
				   *s != '`' &&
				   *s != ',' &&
				   *s != '!' &&
				   *s != '%' &&
				   *s != '\t' &&
				   *s != '\r' &&
				   *s != '\n')
				s++;

			char variable[1024];
			strncpy(variable, r, s-r);
			variable[s-r] = '\0';

			char *q = getenv(variable);

			if (NULL != q)
			{
				strcpy(p, q);
				p += strlen(q);
			}
		}
	}
	while (*s != '\0');

	*p = '\0';
	strcpy(save, buf);
}

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	pid_t m_Pid;
	FILE *m_File;
} CSlaveProcessEntry;

static QList<CSlaveProcessEntry> gSlavePIDList;

FILE *PopenUID(LPCSTR cmd, uid_t uid, gid_t gid)
{
  pid_t pid;
	int input[2];

  pipe(input);

	if ((pid = fork()) < 0)
	{
		return NULL; // oops
	}

	if (pid == 0)
	{
		/* child */
		dup2(input[1], STDOUT_FILENO); /* stdout */
		close(input[0]);
		close(2); // close stderr
		setgid(gid);
		setuid(uid);
		execl("/bin/sh", "sh", "-c", cmd, NULL);
		exit(127);     /* execl error */
	}

	close(input[1]);

	CSlaveProcessEntry *pE = new CSlaveProcessEntry;
	pE->m_File = fdopen(input[0], "r");
	pE->m_Pid = pid;

	//sprintf(LogBuf,"File = %lx, pid = %ld", (long)pE->m_File, (long)pE->m_Pid);
  //WriteLog(LogBuf);

	gSlavePIDList.append(pE);

	return pE->m_File;
}

void PcloseUID(FILE *f)
{
	//sprintf(LogBuf,"PcloseUID: %lx, list has %d entries.",(long)f, gSlavePIDList.count());
	//WriteLog(LogBuf);

	fclose(f);

	CSlaveProcessEntry *pE;

	for (pE=gSlavePIDList.first(); NULL != pE && f != pE->m_File; pE=gSlavePIDList.next());

	if (NULL != pE)
	{
		//WriteLog("EntryFound!");

		int status;
		waitpid(pE->m_Pid, &status, 0);

		gSlavePIDList.setAutoDelete(TRUE);
		gSlavePIDList.removeRef(pE);
	}
	//else
		//WriteLog("EntryNotFound!");
}

////////////////////////////////////////////////////////////////////////////

typedef FILE *(*LPFNCommandHandler)(LPCSTR p,
																		LPCSTR pOriginalCommand,
																		BOOL& bCloseConnection,
																		uid_t uid,
																		gid_t gid,
																		int clifd,
																		int& CloseFD);

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	LPCSTR m_Command;
	LPFNCommandHandler m_Handler;
} CHandlerEntry;

////////////////////////////////////////////////////////////////////////////

FILE *CommandSmbclient(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	bCloseConnection = FALSE;
	return PopenUID(pOriginalCommand, uid, gid);
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandSmbmount(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	QString realcmd(QByteArray(1024));

	realcmd.sprintf("\"%s\" %s", gSmbMountLocation, p);

	int status = ExecuteCommand(realcmd, uid, gid);

	realcmd.sprintf("%d", (short)status);
	write(clifd, realcmd, realcmd.length());

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandGetenv(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	char variable[100];
	ExtractWord(variable, sizeof(variable), p);

	char *value = getenv(variable);

	if (value != NULL)
		strcpy(variable, getenv(variable));
	else
		variable[0] = '\0';

	write(clifd, variable, strlen(variable));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandSetenv(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	char variable[100];
	char value[100];

	ExtractWord(variable, sizeof(variable), p);
	ExtractWord(value, sizeof(value), p);
	ShellExpand(value);
	setenv(variable, value, 1);
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandLogout(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	ExecuteCommand("/usr/X11R6/bin/CopyAgent umount", uid, gid);
	exit(0);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandWaitpid(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	pid_t pid;
	int status;

	sscanf(p, " %d", &pid);
	waitpid(pid, &status, 0);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandSmbreset(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	if (!access("/etc/init.d/samba", X_OK)) // CorelLinux or any Debian-based
	{
		system("/etc/init.d/samba restart");
	}
	else // RedHat
	{
		system("/etc/rc.d/init.d/smb stop;/etc/rc.d/init.d/smb start");
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

BOOL RenameNFSExportedFolder(LPCSTR pszOldPath,
                             LPCSTR pszNewPath)
{
  LPCSTR FileName = "/etc/exports";
  struct stat st;

	char TmpFileName[] = "/tmp/~~exports~XXXXXX";
	mktemp(TmpFileName);

	FILE *fo = fopen((LPCSTR)TmpFileName, "w");

	if (NULL == fo)
		return FALSE;

  if (!stat(FileName, &st))
		fchown(fileno(fo), st.st_uid, st.st_gid);

	FILE *fi = fopen(FileName, "r");

	int nPathLen = strlen(pszOldPath);

	if (nPathLen > 1 && pszOldPath[nPathLen-1] == '/')
		nPathLen--;

	if (NULL != fi)
	{
		fseek(fi, 0L, 2);
		long nSize = ftell(fi);
		fseek(fi, 0L, 0);

		char *pBuf = new char[nSize+1];

		while (!feof(fi))
		{
			*pBuf = '\0';
			fgets(pBuf, nSize+1, fi);

			if (feof(fi) && !strlen(pBuf))
				break;

			int nLen = strlen(pBuf);

			if (nLen > 0 && pBuf[--nLen] == '\n')
				pBuf[nLen] = '\0';

      while (!feof(fi) && pBuf[strlen(pBuf)-1] == '\\')
			{
				fgets(pBuf+strlen(pBuf)-1, nSize+1, fi);

				if (pBuf[strlen(pBuf)-1] == '\n')
					pBuf[strlen(pBuf)-1] = '\0';
			}

			if ((int)strlen(pBuf) > nPathLen+1 &&
					!strncmp(pBuf, pszOldPath, nPathLen) &&
					(pBuf[nPathLen] == ' ' || pBuf[nPathLen] == '\t' ||
					(pBuf[nPathLen] == '/' && (pBuf[nPathLen+1] == ' ' || pBuf[nPathLen+1] == '\t'))))
			{
				if (NULL != pszNewPath)
          fprintf(fo, "%s%s\n", pszNewPath, pBuf+nPathLen);
			}
			else
				fprintf(fo, "%s\n", pBuf);
		}

		delete []pBuf;
		fclose(fi);
	}

	fclose(fo);

  unlink(FileName);
  rename(TmpFileName, FileName);

  ExecuteCommand("/etc/init.d/nfs-server restart", getuid(), getgid());
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandNFSUpdate(LPCSTR p, LPCSTR /*pOriginalCommand*/,
                       BOOL& /*bCloseConnection*/,
                       uid_t /*uid*/,
                       gid_t /*gid*/,
                       int /*clifd*/,
                       int& /*CloseFD*/)
{
	QString OldPath = ExtractQuotedWord(p);
  QString NewPath = ExtractQuotedWord(p);

  RenameNFSExportedFolder((LPCSTR)OldPath, NewPath.isEmpty() ? NULL : (LPCSTR)NewPath);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandNetmap(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	pid_t PID;
	uid_t UID;
	gid_t GID;
	const char* UNC;
	QString parameter;

	//preserve clifd
	bCloseConnection = FALSE;

	parameter = ExtractWord(p, " ");
	PID = (pid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	UID = (uid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	GID = (gid_t)atol((const char*)parameter);

	parameter = ExtractTail(p);
	UNC = (const char*)parameter;
	netmap(clifd, PID, UID, GID, UNC);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandNetunmap(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	pid_t PID;
	uid_t UID;
	gid_t GID;
	const char* UNC;
	QString parameter;
	int nResult;
	char buf[1024];

	bCloseConnection = FALSE;

	parameter = ExtractWord(p, " ");
	PID = (pid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	UID = (uid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	GID = (gid_t)atol((const char*)parameter);

	parameter = ExtractTail(p);
	UNC = (const char*)parameter;

	nResult = netunmap(PID, UID, GID, UNC);
	if (nResult == 0)
		sprintf(buf, "%d Successfully unmapped the specified location", nResult);
	else
		sprintf(buf, "%d Unable to unmap the specified location", nResult);

	write(clifd, (const char*)buf, strlen(buf));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandNotify(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	int nResultCode;
	pid_t PID;
	uid_t UID;
	gid_t GID;
	int nFd;
	QString szMount;
	QString szUNC;
	QString parameter;
	QString Server;
	QString Share;
	QString Path;
	char szFullMntPath[1024];
	char szShortUNC[1024];
	char szMessage[1024];

	parameter = ExtractWord(p, " ");
	nResultCode = atoi((const char*)parameter);

	szMount = ExtractWord(p, " ");
	URLDecode(szMount);

	szUNC = ExtractWord(p, " ");
	URLDecode(szUNC);

	ParseUNCPath((LPCSTR)szUNC, Server, Share, Path);
	sprintf(szShortUNC, "//%s/%s", (const char*)Server, (const char*)Share);
	sprintf(szFullMntPath, "%s%s", (LPCSTR)szMount, (LPCSTR)Path);

	parameter = ExtractWord(p, " ");
	PID = (pid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	UID = (uid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	GID = (gid_t)atol((const char*)parameter);

	parameter = ExtractWord(p, " ");
	nFd = atoi((const char*)parameter);

	CMountEntry *pMtemp = new CMountEntry;
	pMtemp->CreateEntry((const char*)szMount, (const char*)szShortUNC, UID, GID, PID);
	gMountTable.append(pMtemp);
	pMtemp = NULL;

	sprintf(szMessage, "%d %s", nResultCode, (const char*)szFullMntPath);

	//WriteLog((const char*)szMessage);
	notify(nFd, (const char*)szMessage);
	CloseFD = nFd;

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandEject(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	QString Location, Device;

	Location = ExtractWord(p);
	URLDecode(Location);

	Device = ExtractWord(p);
	URLDecode(Device);

	struct stat st;

	// Make sure it is really a CDROM (majors 3, 11, 22) or ZIP drive (major 8)

	if (!access(Location, 0) &&
			!stat(Device, &st) &&
			S_IFBLK == (st.st_mode & S_IFMT) &&
			((3 == major(st.st_rdev)) ||
       (11 == major(st.st_rdev)) ||
       (22 == major(st.st_rdev)) ||
       (8 == major(st.st_rdev))))
	{
		QString	command;
		command = "/bin/umount \"";
		command += Location;	// the umount command
		command += "\"&& /usr/bin/eject \"";
		command += Device;
		command += "\" &";

		system((LPCSTR)command);
		write(clifd, "0", 1);
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandFdumount(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	QString Location;
	Location = ExtractWord(p);
	URLDecode(Location);

	struct stat st;

	// Make sure it is floppy indeed...
	// or we have I/O error or EACCES error

	int ncode = stat(Location, &st);

	if ((ncode && (errno == EIO || errno == EACCES)) ||
			(!ncode && S_IFBLK == (st.st_mode & S_IFMT) && 2 == major(st.st_rdev)))
	{
		QString command;

		command.sprintf("/bin/umount \"%s\"", (LPCSTR)Location);

		int nret = system((LPCSTR)command);

		command.sprintf("%d", nret);
		write(clifd, command, strlen(command));
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////

void getcred(LPCSTR Name)
{
  QString ShareName(Name);
  cout<<"In getcred"<<endl;
  cout<<"pid="<<getpid()<< endl;
  QString tmp1;
  if (ShareName.right(1) != "\\")
		ShareName += "\\";

  if(ShareName.left(4) != "\\\\\\\\")
  {
    tmp1 = MakeSlashesBackwardDouble(Name);
    if (tmp1.right(1) == "\\")
		      tmp1.remove(tmp1.length()-1,1);
    else
      tmp1 += "\\";
  }
  else
    tmp1 = ShareName;

	QListIterator<CCredentialsCacheEntry> it(gCredentialsCache);

	unsigned int nMaxMatch=0;
	CCredentialsCacheEntry *pMatch = NULL;

  cout<<"Before for in getcred"<<endl;

  for (it.toFirst(); NULL != it.current(); ++it)
	{
    cout<<"In for"<<endl;

    QString tmp2(it.current()->m_ShareName);
    if(tmp2.left(4) != "\\\\\\\\")
      {
        tmp2 = MakeSlashesBackwardDouble(it.current()->m_ShareName);
        if (tmp2.right(1) == "\\")
		      tmp2.remove(tmp2.length()-1,1);
      }

    cout<<"ShareName= "<<tmp1<<endl;
    cout<<"it.current()->m_ShareName= "<<tmp2<<endl;
    if(tmp1 == tmp2)
//    if (it.current()->m_ShareName == ShareName)
		{
      cout<<"changed pMatch"<<endl;
			pMatch = it.current();

			break;
		}


    unsigned int l = it.current()->m_ShareName.length();

		if (l > nMaxMatch &&
				l < ShareName.length() &&
        !strnicmp(ShareName, it.current()->m_ShareName, l))
		{
      cout<<"changed pMatch"<<endl;
			nMaxMatch = l;
      pMatch = it.current();
		}
	}

	QString s;

	if (NULL != pMatch)
	{
    cout<<"In if"<<endl;
		s.sprintf("%s%%%s", (LPCSTR)pMatch->m_UserName, (LPCSTR)pMatch->m_Password);

		setenv("CURRENTWORKGROUP", (LPCSTR)pMatch->m_Workgroup, 1);
		setenv("CURRENTUSER", (LPCSTR)pMatch->m_UserName, 1);
	}
	else // entry not found, use default credentials
	{
    cout<<"In else"<<endl;
		s.sprintf("%s%%%s", getenv("DEFAULTUSER"), getenv("DEFAULTPASSWORD"));

		setenv("CURRENTWORKGROUP", getenv("DEFAULTWORKGROUP"), 1);
		setenv("CURRENTUSER", getenv("DEFAULTUSER"), 1);
	}
  cout<<"Before set USER "<<endl;
	setenv("USER", (LPCSTR)s, 1);
  cout<<"USER= "<<getenv("USER")<<endl;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandGetcred(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	getcred(MakeSlashesBackward(ExtractQuotedWord(p)));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandSetcred(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	//WriteLog(pOriginalCommand);

	CCredentialsCacheEntry e;

	e.m_ShareName = MakeSlashesBackward(ExtractQuotedWord(p));
  cout<<"In Commandsetcred"<<endl;
  cout<<"e.m_ShareName="<<e.m_ShareName<<endl;
	if (e.m_ShareName.right(1) != "\\")
		e.m_ShareName += "\\";
  //QString str= ExtractQuotedWord(p);
	e.m_UserName = ExtractQuotedWord(p);
	e.m_Password = ExtractQuotedWord(p);
	e.m_Workgroup = ExtractQuotedWord(p);
  cout<<"e.m_UserName="<<e.m_UserName<<endl;
  cout<<"e.m_Password="<<e.m_Password<<endl;
  cout<<"e.m_Workgroup="<<e.m_Workgroup<<endl;

	QListIterator<CCredentialsCacheEntry> it(gCredentialsCache);

	for (it.toFirst(); NULL != it.current(); ++it)
	{
    cout<<"In for of CommandSetcred"<<endl;
		if (it.current()->m_ShareName == e.m_ShareName)
			break;
	}

	if (NULL == it.current())
	{
    cout<<"Add CCredentialsCacheEntry"<<endl;
		gCredentialsCache.append(new CCredentialsCacheEntry(e));
  }
	else
		*it.current() = e;

  for (it.toFirst(); NULL != it.current(); ++it)
  {
    cout<<"it.current()->m_ShareName="<<it.current()->m_ShareName<<endl;
  }

	write(clifd, "0", 1);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandAdduser(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	QString UserName(ExtractQuotedWord(p));
	QString Password(ExtractQuotedWord(p));

	int retcode = 0;

	if (UserName.left(12) == "$_SHAREUSER_")
	{
		QString cmd;

		if (NULL == getpwnam(UserName)) // User not found, just add it at the end...
		{
      ::createUser((LPCSTR)UserName,
                 "ShareUsers",
                 "Share access account",
                 "/dev/null",
                 "/bin/false");

			cmd.sprintf("/usr/bin/smbpasswd -a \"\\%s\" \"%s\"", (LPCSTR)UserName, (LPCSTR)Password);
    }
		else
			cmd.sprintf("/usr/bin/smbpasswd \"\\%s\" \"%s\"", (LPCSTR)UserName, (LPCSTR)Password);

		//WriteLog(cmd);

		if (!retcode)
			ExecuteCommand(cmd, 0, 0);
	}
	else
		retcode = -2; // invalid user name

	Password.sprintf("%d", retcode);
	write(clifd, (LPCSTR)Password, 1); // return retcode

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandDeluser(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
	write(clifd, "0", 1);

	QString UserName(ExtractQuotedWord(p));

	if (UserName.left(12) == "$_SHAREUSER_")
	{
		if (NULL == getpwnam(UserName)) // User not found, bail out
			return NULL;

		LPCSTR TmpPasswdFile = "/etc/passwd.tmp$$";

		FILE *f = fopen(TmpPasswdFile, "w");

		if (NULL == f)
			return NULL;

		setpwent();
		struct passwd *p;

		while (NULL != (p=getpwent()))
		{
			if (strcmp(p->pw_name, UserName))
				putpwent(p, f);
		}

    endpwent();
		fclose(f);

		rename(TmpPasswdFile, "/etc/passwd");
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

FILE *CommandSMBSection(LPCSTR p, LPCSTR pOriginalCommand, BOOL& bCloseConnection, uid_t uid, gid_t gid, int clifd, int& CloseFD)
{
 	if (gSmbConfLocation.isEmpty())
    SearchSmbConfLocation();

  //WriteLog(gSmbConfLocation);

  char BakFileName[] = "/tmp/~~samba~configXXXXXX";
  LPSTR pData = NULL;
  char *retcode = "0";
  int nbytes;
  long FileSize;
  FILE *fi, *fo;
  long StartFileOffset, EndFileOffset;

  QString SectionName = ExtractQuotedWord(p);
  QString datalen = ExtractWord(p);

  sscanf((LPCSTR)datalen, "%d", &nbytes);

  if (nbytes < 0 || nbytes > 8192) // size of data should be from 0 to 8192 (for security reasons)
  {
    WriteLog("Invalid number of bytes");
    goto SafeExit;
  }

  if (nbytes > 0)
  {
    write(clifd, ">", 1); // Prompt for data stream

    pData = new char[nbytes];
    int nread;

    while (-1 == (nread=read(clifd, pData, nbytes)) && EAGAIN == errno);

    if (-1 == nread) // unable to read data
    {
      WriteLog("Unable to read data stream");
      goto SafeExit;
    }
  }

  fi = fopen(gSmbConfLocation, "r");

  if (NULL == fi)
  {
    WriteLog("Unable to open smb.conf for reading");
    goto SafeExit;
  }

 	mkstemp(BakFileName);

 	fo = fopen((LPCSTR)BakFileName, "w");

 	if (NULL == fo)
 	{
    WriteLog("Unable to open temp file for writing");
    fclose(fi);
    goto SafeExit;
  }

  // Re-read the source INI file to get proper offset values.

  if (!FindSectionOffsets(fi, SectionName, StartFileOffset, EndFileOffset))
  {
  	fseek(fi, 0L, 2);

  	StartFileOffset = ftell(fi);
  	EndFileOffset = StartFileOffset;
  }

  //printf("Writing section %s: start offset = %ld, end offset = %ld, file size = %ld\n",(LPCSTR)m_SectionName, StartFileOffset, EndFileOffset, ftell(fi));

  fseek(fi, 0L, 0);

  // First, write all preceding sections (if any)

  if (StartFileOffset > 0)
  {
  	char *pBuf = new char[StartFileOffset];

  	//printf("Writing start %ld bytes\n", StartFileOffset);

    if (StartFileOffset != (long)fread(pBuf, 1, StartFileOffset, fi) ||
      StartFileOffset != (long)fwrite(pBuf, 1, StartFileOffset, fo))
    {
      WriteLog("Error reading or writing");

      delete []pBuf;
      fclose(fi);
      fclose(fo);
      ::unlink(BakFileName);

      return NULL; // oops, unable to write
    }

    delete []pBuf;
  }

  // Now write section body itself

  if (NULL != pData &&
      nbytes != fwrite(pData, 1, nbytes, fo))
  {
    WriteLog("Error writing section body");
    goto SafeExit;   // error writing section body
  }

  // And copy the file tail from the source file.

  fseek(fi,0L,2);
  FileSize = ftell(fi);

  if (EndFileOffset < FileSize)
  {
    fseek(fi, EndFileOffset, 0);

    long lCount = FileSize - EndFileOffset;

    //printf("Writing %ld tail bytes\n", lCount);

    char *pBuf = new char[lCount];

    if (lCount != (long)fread(pBuf, 1, lCount, fi) ||
      lCount != (long)fwrite(pBuf, 1, lCount, fo))
    {
      WriteLog("Error reading/writing tail");

      delete []pBuf;
      fclose(fi);
      fclose(fo);
      goto SafeExit;
    }

    delete []pBuf;
  }

  // Finish up...

  fclose(fi);
  fclose(fo);

  if (!CopyFile(BakFileName, gSmbConfLocation))
  {
    WriteLog("Unable to CopyFile");
    goto SafeExit; // unable to copy file
  }

  retcode = "1";

SafeExit:;

  write(clifd, retcode, 1);

  if (NULL != pData)
    delete []pData;

  ::unlink(BakFileName);

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

CHandlerEntry HandlerArray[] =
{
	{ "smbclient", CommandSmbclient },
	{ "smbmount", CommandSmbmount },
	{ "getenv", CommandGetenv },
	{ "setenv", CommandSetenv },
	{ "logout", CommandLogout },
	{ "waitpid", CommandWaitpid },
	{ "smbreset", CommandSmbreset },
	{ "netmap", CommandNetmap },
	{ "netunmap", CommandNetunmap },
	{ "notify", CommandNotify },
	{ "eject", CommandEject },
	{ "fdumount", CommandFdumount },
	{ "getcred", CommandGetcred },     // use credentials from cache
	{ "setcred", CommandSetcred },			 // set credentials in chache
	{ "adduser", CommandAdduser }, // Add special user for share level access
	{ "deluser", CommandDeluser }, // Remove special user for share level access
  { "nfsupdate", CommandNFSUpdate }, // Modify /etc/exports
  { "smbsection", CommandSMBSection } // Replace section in smb.conf file
};

////////////////////////////////////////////////////////////////////////////

FILE *request(char *buf, int nread, int clifd, uid_t uid, gid_t gid, BOOL& bCloseConnection, int& CloseFD)
{
	char command[1024];
	LPCSTR pCommand = buf;
	bCloseConnection = TRUE;

	do
	{
		ExtractWord(command, sizeof(command), pCommand, ";");

    if (command[0] != '\0')
		{
			char word[20];
			LPCSTR p = &command[0];
			ExtractWord(word, sizeof(word), p);

			for (int i=0; i < (int)(sizeof(HandlerArray)/sizeof(CHandlerEntry)); i++)
			{
				if (!strcmp(word, HandlerArray[i].m_Command))
				{
					FILE *f = (*HandlerArray[i].m_Handler)(p, command, bCloseConnection, uid, gid, clifd, CloseFD);

					if (NULL != f)
						return f;

					break;
				}
			}
		}
	}
	while (command[0] != '\0');

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
//
//	Method: netmap()
//
//	Purpose:	This funtion handles all UNC mount requests for client netmap
//						calls.  This function adds entries to both the process table
//						and the mount table depending on the particular request.  The
//						actual smbmount request is achieved through a call to DoMount().
//
////////////////////////////////////////////////////////////////////////////////

void netmap(int nCliFd, pid_t nPID, uid_t nUID, gid_t nGID, const char* szUNC)
{
	bool Found = FALSE;
	char szShortUNC[1024];		//contains only the server and share
	char szFullUNC[1024];			//contains the server, share, and the path
	char szMount[1024];
	char szBuffer[1024];
	int nFd;
	char szMountPoint[1024];
  char szTagFile[1024];
	QString Server;
	QString Share;
	QString Path;
	QString szUNCforward;
  QListIterator <CProcessEntry> ProcessTableIter(gProcessTable);
  QListIterator <CMountEntry> MountTableIter(gMountTable);

	//check to see if the mapper functionality has been disabled
	if (gMapperDisabled)
	{
		sprintf(szBuffer, "%d Mapper functionality has been disabled", knNetmapError);
		notify(nCliFd, szBuffer);
		return;
	}

	//format the UNC path to only contain forward slashes
	szUNCforward = MakeSlashesForward(szUNC);

	//parse the UNC pathname
	if (!ParseUNCPath((const char*)szUNCforward, Server, Share, Path))
	{
	  WriteLog("Netmap: Invalid UNC path");
		sprintf(szBuffer, "%d Invalid UNC pathname", knNetmapUNCError);
	  notify(nCliFd, szBuffer);
	  return;
	}
	sprintf(szShortUNC, "//%s/%s", (const char*)Server, (const char*)Share);
	sprintf(szFullUNC, "%s%s", szShortUNC, (const char*)Path);

	//search process table to see if a match exists
  ProcessTableIter.toFirst();
  for(; ProcessTableIter.current() != NULL; ++ProcessTableIter)
  {
		if (nPID == ProcessTableIter.current()->GetPID())
		{
	    ProcessTableIter.current()->AddCount();
	    WriteLog("Netmap: Found PID in process table");
			Found = TRUE;
			break;
		}
  }

	//add to process table if no match was found
  if (!Found)
  {
		WriteLog("Netmap: Did not find PID in process table");
		CProcessEntry *pPtemp = new CProcessEntry(nPID);
		gProcessTable.append(pPtemp);
		pPtemp = NULL;
		gProcessTable.current()->AddCount();
  }

  //search the mount table for a match based on the UNC, UID and GID
  Found = FALSE;
  MountTableIter.toFirst();
  for (; MountTableIter.current() != NULL; ++MountTableIter)
  {
		if (MountTableIter.current()->SearchEntry((const char*)szShortUNC, nUID, nGID) == TRUE)
		{
			MountTableIter.current()->AddPID(nPID);
			int n = MountTableIter.current()->AddCount();
      //sprintf(szBuffer, "Netmap: Found a match,  count now at: %d", n);
      //WriteLog(szBuffer);
	    sprintf(szMount, "%s", MountTableIter.current()->GetMount());
	    Found = TRUE;
			break;
		}
  }

  //A match was not found therefore create a directory and mount the UNC there.
	//The mount location name is based on a defined name plus the current
	//	system time. A tag file is placed into the directory to provide the
	//	ability to determine whether or not the directory had been modified.
	if (!Found)
  {
		sprintf(szMountPoint, "%s/%s%d_%d", gTempMntDir, UNCMOUNTNAME, time(NULL), rand() % 10000);
		if (mkdir(szMountPoint, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
    {
      sprintf(szBuffer, "Netmap: Can't create Corel Temp Directory : %s", strerror(errno));
      WriteLog(szBuffer);
			sprintf(szBuffer, "%d Can't create the mountpoint directory", knNetmapDirError);
      notify(nCliFd, szBuffer);
			return;
    }

		chown(szMountPoint, nUID, nGID);

		sprintf(szTagFile, "%s%s", szMountPoint, TAGNAME);
		if ((nFd = open(szTagFile, O_RDONLY | O_CREAT | O_TRUNC, S_IRUSR)) < 0)
		{
			WriteLog("Netmap: Cannot create tag file");
			sprintf(szBuffer, "%d Can't create the required tag file in the mount directory", knNetmapTagError);
			notify(nCliFd, szBuffer);
			return;
		}
		else
		{
			if (close(nFd) < 0)
			{
				WriteLog("Netmap: Cannot close the tag file");
			}
		}

		//call DoMount to mount the UNC directory
    BOOL tmp;
    int tmp2;
    CommandGetcred((LPCSTR)szFullUNC, NULL, tmp, 0, 0, 0, tmp2);
    cout<<"Currentuser="<<getenv("CURRENTUSER")<<endl;
    cout<<"Currentworkgroup="<<getenv("CURRENTWORKGROUP")<<endl;
    QListIterator<CCredentialsCacheEntry> it(gCredentialsCache);
    for (it.toFirst(); NULL != it.current(); ++it)
    {
      cout<<"it.current().m_Password="<<endl;
    }

		DoMount((LPCSTR)szFullUNC, szMountPoint, nPID, nUID, nGID, nCliFd);
	}
  else
  {
		//called directly when a match has been found and mount is unnecessary
		sprintf(szBuffer, "%d %s%s", knNetmapSuccess, szMount, (const char*)Path);
		notify(nCliFd, (const char*)szBuffer);
  }

  //garbage collection
  if (gDoGarbageCleanup)
  {
		if (DoGarbageCleanup() == FALSE)
		{
			WriteLog("Garbage cleanup unsuccessful");
		}
		gDoGarbageCleanup = FALSE;  //reset the variable
  }
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Method:	netunmap()
//
//	Purpose:	This function handles all UNC unmount requests for client
//						netunmap calls.  This function removes entries from both the
//						process and the mount tables depending on the particular request.
//
////////////////////////////////////////////////////////////////////////////////

int netunmap(pid_t nPID, uid_t nUID, gid_t nGID, const char* szUNC)
{
	char szNewUNC[1024];
	char szCommandBuf[1024];
	char szMountDir[1024];
	char szBuffer[1024];
	short status;
	QString Server;
	QString Share;
	QString Path;
	QString szUNCforward;
	struct stat statbuf1;
	struct stat statbuf2;
	bool bUMountFail = FALSE;
  QListIterator <CProcessEntry> ProcessTableIter(gProcessTable);
  QListIterator <CMountEntry> MountTableIter(gMountTable);

	//check to see if mapper functionality has been disabled
	if (gMapperDisabled)
	{
		return(knNetunmapError);
	}

	//format the UNC path to only contain forward slashes
	szUNCforward = MakeSlashesForward(szUNC);

  //parse the UNC pathname
  if (!ParseUNCPath((const char*)szUNCforward, Server, Share, Path))
  {
    WriteLog("Netmap: Invalid UNC path");
    return(knNetunmapUNCError);
  }
	sprintf(szNewUNC, "//%s/%s", (const char*)Server, (const char*)Share);

	//Search the mount table for a match. If a match is found then the PID
	//	is removed from the mount table. The count in the process table is decreased
	//  until 0 is reached where the entry is then removed completely.  If the mount
	//	table count decreases to 0 then the UNC directory is unmounted.
  MountTableIter.toFirst();
  for (; MountTableIter.current() != NULL; ++MountTableIter)
  {
		if (MountTableIter.current()->SearchEntry((const char*)szNewUNC, nUID, nGID) == TRUE)
		{
	    //WriteLog("NetUnmap: found a mount table match");
			if (MountTableIter.current()->SearchPID(nPID) == FALSE)
			{
				return(knPermissionError);
			}
	    if (MountTableIter.current()->DecreaseCount() < 1)
	    {
				//WriteLog("Netunmap: The count for this mount has decreased to 0");
				sprintf(szCommandBuf, "%s %s", gSmbUmountLocation, MountTableIter.current()->GetMount());
				//WriteLog(szCommandBuf);

				if (ExecuteCommand(szCommandBuf, nUID, nGID) != 0)
				{
					//sprintf(szCommandBuf, "Returned status: %d", status);
					//WriteLog(szCommandBuf);
					return(knNetunmapSMBError);
				}
				else
				{
					if (stat((const char*)gTempMntDir, &statbuf1) < 0)
					{
				    WriteLog("Netunmap: unable to stat the parent mount directory");
				    return(knNetunmapError);
				  }
				  if (stat(MountTableIter.current()->GetMount(), &statbuf2) < 0)
				  {
				    WriteLog("Netunmap:  unable to stat the mountpoint");
				    return(knNetunmapError);
				  }
				  if (statbuf1.st_dev != statbuf2.st_dev)
				  {
				    WriteLog("Netunmap: unsuccessfully unmounted the UNC path");
						return(knNetunmapError);
				  }
				}

				sprintf(szMountDir, "%s", MountTableIter.current()->GetMount());
				if (!RemoveMountDir(szMountDir))
				{
				  sprintf(szBuffer, "Netunmap: Did not remove %s", szMountDir);
				  WriteLog(szBuffer);
					return(knNetunmapError);
				}

				//remove the entry from the mount table
				gMountTable.remove(MountTableIter.current());
	    }
			else
			{
				MountTableIter.current()->RemovePID(nPID);
			}
		  //search the process table for a match
	    ProcessTableIter.toFirst();
	    for(; ProcessTableIter.current() != NULL; ++ProcessTableIter)
	    {
		  	if (nPID == (ProcessTableIter.current()->GetPID()))
		    {
		    	//WriteLog("Netunmap: Found PID in process table");
		      if (ProcessTableIter.current()->DecreaseCount() == 0)
		      {
			    	//WriteLog("Netunmap: Count for PID in process table is 0");
				    gProcessTable.remove(ProcessTableIter.current());
					}
					else
					{
						//sprintf(szBuffer, "Netunmap: count for PID %d is %d", nPID, ProcessTableIter.current()->CheckCount());
					  //WriteLog(szBuffer);
					}
				}
			}//end for loop
			return(knNetunmapSuccess);
		}//end if
	}//end for loop
	return(knNetunmapError);
}

/////////////////////////////////////////////////////////////////////////////////
//
//	Method:	notify()
//
//	Purpose:	This function handles all mapper related messages that must be
//						returned to the client.
//
/////////////////////////////////////////////////////////////////////////////////

void notify(int szCliFd, const char *szMessage)
{
	char szErrorBuf[1024];
	if(write(szCliFd, szMessage, strlen(szMessage)) < 0)
	{
		sprintf(szErrorBuf, "Can't write to client fd: %s", strerror(errno));
		WriteLog(szErrorBuf);
	}
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Method:	FindMntDir()
//
//	Purpose:	This method prepares the parent mount directory that will be used
//						to contain all the UNC mount points requested by clients.
//
/////////////////////////////////////////////////////////////////////////////////

bool FindMntDir()
{
  LPCSTR szTempMntDir = NULL;
  LPCSTR szPossibleMntDir[] = {"/mnt", "/tmp", "/var"};
  char szErrorBuf[1024];
  char szMountDir[1024];

	//Searches through the default locations checking whether or not the specified
	// directory exists and has the proper permissions
	for (int i = 0; i < sizeof(szPossibleMntDir)/sizeof(LPCSTR); i++)
  {
		if (!access(szPossibleMntDir[i], F_OK | X_OK))
		{
			szTempMntDir = szPossibleMntDir[i];
			break;
		}
  }

	//If no adequate location was found then a check is made for the existence of a
	// CORELTMPDIR.  If it exists then the permissions are changed, if it doesn't then
	// the directory is created and given the appropriate permissions
	if ((szTempMntDir == NULL) && (access(CORELTMPDIR, F_OK) < 0))
  {
		if (mkdir(CORELTMPDIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
		{
			sprintf(szErrorBuf, "Can't create Corel Temp Directory : %s", strerror(errno));
	    WriteLog(szErrorBuf);
	    return(FALSE);
		}

		szTempMntDir = CORELTMPDIR;
	}
	else
	{
		if ((szTempMntDir == NULL) && (!access(CORELTMPDIR, F_OK)))
		{
			if (chmod(CORELTMPDIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
			{
				sprintf(szErrorBuf, "Can't change permissions for Corel Temp Directory : %s", strerror(errno));
				WriteLog(szErrorBuf);
				return(FALSE);
			}
		}
	}

  //A parent directory for the client mount points is then created, if it doesn't exist, under the location
	// determined above
	sprintf(szMountDir, "%s%s", szTempMntDir, CORELMNTDIR);
	if (access(szMountDir, F_OK) < 0)				//does it exist?
	{
  	if (mkdir(szMountDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
  	{
			sprintf(szErrorBuf, "Can't create Corel Mount Directory : %s", strerror(errno));
			WriteLog(szErrorBuf);
			return(FALSE);
  	}
	}
	else				//if it does exist then make sure its permissions are set correctly
	{
			if (chmod(szMountDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0)
			{
				sprintf(szErrorBuf, "Can't change permissions for Corel Mount Directory : %s", strerror(errno));
				WriteLog(szErrorBuf);
				return(FALSE);
			}
	}
	sprintf(gTempMntDir, "%s", szMountDir);	//set the parent mountpoint location
  //WriteLog(gTempMntDir);
  return(TRUE);
}


/////////////////////////////////////////////////////////////////////////////////
//
//	Method:	OnTimer()
//
//	Purpose:	Set the global variable to indicate that a garbage cleanup is
//						pending
//
/////////////////////////////////////////////////////////////////////////////////

void OnTimer(int signo)
{
  if (signo == SIGALRM)
  {
		gDoGarbageCleanup = TRUE;
		alarm(60);
  }
}


////////////////////////////////////////////////////////////////////////////////
//
//	Method:	DoGarbageCleanup()
//
//	Purpose:	Searches through the process table to find processes that are no
//						longer active.  If a process is no longer active then all mounts
//						corresponding to the process are removed from the mount table and
//						unmounted should the mount no longer be required. The directory is
//						is removed provided that its tag file has a modification time equal
//						to or newer than the directory's modification time.
//						To check if the PID is still valid a check is made in the /proc
//						directory for a directory entry corresponding to the PID exists.
//
////////////////////////////////////////////////////////////////////////////////

bool DoGarbageCleanup()
{
  char szBuffer[1024];
  char szPidPath[1024];
	char szMountDir[1024];
	short status;
	pid_t CurPID;
	pid_t tempPID;
	bool UnMount = FALSE;
  QListIterator <CProcessEntry> ProcessTableIter(gProcessTable);
  QListIterator <CMountEntry> MountTableIter(gMountTable);

  //WriteLog("Garbage cleanup: Doing garbage cleanup");

  //search process table
  ProcessTableIter.toFirst();
  while(ProcessTableIter.current() != NULL)
  {
    CurPID = ProcessTableIter.current()->GetPID();
    //sprintf(szBuffer, "the CurPID is: %d", CurPID);
    //WriteLog(szBuffer);

		//forms pathname to /proc/"PID" directory
    sprintf(szPidPath, "%s/%d", PROCPATH, CurPID);
    if (access(szPidPath, F_OK) < 0)
    {
      //WriteLog("Garbage cleanup: Removing the process table entry");

			if (ProcessTableIter.atLast())
			{
				gProcessTable.remove(ProcessTableIter.current());
				++ProcessTableIter;
			}
			else
				gProcessTable.remove(ProcessTableIter.current());

			//search mount table for the PID
      MountTableIter.toFirst();
			while(MountTableIter.current() != NULL)
      {
        if (MountTableIter.current()->SearchPID(CurPID) == TRUE)
        {
          //WriteLog("Garbage cleanup: Found the CurPID");
          MountTableIter.current()->RemovePID(CurPID);
          int n = MountTableIter.current()->DecreaseCount();
          //sprintf(szBuffer, "Garbage cleanup: the count is %d", n);
          //WriteLog(szBuffer);
          //if (MountTableIter.current()->DecreaseCount() <= 0)
          if (n <= 0)
          {
            sprintf(szBuffer, "%s \"%s\"", gSmbUmountLocation, MountTableIter.current()->GetMount());
            //WriteLog(szBuffer);

						//unmount the UNC share from the mount directory
						status = ExecuteCommand(szBuffer, getuid(), 0);
           	//sprintf(szBuffer, "Returned status: %d", status);
						//WriteLog(szBuffer);

						sprintf(szMountDir, "%s", MountTableIter.current()->GetMount());
						if (!RemoveMountDir(szMountDir))
						{
							 sprintf(szBuffer, "GarbageCleanup: Did not remove %s", szMountDir);
							 WriteLog(szBuffer);
						}
            gMountTable.remove(MountTableIter.current());
          }
        }
				else
				{
					++MountTableIter;
				}
      }//end while loop
    }
		else
		{
			++ProcessTableIter;
		}
  }//end while
  return(TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//	Method:	MountPointCleanup()
//
//	Purpose:	This method is used on netserv startup to remove any client
//						mount mount directories that may have been left over during
//						a previous execution.
//
////////////////////////////////////////////////////////////////////////////

bool MountPointCleanup()
{
	char szMntDir[1024];
	char szTempMntDir[1024];
	char szBuffer[1024];
  struct dirent* dirp;
  DIR* dp;
  const char* szPossibleMntDir[] = {"/mnt", "/tmp", "/var", CORELTMPDIR};

  for (int i = 0; i < sizeof(szPossibleMntDir)/sizeof(const char*); i++)
  {
    sprintf(szMntDir, "%s%s", szPossibleMntDir[i], CORELMNTDIR);
    struct stat st1, st2;

    if (!stat(szMntDir, &st1))
    {
      if ((dp = opendir(szMntDir)) == NULL)
      {
				WriteLog("Mountpoint cleanup: Unable to open the directory");
        return(FALSE);
      }
      while ((dirp = readdir(dp)) != NULL)
      {
        if ((strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..") == 0))
          continue;

				if (strncmp((const char*)dirp->d_name, UNCMOUNTNAME, (size_t)sizeof(UNCMOUNTNAME) - 1) == 0)
        {
          sprintf(szTempMntDir, "%s/%s", szMntDir, dirp->d_name);
					//WriteLog(szTempMntDir);

          if (!stat(szTempMntDir, &st2))
          {
            if (st1.st_dev != st2.st_dev) // Still have something mounted here...
            {
              char szBuffer[1024];
              sprintf(szBuffer, "%s \"%s\"", gSmbUmountLocation, szTempMntDir);
              //WriteLog(szBuffer);
              ExecuteCommand(szBuffer, getuid(), getgid()); // unmount
            }

            if (!RemoveMountDir(szTempMntDir))
  					{
  						sprintf(szBuffer, "MountPointCleanup: Did not remove %s", szTempMntDir);
  						WriteLog(szBuffer);
  					}
          }
        }
      }//end while

      if (closedir(dp))
			{
				WriteLog("Unable to close the mount directory");
        return(FALSE);
			}
    }
  }
	return(TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//	Method:	RemoveMountDir()
//
//	Purpose:	This function will remove a specified mount point directory
//						by comparing the modification times of the directory and the
//						tag file within the directory created when the directory
//						was created.
//
////////////////////////////////////////////////////////////////////////////

bool RemoveMountDir(LPCSTR szDirectory)
{
	char szTagFile[1024];
	struct stat statbuf1;
	struct stat statbuf2;

	if (lstat(szDirectory, &statbuf1) < 0)
  {
    WriteLog("RemoveMountDir: unable to lstat the mountpoint directory");
	  return(FALSE);
  }

  if (S_ISDIR(statbuf1.st_mode))
  {
    sprintf(szTagFile, "%s%s", szDirectory, TAGNAME);
    if (lstat(szTagFile, &statbuf2) < 0)
    {
      WriteLog("RemoveMountDir: unable to lstat the tag file");
      return(FALSE);
    }

		//compare their modification times and remove both the mount point directory
    //  and the tag file if the tag file modification time is greater than or
	  //  equal to the mount point directory modication time.  If this comparison
	  //  fails then only the tag file is removed

		if (statbuf2.st_mtime >= statbuf1.st_mtime)
    {
      if (remove(szTagFile) < 0)
      {
        WriteLog("RemoveMntDir: unable to remove the tag file, case 1");
        return(FALSE);
      }
      else
      {
        if (remove(szDirectory) < 0)
        {
          WriteLog("RemoveMntDir: unable to remove the mountpoint directory");
          return(FALSE);
        }
      }
	  }
	  else
	  {
	    if (remove(szTagFile) < 0)
	    {
	      WriteLog("RemoveMntDir: unable to remove the tag file, case 2");
	      return(FALSE);
	    }
	  }
	}
	return(TRUE);
}



////////////////////////////////////////////////////////////////////////////
//
//	Method:	main()
//
//	Purpose:
//
////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	gArgc = argc;
	gArgv = argv;

	srand(time(NULL));

	LPCSTR DefaultUser = getenv("DEFAULTUSER");

	if (NULL == getenv("DISPLAY"))
	{
		setenv("DISPLAY", ":0.0", 0);
	}

	if (NULL == DefaultUser)
	{
		WriteLog("DEFAULTUSER is not defined, aborting");
		exit(-1);
	}

	if (!LocateSmbMount())
		exit(-1);

	//WriteLog("Server up");
	//setenv("LSEXIT", "\"ls;exit\"", 1);
	setenv("SMBEXIT", ";exit", 1);


	uid_t uid;
	gid_t gid;

	if (getenv("UID") == NULL)
	{
		struct passwd *pw = getpwnam(DefaultUser);

		if (NULL != pw)
		{
			char buf[20];
			sprintf(buf, "%d", pw->pw_uid);
			setenv("UID", buf, 0);

			uid = pw->pw_uid;
		}
		else
		{
			WriteLog("Unable to get UID");
			exit(-1);
		}
	}
	else
		sscanf(getenv("UID"), "%u", &uid);

	if (getenv("HOME") == NULL)
	{
		struct passwd *pw = getpwuid(uid);
		setenv("HOME", pw != NULL ? pw->pw_dir : "", 1);
	}

	if (getenv("GID") == NULL)
	{
		struct passwd *pw = getpwuid(uid);

		if (NULL != pw)
		{
			char buf[20];
			sprintf(buf, "%d", pw->pw_gid);
			setenv("GID", buf, 0);

			gid = pw->pw_gid;
		}
		else
		{
			WriteLog("Unable to get GID");
			exit(-1);
		}
	}
	else
		sscanf(getenv("GID"), "%u", &gid);

	// Launch automount.
	// All job is done by the CopyAgent being invoked.

	if (!fork())
	{
		ExecuteCommand("/usr/X11R6/bin/CopyAgent mount", uid, gid);

		QString msg;
		msg.sprintf("waitpid %d", getpid());

		int fd = GetServerOpenHandle(msg);

		if (fd >= 0)
			close(fd);

		exit(0);

	}

	//set to ensure that all items in the lists are deleted when removed
	gProcessTable.setAutoDelete(TRUE);
	gMountTable.setAutoDelete(TRUE);

	if (!MountPointCleanup())
	{
		WriteLog("Can't cleanup mountpoint");
	}

	if (!FindMntDir())
	{
	  WriteLog("Can't find a mount directory");
	  gMapperDisabled = TRUE;
	}

	//signals garbage collection to execute on next netmap request
	if (signal(SIGALRM, OnTimer) == SIG_ERR)
		WriteLog("Can't catch SIGALRM");
	else
	  alarm(GARBAGETIME);

	loop();

}

////////////////////////////////////////////////////////////////////////////



