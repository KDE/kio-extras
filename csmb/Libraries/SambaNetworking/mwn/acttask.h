/* Name: acttask.h
 
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

#ifndef __INC_ACTTASK_H__
#define __INC_ACTTASK_H__

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "qlist.h"
#include "qtimer.h"
#include "sys/types.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////////

typedef void (*LPFNACTTASKCALLBACK)(int, void *);

class CActiveTask
{
public:
	CActiveTask(pid_t pid, LPFNACTTASKCALLBACK Callback, void *Param, int Lifetime = 0)
	{
		m_Pid = pid;
		m_Callback = Callback;
		m_pParam = Param;

		if (Lifetime > 0)
		{
			m_StopTime = time(NULL) + Lifetime;
		}
		else
			m_StopTime = 0; // no time limit
	}

	pid_t m_Pid;
	
	time_t m_StopTime; // no time limit if = 0

	LPFNACTTASKCALLBACK m_Callback;
	void *m_pParam;
};
	
class CActiveTaskList : public QObject, public QList<CActiveTask>
{
	Q_OBJECT

public:
	CActiveTaskList();
	~CActiveTaskList();
	
	void PrepareForExit();

	void StopAll();
	void KillEntireKind(LPFNACTTASKCALLBACK Kind);

private slots:
	void Update();

private:
	QTimer *m_pTimer;
};

////////////////////////////////////////////////////////////////////////////

extern CActiveTaskList gTasks;

////////////////////////////////////////////////////////////////////////////

#endif /* __INC_ACTTASK_H__ */
