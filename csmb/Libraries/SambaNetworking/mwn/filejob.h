/* Name: filejob.h

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

#ifndef __INC_FILEJOB_H__
#define __INC_FILEJOB_H__

#include "qlist.h"
#include "array.h"
#include <qstrlist.h>

typedef enum
{
	keFileJobSuccess,
	keFileJobDestinationNotFound,
	keFileJobDestinationNotWritable,
	keFileJobSourceNotFound,
	keFileJobSourceNotReadable
} CFileJobError;

class CFileJobElement
{
public:
	CFileJobElement()
	{
	}

	CFileJobElement(LPCSTR FileName,
									time_t FileTime,
									size_t FileSize,
									mode_t FileMode,
					unsigned long nFileCount,
					double nByteCount,
					int BaseNameLength,
				  int ChainStatus) : m_FileName(FileName)
	{
		m_FileTime = FileTime;
		m_FileSize = FileSize;
		m_FileMode = FileMode;
		m_nFileCount = nFileCount;
		m_nByteCount = nByteCount;
		m_nBaseNameLength = BaseNameLength;
		m_nChainStatus = ChainStatus;
	}

	CFileJobElement(const CFileJobElement& other) : m_FileName(other.m_FileName)
	{
		m_FileTime = other.m_FileTime;
		m_FileSize = other.m_FileSize;
		m_FileMode = other.m_FileMode;
		m_nFileCount = other.m_nFileCount;
		m_nByteCount = other.m_nByteCount;
		m_nBaseNameLength = other.m_nBaseNameLength;
		m_nChainStatus = other.m_nChainStatus;
	}

	CFileJobElement& operator=(const CFileJobElement& other)
	{
		m_FileTime = other.m_FileTime;
		m_FileSize = other.m_FileSize;
		m_FileName = other.m_FileName;
		m_FileMode = other.m_FileMode;
		m_nFileCount = other.m_nFileCount;
		m_nByteCount = other.m_nByteCount;
		m_nBaseNameLength = other.m_nBaseNameLength;
		m_nChainStatus = other.m_nChainStatus;
		return *this;
	}

	CSMBErrorCode CopyMove(LPCSTR Destination, // where to copy or move (will be /mnt/CorelMntDir/xxxx for UNC names)
								LPCSTR DestinationDisplay, // what to display on screen as a destination (can be UNC name)
								BOOL bMove, 
								BOOL bDestinationFinalized, 
								double& dwTotalSize, 
								double& dwNowAtByte,
								BOOL bThisIsLastItem);

	BOOL Delete(BOOL& bNeedWarning);
	BOOL FTPDelete();
	BOOL LocalDelete();
	
	void SetParentModTime();

	QString m_FileName;
	unsigned long m_nFileCount;
	double m_nByteCount;
	int m_nBaseNameLength;
	time_t m_FileTime;
	size_t m_FileSize;
	mode_t m_FileMode;
	int m_nChainStatus; // 0: beginning; 1: inside; 2: tail
};

typedef enum
{
	keFileJobNone,
	keFileJobCopy,
	keFileJobMove,
	keFileJobDelete,
	keFileJobMkdir,
	keFileJobMount,
	keFileJobUmount,
	keFileJobList,
	keFileJobPrint
} CFileJobType;

class CFileJob : public /*CVector<CFileJobElement, CFileJobElement&>*/ QList<CFileJobElement>
{
public:
	CFileJob() :
		m_Outstanding(TRUE),
		m_Ignore(TRUE)
	{
		setAutoDelete(TRUE);
		m_Type = keFileJobNone;
		/*SetSize(5000, 5000);*/
		
		m_dwTotalSize = 0.;
		m_dwNumFiles = 0;
		m_dwNumFolders = 0;
		m_dwNowAtFile = 0;
		m_dwNowAtByte = 0;
	}

	CSMBErrorCode Run();

	QString m_Destination;
	CFileJobType m_Type;
	double m_dwTotalSize;
	unsigned long m_dwNumFiles;
	unsigned long m_dwNumFolders;
	unsigned long m_dwNowAtFile;
	double m_dwNowAtByte;
	static QString m_NowAtFile;
	static QString m_UnfinishedFile;
	static int m_nCount;
	BOOL m_bDestinationFinalized;
	BOOL m_bNeedDeleteWarning;
	QStrList m_Outstanding;
	QStrList m_Ignore;
};

extern int gIdleTime;
extern BOOL gbIdling;
void StartIdle();
void StopIdle();

#endif /* __INC_FILEJOB_H__ */
