/* Name: acttask.cpp

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


#include "acttask.h"
#include <sys/wait.h> // for waitpid()
#include <signal.h>
#include <time.h>

CActiveTaskList gTasks;

////////////////////////////////////////////////////////////////////////////

CActiveTaskList::CActiveTaskList()
{
	m_pTimer = new QTimer(this);
	setAutoDelete(TRUE);
	connect(m_pTimer, SIGNAL(timeout()), SLOT(Update()));
	
	m_pTimer->start(150);
}

////////////////////////////////////////////////////////////////////////////

CActiveTaskList::~CActiveTaskList()
{
	if (NULL != m_pTimer)
	{
		//delete m_pTimer; Never delete a timer, Qt2 crashes if application doesn't exist any more.
	}
}

////////////////////////////////////////////////////////////////////////////

void CActiveTaskList::PrepareForExit()
{
	if (NULL != m_pTimer)
	{
		delete m_pTimer;
		m_pTimer = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////

void CActiveTaskList::Update()
{
	QListIterator<CActiveTask> it(*this);
	
	for (it.toLast(); it.current() != NULL;)
	{
		CActiveTask *pTask = it.current();
		--it;
		
		int status;

		if (waitpid(pTask->m_Pid, &status, WNOHANG) > 0)
		{
			if (NULL != pTask->m_Callback)
			{
				(*pTask->m_Callback)(status, pTask->m_pParam);
			}
			remove(pTask);
		}
		else
		{
			if (pTask->m_StopTime > 0 && time(NULL) >= pTask->m_StopTime)
			{
				kill(pTask->m_Pid, SIGKILL);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CActiveTaskList::KillEntireKind(LPFNACTTASKCALLBACK Kind)
{
	QListIterator<CActiveTask> it(*this);
	
	for (it.toLast(); it.current() != NULL;)
	{
		CActiveTask *pTask = it.current();
		--it;

		if (pTask->m_Callback == Kind)
		{
			pTask->m_Callback = NULL;
			kill(pTask->m_Pid, SIGKILL);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CActiveTaskList::StopAll()
{
	//printf("Post-mortem\n");

	QListIterator<CActiveTask> it(*this);
	
	for (it.toLast(); it.current() != NULL;--it)
	{
		it.current()->m_StopTime = time(NULL)-1;
	}

	// Ensure all tasks are waited for or killed...

	Update();
}

////////////////////////////////////////////////////////////////////////////

