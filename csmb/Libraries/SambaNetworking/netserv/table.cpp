/* Name: table.cpp

   Description: This file is a part of the netserv application.

   Author:	Chris Ellison

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

/////////////////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "qstring.h"
#include "qlist.h"
#include "table.h"

////////////////////////////////////////////////////////////////////////////////
//
//	CProcessEntry
//
////////////////////////////////////////////////////////////////////////////////
CProcessEntry::CProcessEntry(pid_t PID) : m_PID(PID), m_nCount(0)
{
}

CProcessEntry::~CProcessEntry()
{
}

int CProcessEntry::AddCount()
{
  ++m_nCount;
  return m_nCount;
}

int CProcessEntry::DecreaseCount()
{
  --m_nCount;
  return m_nCount;
}

pid_t CProcessEntry::GetPID()
{
  return m_PID;
}

int CProcessEntry::CheckCount()
{
  return m_nCount;
}


////////////////////////////////////////////////////////////////////////////////
//
//	CMountTable
//
////////////////////////////////////////////////////////////////////////////////
CMountEntry::CMountEntry() : m_PIDIter(m_PIDList)
{
}

CMountEntry::~CMountEntry()
{
}

void CMountEntry::CreateEntry(const char* Mount, const char *UNC, uid_t UID, 
			     gid_t GID, pid_t PID)
{
	pid_t *tempPID = new pid_t;
  *tempPID = PID;
  sprintf(m_pMount, "%s", Mount);
  sprintf(m_pUNC, "%s", UNC);
  m_UID = UID;
  m_GID = GID;
  m_nCount = 1;
  m_PIDList.append(tempPID);
}

bool CMountEntry::SearchEntry(const char* UNC, uid_t UID, gid_t GID)
{
  if (( UID == m_UID) && (GID == m_GID) && (!strcmp(UNC, m_pUNC)))
		return TRUE;
  else
		return FALSE;
}

bool CMountEntry::SearchPID(pid_t PID)
{
	m_PIDIter.toFirst();
  for ( ; m_PIDIter.current() != NULL; ++m_PIDIter )
  {
		if ( *(m_PIDIter.current()) == PID )
	    return TRUE;
  }
  return FALSE;
}

int CMountEntry::AddCount()
{
  ++m_nCount;
  return m_nCount;
}

int CMountEntry::DecreaseCount()
{
  --m_nCount;
  return m_nCount;
}

void CMountEntry::AddPID(pid_t PID)
{
  pid_t *tempPID = new pid_t;
  *tempPID = PID;
  m_PIDList.append(tempPID);
	tempPID = NULL;
}

bool CMountEntry::RemovePID(pid_t PID)
{
  m_PIDIter.toFirst();
	for ( ; m_PIDIter.current() != NULL; ++m_PIDIter )
	{
		if ( *(m_PIDIter.current()) == PID )
		{
			m_PIDList.remove(m_PIDIter.current());
			return TRUE;
		}
	}
	return FALSE;
}

const char* CMountEntry::GetMount()
{
  return m_pMount;
}

uid_t CMountEntry::GetUID()
{
  return m_UID;
}
