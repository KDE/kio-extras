/* Name: table.h

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

#ifndef _TABLE_H
#define _TABLE_H

class CProcessEntry;
class CMountEntry;
extern QList <CProcessEntry> gProcessTable;
extern QList <CMountEntry> gMountTable;

// CProcessEntry
class CProcessEntry
{
public:
    CProcessEntry(pid_t PID);
    ~CProcessEntry();
    int AddCount();
    int DecreaseCount();
    pid_t GetPID();
    int CheckCount();
        
private:
    pid_t m_PID;
    int m_nCount;
};


//CMountEntry
class CMountEntry
{
public:
    CMountEntry();
    ~CMountEntry();
    void CreateEntry(const char* Mount, const char *UNC, uid_t UID,
		    gid_t GID, pid_t PID);
    bool SearchEntry(const char* UNC, uid_t UID, gid_t GID);
    bool SearchPID(pid_t PID);
    int AddCount();
    int DecreaseCount();
    void AddPID(pid_t PID);
    bool RemovePID(pid_t PID);
    const char* GetMount();
    uid_t GetUID();
    
private:
    char m_pMount[1024];
    char m_pUNC[1024];
    uid_t m_UID;
    gid_t m_GID;
    int m_nCount;
    QList <pid_t> m_PIDList;
    QListIterator <pid_t> m_PIDIter;
};    

#endif /* _TABLE_H */
