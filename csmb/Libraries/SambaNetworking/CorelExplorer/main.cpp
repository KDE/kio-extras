/* Name: main.cpp

   Description: This file is a part of the Corel File Manager application.

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

#include "qapplication.h"
#include "kapp.h"
#include "mainfrm.h"
#include "unistd.h"
#include <stdlib.h>
#include "locale.h"
#ifndef QT_20
#include "corelclipboard.h"
#else
#include "qclipboard.h"
#include "qwindowsstyle.h"
#include <kglobal.h>
#include <klocale.h>
#endif
#include "X11/Xlib.h"
#include "shell.h"
#include "automount.h"
#include <pwd.h>

QApplication *gpApp;
#include <qdragobject.h>

BOOL gbNeedSaveConfigSettings = TRUE;
BOOL gbShowTree = -1;
BOOL gbUseBigIcons = -1;
BOOL gbShowAddressBar = -1;
BOOL gbShowToolBar = -1;
BOOL gbShowStatusBar = -1;
BOOL gbShowMyComputer = -1; // special value "-1" means "not initialized"

int gnDesiredAppWidth = -1;
int gnDesiredAppHeight = -1;
int gnDesiredAppX = -1;
int gnDesiredAppY = -1;
int gnScreenWidth;
int gnScreenHeight;
QString gsStartAddress;
QString gsExtraCaption;

extern QObject *qt_clipboard;
void SetExtraCaption();

BOOL VerifyNetserv()
{
	int fd = GetServerOpenHandle("getenv DEFAULTUSER");

  //printf("fd = %d\n", fd);

	if (-4 == fd)
	{
		pid_t pid;

		if ((pid = fork()) < 0)
		{
			printf("Unable to fork()\n");
			return FALSE;
		}
		else if (pid == 0)
		{
			/* child */
			//close(1); // close stdout
			close(2); // close stderr

			LPCSTR s = getenv("USER");

			if (NULL == s)
				s = getenv("USERNAME");

			if (NULL == s)
				s = "";

			setenv("DEFAULTUSER", s, TRUE);
			setenv("DEFAULTWORKGROUP", "%notset%", TRUE);

			char buf[30];

			sprintf(buf,"%u", getuid());
			setenv("UID", buf, TRUE);

			sprintf(buf,"%u", getgid());
			setenv("GID", buf, TRUE);

#ifdef QT_20
      QString Path("/usr/X11R6/bin");
#else
      QString Path = KApplication::kde_bindir();
#endif

			if (Path.right(1) != "/")
				Path += "/";

      //printf("Lanching %snetserv\n", (LPCSTR)Path);
			execl((LPCSTR)(Path + "netserv")
#ifdef QT_20
  .latin1()
#endif
      , "netserv", NULL);
      _exit(127);     /* execl error */
    }
		else
			sleep(2);
	}
	else
		close(fd);

	return TRUE;
}

