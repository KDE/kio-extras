/* Name: expcommon.h

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

#ifndef __INC_EXPCOMMON_H__
#define __INC_EXPCOMMON_H__

#include "acttask.h" // for LPFNACTTASKCALLBACK

extern int gnDesiredAppWidth;
extern int gnDesiredAppHeight;
extern int gnDesiredAppX;
extern int gnDesiredAppY;
extern int gnScreenWidth;
extern int gnScreenHeight;

class KHTMLView;
class CFileSystemInfo;

class CDocumentRequest 
{
public:
	CDocumentRequest(KHTMLView *pView, LPFNACTTASKCALLBACK Callback, LPCSTR Url, LPCSTR PostData = NULL);
	KHTMLView *m_pView;
	QString m_URL;
	QString m_LocalName;
	bool m_bTemp;
	QString m_PostData;
	LPFNACTTASKCALLBACK m_Callback;
};

void DownloadHTTP(CDocumentRequest *pRequest, BOOL bSilent = TRUE);
BOOL DecodeURLList(QDragEnterEvent *e, QStrList& l);
BOOL ItemAcceptsThisDrop(CNetworkTreeItem *pItem, QStrList& DraggedURLList, BOOL bCopyOnly);
int GetMouseState(QWidget *w);
CFileSystemInfo *FindMountedFileSystem(LPCSTR Path);

BOOL CanDeleteLocalPath(QString Path);
void RunMimeBinding(BOOL bNeedOpenWithDialog, QString &Name, LPCSTR ApplicationName = NULL);
void LaunchProgram(char *const *Param);
void LaunchURL(LPCSTR URL);

#endif /* __INC_EXPCOMMON_H__ */
