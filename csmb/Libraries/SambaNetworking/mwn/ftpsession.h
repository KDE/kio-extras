/* Name: ftpsession.h

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

#ifndef __INC_FTPSESSION_H__
#define __INC_FTPSESSION_H__

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "ftpfile.h"
#include <qlist.h>

typedef enum
{
	keFTP_Undefined,
	keFTP_Idle,
	keFTP_Working
} CFTPStatus;

////////////////////////////////////////////////////////////////////////////

class CFtpSession
{
public:
	CFtpSession(CNetworkTreeItem *pItem, LPCSTR Url, int nCredentialsIndex);

	~CFtpSession();
	
	CSMBErrorCode GetFTPFileList(LPCSTR UNCPath, 
								 CFileArray *pFileList, 
								 int nCredentialsIndex, 
								 BOOL bWantRegularFiles = FALSE);

	CSMBErrorCode GetFile(LPCSTR Source, LPCSTR Destination, LPFNStatusCallback pStatusCallback, void *UserData);
	CSMBErrorCode PutFile(LPCSTR Source, LPCSTR Destination, LPFNStatusCallback pStatusCallback, void *UserData);
	CSMBErrorCode RenameFile(LPCSTR Source, LPCSTR Destination);
	CSMBErrorCode DeleteFile(LPCSTR Url);

	BOOL CountFolderContents(LPCSTR Url, unsigned long& dwNumFiles, unsigned long& dwNumFolders, double& dwTotalSize, CFileJob *pResult, BOOL bRecursive, BOOL bContainersGoLast, int BaseNameLength);
	BOOL ExecuteCommand(LPCSTR cmd);
	
	int Receive(LPCSTR *Keywords);
	CSMBErrorCode Login();
	BOOL Chdir(LPCSTR Folder);
	void ReadInput(LPSTR buf, int nMaxCount);
	BOOL RetryAuthentication();
	BOOL IsFTPValidFolder(LPCSTR name);
	BOOL IsFTPValidFile(LPCSTR name);
	int FTPFolderExists(LPCSTR Url);
	int FTPMakeDir(LPCSTR name);
	LPCSTR PopupCaption();
	int FTPStat(LPCSTR Url, struct stat *st);

	QString m_URL;

	FILE *m_Out;
	FILE *m_In;
	FILE *m_Err;
	
	pid_t m_Pid;
	CFTPStatus m_Status;
	int m_nCredentialsIndex;
	CNetworkTreeItem *m_pItem;
	char m_MsgBuf[1024];
private:
  void InitFtpAgent();
};

////////////////////////////////////////////////////////////////////////////

class CFtpSessionList : public QList<CFtpSession>
{
public:
	CFtpSessionList()
	{
		setAutoDelete(TRUE);
	}

	CListViewItem *FindAndExpand(CListViewItem *pParent, LPCSTR Url);
	CFtpSession *GetSession(CNetworkTreeItem *pItem, LPCSTR Url, int nDefaultCredentialsIndex = 1);
};

////////////////////////////////////////////////////////////////////////////

extern CFtpSessionList gFtpSessions;
BOOL IsFTPValidFolder(LPCSTR name);
BOOL IsFTPValidFile(LPCSTR name);
int FTPFolderExists(LPCSTR Url);
int FTPMakeDir(LPCSTR Url);
int FTPStat(LPCSTR Url, struct stat *st);
int FTPUnlink(LPCSTR Url);

#endif /* __INC_FTPSESSION_H__ */