int main(int argc, char **argv)
{
	KApplication a(argc, argv, "CorelExplorer");

#ifndef QT_20
	a.getLocale()->insertCatalogue("mwn");
	a.getLocale()->insertCatalogue("propdlg");
#else
	KGlobal::locale()->insertCatalogue("mwn");
	KGlobal::locale()->insertCatalogue("propdlg");
#endif

	for (int i=1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-restore"))
    {
      i++;
      continue;
    }

    if (!strcmp(argv[i], "-a"))
    {
      gbNeedSaveConfigSettings = FALSE;
      gbShowAddressBar = FALSE;
    }
		else
			if (!strcmp(argv[i], "+a"))
      {
        gbNeedSaveConfigSettings = FALSE;
        gbShowAddressBar = TRUE;
      }
			else
				if (!strcmp(argv[i], "-b"))
        {
          gbNeedSaveConfigSettings = FALSE;
          gbUseBigIcons = FALSE;
        }
				else
					if (!strcmp(argv[i], "+b"))
          {
            gbNeedSaveConfigSettings = FALSE;
            gbUseBigIcons = TRUE;
          }
					else
						if (!strcmp(argv[i], "-t"))
            {
              gbNeedSaveConfigSettings = FALSE;
              gbShowTree = FALSE;
            }
						else
							if (!strcmp(argv[i], "+t"))
              {
                gbNeedSaveConfigSettings = FALSE;
                gbShowTree = TRUE;
              }
							else
								if (strlen(argv[i]) > 2 && !strncmp(argv[i], "-w", 2))
                {
                  gbNeedSaveConfigSettings = FALSE;
									gnDesiredAppWidth = atoi(argv[i] + 2);
                }
								else
									if (strlen(argv[i]) > 2 && !strncmp(argv[i], "-h", 2))
                  {
                    gbNeedSaveConfigSettings = FALSE;
										gnDesiredAppHeight = atoi(argv[i] + 2);
                  }
									else
										if (strlen(argv[i]) > 2 && !strncmp(argv[i], "-x", 2))
                    {
                      gbNeedSaveConfigSettings = FALSE;
											gnDesiredAppX = atoi(argv[i] + 2);
                    }
										else
											if (strlen(argv[i]) > 2 && !strncmp(argv[i], "-y", 2))
                      {
                        gbNeedSaveConfigSettings = FALSE;
												gnDesiredAppY = atoi(argv[i] + 2);
                      }
											else
												if (argv[i][0] != '-')
												{
													gsStartAddress = argv[i];
													URLDecodeSmart(gsStartAddress);
												}
	}

  gpApp = &a;

#ifndef QT_20
  a.enableSessionManagement(FALSE);
#endif

	// Read screen size
	Display *dpy = QApplication::desktop()->x11Display();
	gnScreenWidth = DisplayWidth(dpy,DefaultScreen(dpy));
	gnScreenHeight = DisplayHeight(dpy,DefaultScreen(dpy));

#ifndef QT_20
  extern int gUseCorelClipboard;
	gUseCorelClipboard = 1;

	CCorelClipboard *cb = GetCorelClipboard();
	
	/* This clears the clipboard and removes unnecessary startup
		 delay */
	
	if (NULL != cb)
		cb->setText("");
#else
	
	QApplication::clipboard()->setText("");

#endif

	/* ----------------------------------------------------- */

	setlocale(LC_ALL, "");
	VerifyNetserv();

	void ReadConfiguration();
	ReadConfiguration();
  ReadAutoMountList(gAutoMountList);
	SetExtraCaption();

#ifdef QT_20
	a.setStyle(new QWindowsStyle);
#else
	a.setStyle(WindowsStyle);
#endif
	CMainFrame *pMainFrame = new CMainFrame(&a);

	QObject::connect(qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));

	int nRet = a.exec();
    gbStopping = true;

#ifdef QT_20
	qt_clipboard = NULL;	// prevents segmentation fault
#endif
	
	delete pMainFrame;
	return nRet;
}

static bool GetProcessInfo(pid_t pid, pid_t& ppid, uid_t& uid, gid_t& gid, char *Name)
{
	char FileName[1024];

	sprintf(FileName, "/proc/%u/status", pid);
	FILE *f = fopen(FileName, "r");

	if (NULL == f)
		return false;

	while (!feof(f))
	{
		char buf[1024];
		fgets(buf, sizeof(buf), f);
		buf[strlen(buf)-1] = '\0';

		if (!strncmp(buf, "Name:", 5))
		{
			strcpy(Name,&buf[6]);
			continue;
		}

		if (!strncmp(buf, "PPid:", 5))
		{
			sscanf(&buf[5],"%u", &ppid);
			continue;
		}
		if (!strncmp(buf, "Uid:", 4))
		{
			sscanf(&buf[5],"%u", &uid);
			continue;
		}
		if (!strncmp(buf, "Gid:", 4))
		{
			sscanf(&buf[5],"%u", &gid);
			continue;
		}
	}
	fclose(f);
	return true;
}

void SetExtraCaption()
{
	pid_t pid = getpid();
	pid_t ppid;
	uid_t uid;
	gid_t gid;
	char Name[1024];

	gsExtraCaption = "";

	while (GetProcessInfo(pid, ppid, uid, gid, Name))
	{
		if (!strcmp(Name, "kwm"))
		{
			if (uid != getuid())
			{
				struct passwd *pw;
				pw = getpwuid(geteuid());

				if (NULL != pw)
					gsExtraCaption = QString(" [") + pw->pw_name + QString("] ");
			}
			else
				return;
		}
		pid = ppid;
	}
}

