/* Name: expcommon.cpp

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

#include "common.h"
#include "expcommon.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "kurl.h"
#include "unistd.h"
#include "qapplication.h"
#include <sys/wait.h> // for waitpid
#include "kapp.h"
#include <stdlib.h> // for mktemp
#include "treeitem.h"
#include "browsercache.h"
#include <string.h> // for strerror
#include <errno.h>
#include "explres.h"
#include <qmessagebox.h>
#include <X11/Xlib.h>
#include "filesystem.h"
#include <qobjcoll.h>
#include "waitkfm.h"

#if (QT_VERSION >= 200)
#include "kstddirs.h"
#include "kglobal.h"
#include "kdebug.h"
#endif

/* The following flags are used as return values for CopyAgent. */

#define F_INTRANET	W_EXITCODE(1, 0)

#define F_SUCCESS		W_EXITCODE(0, 0)
#define F_FAILURE		W_EXITCODE(2, 0)
#define F_INFO			W_EXITCODE(4, 0)

////////////////////////////////////////////////////////////////////////////

CDocumentRequest::CDocumentRequest(KHTMLView *pView, LPFNACTTASKCALLBACK Callback, LPCSTR Url, LPCSTR PostData) :
	m_URL(Url), m_PostData(PostData)
{
	m_pView = pView;
	m_Callback = Callback;

	char LocalName[128];
	static int nTempCount=0;

	sprintf(LocalName, "/tmp/CorExp.%d.XXXXXX", nTempCount++);
	mktemp(LocalName);
	m_LocalName = LocalName;
	m_bTemp = true;
}

////////////////////////////////////////////////////////////////////////////

