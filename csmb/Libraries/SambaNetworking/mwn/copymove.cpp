/* Name: copymove.cpp

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
#include "copymove.h"
#include <errno.h>
#include <sys/wait.h> // for waitpid()
#include <unistd.h> // for fork()
#include "acttask.h"
#include "qapplication.h"
#include "kapp.h"
#include "qmessagebox.h"

#if (QT_VERSION >= 200)
#include "kglobal.h"
#include "kstddirs.h"
#endif

void OnFileOperationCompleted(int /*status*/, void *pParam)
{
	QStrList *pList = (QStrList *)pParam;
  for (LPCSTR x=pList->first(); x; x=pList->next())
  {
		QString z(x);
    URLDecodeSmart(z);
    
    if (IsUNCPath(z))
    {
      gTreeExpansionNotifier.DoRefreshRequested(z);
    }
  }

  delete pList;
}

void StartCopyMove(QStrList& Source, LPCSTR Destination, BOOL bMove /* = FALSE */, BOOL bOverwrite /* = FALSE */, BOOL bDestinationFinalized /* = FALSE */)
{
	pid_t pid;
	LPCSTR x;
  
  //printf("StartCopyMove (%s)\n", Destination);
  
  QStrList *pList = new QStrList;
  
  pList->append(Destination);
  
	// Sanity check...
	
	for (x= Source.first(); x != NULL; x = Source.next())
	{
		int nResult;

		//printf("Parent URL = %s, Destination = %s\n", (LPCSTR)GetParentURL(x), (LPCSTR)Destination);
		
		if (GetParentURL(x) == Destination && bMove)
			nResult = 1;
		else
		{
			nResult = IsInSubtree(x, Destination);
		}

		if (nResult == 0)
			continue;

		QString Parent, FileName;

		if (strlen(x) > 7 && !strncmp(x, "file://", 7))
			x += 7;
		
		QString a;
		a.sprintf(LoadString(nResult == 1 ? knDESTINATION_FOLDER_SAME : knDESTINATION_FOLDER_SUBTREE), (LPCSTR)x);

		QMessageBox::critical(qApp->mainWidget(), LoadString(bMove ? knSTR_FILE_MOVE : knSTR_FILE_COPY), a);

		Source.remove();
	}

	if (Source.count() == 0)
		return;

	if ((pid = fork()) < 0)
	{
		printf("Unable to fork()\n");
		return;
	}
	else if (pid == 0)
	{
		LPCSTR *Param = new LPCSTR[Source.count() + 4];

		Param[0] = "CopyAgent";
		Param[1]= bMove ? "move" : "copy";
		Param[2] = Destination;
		
		//printf("Calling CopyAgent: %s %s ", Param[1], Param[2]); 
		
		int i=3;

		for (LPCSTR x=Source.first(); x; x=Source.next())
		{
			Param[i++] = x;
		}

		if (bOverwrite)
		{
			Param[i++] = "-o";
		}
		
		if (bDestinationFinalized)
		{
			Param[i++] = "-r";
		}
		
    Param[i] = NULL;
		
#if (QT_VERSION >= 200)
    QString CopyAgentLocation = KGlobal::dirs()->findResource("exe", "CopyAgent");
#else    
    QString CopyAgentLocation = KApplication::kde_bindir();
		if (CopyAgentLocation.right(1) != "/")
			CopyAgentLocation += "/";

		CopyAgentLocation += "CopyAgent";
#endif
		
		
		execv(CopyAgentLocation, (char *const *)Param);

		_exit(127);     /* execl error */
	}
	else	/* parent */
	{
		if (bMove)
    {
		  for (LPCSTR x=Source.first(); x; x=Source.next())
          pList->append(x);
    }
    
    // Simply add the pid to our global task list.
    
    gTasks.append(new CActiveTask(pid,OnFileOperationCompleted, (void *)pList));
	}
}

void StartDelete(QStrList& List, BOOL bMoveToTrash, BOOL bSilentDelete /*= FALSE */)
{
	pid_t pid;
	LPCSTR x;
  
	if (List.count() == 0)
		return;

	if ((pid = fork()) < 0)
	{
		printf("Unable to fork()\n");
		return;
	}
	else if (pid == 0)
	{
		LPCSTR *Param = new LPCSTR[List.count() + 4];
		QString s;

		Param[0] = "CopyAgent";
		
		QString TrashPath;

		if (bMoveToTrash)
		{
			TrashPath = GetHomeDir();
			
			if (TrashPath.right(1) != "/")
				TrashPath += "/";

			TrashPath += "Desktop/Trash";
			
			// Sanity check...

			for (x= List.first(); x != NULL; x = List.next())
			{
        if (strlen(x) > 7 && !strncmp(x, "file://", 7))
          x += 7;
				
        if (IsInSubtree(x, TrashPath))
				{
					bMoveToTrash = FALSE;
          break;
				}
			}
		}
		
		if (bMoveToTrash)
		{
			Param[1] = "move";
			Param[2] = (LPCSTR)TrashPath;
		}
		else
		{
			Param[1]= "del";
			Param[2] = ".";
		}
		
		//printf("Calling CopyAgent: %s %s %s ", Param[0],Param[1],Param[2]); 
		
		int i=3;

		for (LPCSTR x=List.first(); x; x=List.next())
		{
			Param[i++] = x;
		}

		if (bSilentDelete)
    {
      Param[i++] = "-s";
    }
    
    Param[i] = NULL;
		
#if (QT_VERSION >= 200)
    QString CopyAgentLocation = KGlobal::dirs()->findResource("exe", "CopyAgent");
#else    
    QString CopyAgentLocation = KApplication::kde_bindir();
		if (CopyAgentLocation.right(1) != "/")
			CopyAgentLocation += "/";

		CopyAgentLocation += "CopyAgent";
#endif
		
		execv(CopyAgentLocation, (char *const *)Param);

		_exit(127);     /* execl error */
	}
	else	/* parent */
	{
    QStrList *pList = new QStrList;
    
    for (LPCSTR x=List.first(); x; x=List.next())
    {
      QString Value(x);
      URLDecodeSmart(Value);

      Value = GetParentURL(Value);
      LPCSTR y;

      for (y=pList->first(); NULL != y; y = pList->next())
      {
        if (Value == y)
        {
          break;
        }
      }
      
      if (NULL == y)
        pList->append(Value);
    }

    // Simply add the pid to our global task list.
		gTasks.append(new CActiveTask(pid,OnFileOperationCompleted,(void *)pList));
	}
}

