/* Name: smbutil.h

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

#ifndef __INC_SMBUTIL_H__
#define __INC_SMBUTIL_H__

#include "common.h"

#define MAX_TIMEOUTS 2

class CServerArray;
class CShareArray;
class CFileArray;
class CFileSystemArray;
class CMSWindowsNetworkItem;
class CSMBFileInfo;

QString GetMaster(LPCSTR Workgroup);
CSMBErrorCode GetServerList(LPCSTR Workgroup, LPCSTR MasterHint, CServerArray *pServerList, int nCredentialsIndex);
CSMBErrorCode GetShareList(LPCSTR Server, CShareArray *pShareList, int nCredentialsIndex);
CSMBErrorCode GetWorkgroupList();
CSMBErrorCode GetFileList(LPCSTR UNCPath, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles = FALSE, LPCSTR sFilter = NULL);
CSMBFileInfo *FillFileInfo(CSMBFileInfo *pFileInfo, LPCSTR FileName, LPCSTR Shortname, struct stat *pStat, struct stat *pSttarget);

BOOL GetLocalFileList(LPCSTR Path, CFileArray *pFileList, BOOL bWantRegularFiles = FALSE);
CSMBErrorCode GetWorkgroupByServer(LPCSTR Server, QString& Workgroup);
CSMBErrorCode GetWorkgroupAndCommentByServer(LPCSTR Server, QString& Workgroup, QString& Comment);

BOOL MountNFSShare(LPCSTR ServiceName, LPCSTR MountPoint, BOOL bReconnectAtLogon);

BOOL CanUmountSMB(LPCSTR MountPointPath);
BOOL MountSMBShare(LPCSTR UNCPath, LPCSTR MountPointPath, LPCSTR Workgroup, LPCSTR UserName, LPCSTR Password, BOOL bReconnectAtLogon);

CSMBErrorCode UmountFilesystem(LPCSTR MountPoint, BOOL bIsNFS, QString &ErrorDescription);
BOOL UmountSMBShare(LPCSTR MountPointPath);

BOOL OnMountNetworkShare(QString& MountPointPath, CMSWindowsNetworkItem *pOtherTree, int nCredentialsIndex, QWidget *pParent);
void RestartSamba();

void CopySMBDir(LPCSTR Src, LPCSTR Dst, int nCredentialsIndex);
void MakeFileCopyList(LPCSTR UNCPath, CFileJob *pResult, int nCredentialsIndex, unsigned long &nTotalSize, unsigned long &nFileCount);
CSMBErrorCode SMBMkdir(LPCSTR UNCPath, int nCredentialsIndex = 0);
CSMBErrorCode SMBRename(LPCSTR UNCPath, LPCSTR NewLabel, int nCredentialsIndex = 0);

int SMBFolderExists(LPCSTR UNCPath, int nCredentialsIndex = 0);
int SMBPrint(LPCSTR UNCPath, LPCSTR FileName, int nCredentialsIndex);
void GetMasterBrowser();

#endif /* __INC_SMBUTIL_H__ */