void DownloadHTTP(CDocumentRequest *pRequest, BOOL bSilent)
{
	if (strncmp((LPCSTR)pRequest->m_URL
#ifdef QT_20
  .latin1()
#endif
  , "file:", 5) == 0)
	{
		if (pRequest->m_bTemp)
		{
			/* Download file:/... */

			/* remove 'file:' prefix and #anchors */
			const char *p0 = (const char *) pRequest->m_URL
#ifdef QT_20
  .latin1()
#endif
        + 5;
			const char *p = strrchr(p0, '#');

			pRequest->m_LocalName = p0;
			if (p != NULL)
				pRequest->m_LocalName.truncate((int) (p - p0));

			pRequest->m_bTemp = false;
			pRequest->m_Callback(F_SUCCESS | F_INTRANET, pRequest);
			return;
		}
		else
		{
			/* Save file:/... to disk (e.g., Save Image). */

			if (!CopyFile((LPCSTR) pRequest->m_URL
#ifdef QT_20
  .latin1()
#endif
       + 5, pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
       ))
			{
				QString message;
				message.sprintf(LoadString(knCANNOT_OPEN_FILE_X), (LPCSTR) pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
        , strerror(errno));
				QMessageBox::warning(qApp->mainWidget(), LoadString(knAPP_TITLE), message);
			}
			pRequest->m_Callback(F_SUCCESS, pRequest);
			return;
		}
	}

	QString TruncatedURL = pRequest->m_URL;
	int i = TruncatedURL.find('#');

	if (i > 0)
  {
#if (QT_VERSION < 200)
    TruncatedURL.detach();
#endif
  	TruncatedURL.truncate(i);
  }

	BOOL bIntranet;

	QString CachedFile = CBrowserCache::Instance()->Lookup((LPCSTR)TruncatedURL
#ifdef QT_20
  .latin1()
#endif
  , &bIntranet);
	if (!CachedFile.isNull())
	{
		if (pRequest->m_bTemp)
		{
			pRequest->m_LocalName = CachedFile;
			pRequest->m_bTemp = false;
		}
		else
		{
			if (!CopyFile((LPCSTR)CachedFile
#ifdef QT_20
  .latin1()
#endif
      , pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
      ))
			{
				QString message;
				message.sprintf(LoadString(knCANNOT_OPEN_FILE_X), (LPCSTR) pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
        , strerror(errno));
				QMessageBox::warning(qApp->mainWidget(), LoadString(knAPP_TITLE), message);
			}
		}
		pRequest->m_Callback(F_SUCCESS | (bIntranet ? F_INTRANET : 0), pRequest);
		return;
	}

	pid_t pid;

	if ((pid=fork()) < 0)
		return; // unable to fork()

	if (pid == 0)
	{
		const char *szSilentSwitch = "";

		if (bSilent)
			szSilentSwitch = "-s";

#if (QT_VERSION >= 200)
    QString CopyAgentLocation = KGlobal::dirs()->findResource("exe", "CopyAgent");
#else
    QString CopyAgentLocation = KApplication::kde_bindir();
#endif

		if (CopyAgentLocation.right(1) != "/")
			CopyAgentLocation += "/";

		if (pRequest->m_PostData.isNull())
			execl((LPCSTR)(CopyAgentLocation + "CopyAgent")
#ifdef QT_20
  .latin1()
#endif
      , "CopyAgent", szSilentSwitch, "copy", (LPCSTR)pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)pRequest->m_URL
#ifdef QT_20
  .latin1()
#endif
      ,  NULL);
		else
		{
			FILE *f = fopen(pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
      , "w");

			if (NULL != f)
			{
				fwrite((const void *)pRequest->m_PostData
#ifdef QT_20
  .latin1()
#endif
        , 1, pRequest->m_PostData.length(), f);
				fclose(f);
			}

			execl((LPCSTR)(CopyAgentLocation+"CopyAgent")
#ifdef QT_20
  .latin1()
#endif
      , "CopyAgent", szSilentSwitch, "copy", (LPCSTR)pRequest->m_LocalName
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)pRequest->m_URL
#ifdef QT_20
  .latin1()
#endif
      , "-d", NULL);
		}
		exit(127); // execl error
	}
	else
	{
		gTasks.append(new CActiveTask(pid, pRequest->m_Callback, (void*)pRequest));
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL DecodeURLList(QDragEnterEvent *e, QStrList& l)
{
	QByteArray payload = e->data("url/url");

	if (!payload.size())
		payload = e->data("text/uri-list");

	if (payload.size())
	{
		l.clear();
		l.setAutoDelete(TRUE);
		uint c=0;

		char* d = payload.data();

		while (c < payload.size())
		{
			uint f = c;

			while (c < payload.size() && d[c])
				c++;

			if (c < payload.size())
			{
				l.append(d+f);
				c++;
			}
			else
			{
#if (QT_VERSION < 200)
        QString s(d+f,c-f+1);
#else
        QString s = QString(d+f).left(c-f);
#endif
				l.append((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
        );
			}
		}

		return TRUE;
	}
	else
  {
    payload = e->data("text/plain");
    int len = payload.size();

    if (len > 5 && !strnicmp(payload.data(), "file:/", 6))
    {
#if (QT_VERSION < 200)
      QString s(len+1);
#else
      QCString s(len+1);
#endif
      memcpy(s.data(), payload.data(), len);
      s[len] = '\0';
      l.append(s);
      return TRUE;
    }
  }

  return FALSE;
}

////////////////////////////////////////////////////////////////////////////

CFileSystemInfo *FindMountedFileSystem(LPCSTR Path)
{
  int i;

  for (i=0; i < gFileSystemList.count(); i++)
  {
    if (gFileSystemList[i].m_MountedOn == Path)
    {
      return &gFileSystemList[i];
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////

BOOL CanDeleteLocalPath(QString Path)
{
	kdDebug(1000)<<"Path in CanDeleteLocalPath "<<Path<<endl;
  if (IsReadOnlyFileSystemPath(Path) || FindMountedFileSystem(Path) || Path == gDefaultHomeDirectory)
  {
    return FALSE;
  }

  if (!getuid() || !getgid() || !geteuid() || !getegid())
		return TRUE;

  struct stat st;

  if (lstat(Path, &st) < 0)
    return FALSE;

  if (S_ISLNK(st.st_mode))
    return TRUE;

  if (access(Path, W_OK))
    return FALSE;

	if (Path.right(1) != "/")
		Path += "/";

	Path += "..";

	int retcode = !access(Path, 0) ? !access(Path, W_OK) : TRUE;
  return retcode;
}

////////////////////////////////////////////////////////////////////////////

static BOOL CanDeleteAllList(const QStrList &list)
{
	QStrListIterator it(list);

	for (it.toFirst(); NULL != it.current(); ++it)
	{
		QString Path(it.current());

		URLDecodeSmart(Path);

		if (Path[0] == '/' && !CanDeleteLocalPath(Path))
		{
			return FALSE; // there are non-deletable items here...
		}
	}

	return TRUE;
}

BOOL ItemAcceptsThisDrop(CNetworkTreeItem *pItem, QStrList& DraggedURLList, BOOL bCopyOnly)
{
	if (DraggedURLList.count() == 0)
		return FALSE;

	QString ItemPath(pItem->FullName(FALSE));

	if (IsTrashFolder((LPCSTR)ItemPath
#ifdef QT_20
  .latin1()
#endif
  ))
	{
		if (!IsMyTrashFolder((LPCSTR)ItemPath
#ifdef QT_20
  .latin1()
#endif
    ))
		{
			return FALSE; // don't allow to trash to somebody else's trash
		}

		return CanDeleteAllList(DraggedURLList);
	}

	QString URL;
	MakeURL((LPCSTR)ItemPath
#ifdef QT_20
  .latin1()
#endif
  , NULL, URL);
	LPCSTR x;

	for (x = DraggedURLList.first(); x != NULL; x = DraggedURLList.next())
	{
		if (!strcmp(x,(LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    ) || GetParentURL(x) == URL)
		{
			return FALSE;
		}

    if ((IsFTPUrl((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    ) || IsPrinterUrl((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
))
				&& IsFTPUrl(x)) // don't support FTP-to-FTP or FTP-to-printer DND's yet :)
      return FALSE;
	}

	return bCopyOnly ? CanDeleteAllList(DraggedURLList) : TRUE;
}

////////////////////////////////////////////////////////////////////////////

int GetMouseState(QWidget *w)
{
  unsigned int keys_buttons;
  int root_x, root_y, pos_x, pos_y;
  Window root, child;

  int ret = Qt::NoButton;

  XQueryPointer(w->x11Display(), w->winId(), &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);

  if (keys_buttons & Button1Mask)
  {
    ret |= Qt::LeftButton;
  }

  if (keys_buttons & Button2Mask)
  {
    ret |= Qt::MidButton;
  }

  if (keys_buttons & Button3Mask)
  {
    ret |= Qt::RightButton;
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////

void RunMimeBinding(BOOL bNeedOpenWithDialog, QString &Name, LPCSTR ApplicationName)
{
	CWaitKFM *w = new CWaitKFM;

	w->exec(bNeedOpenWithDialog, Name, ApplicationName);
}

////////////////////////////////////////////////////////////////////////////

void LaunchProgram(char *const *Param)
{
  pid_t pid;

  if ((pid = fork()) < 0)
	{
		printf("Unable to fork()\n");
		return;
	}
	else if (pid == 0)
	{
#if (QT_VERSION >= 200)
    QString ProgramLocation = KGlobal::dirs()->findResource("exe", Param[0]);
#else
    QString ProgramLocation = KApplication::kde_bindir();
		
		if (ProgramLocation.right(1) != "/")
			ProgramLocation += "/";

		ProgramLocation += Param[0];
#endif

		execv((LPCSTR)(ProgramLocation)
#ifdef QT_20
  .latin1()
#endif
    , Param);

		exit(127);     /* execl error */
	}
	else	/* parent */
	{
	}
}

////////////////////////////////////////////////////////////////////////////

void LaunchURL(LPCSTR URL)
{
  QString Width;

  int n = (gnScreenWidth * 2) / 3;

  Width.sprintf("-w%d", n);

  QString Height;

  Height.sprintf("-h%d", (gnScreenHeight * 2) / 3);

  LPCSTR Param[6];

  Param[0] = "CorelExplorer";
  Param[1]= URL;
  Param[2] = "-t";
  Param[3] = (LPCSTR)Width
#ifdef QT_20
  .latin1()
#endif
  ;
  Param[4] = (LPCSTR)Height
#ifdef QT_20
  .latin1()
#endif
  ;
  Param[5] = NULL;
  LaunchProgram((char *const *)Param);
}

////////////////////////////////////////////////////////////////////////////

