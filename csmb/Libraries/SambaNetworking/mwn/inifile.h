/* Name: inifile.h

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

#ifndef __INC_INIFILE_H__
#define __INC_INIFILE_H__

#include "common.h"
#include "qlist.h"

class CKeyEntry
{
public:
	CKeyEntry()
	{
	}

	CKeyEntry(const CKeyEntry& other)
	{
		m_Key = other.m_Key;
		m_Value = other.m_Value;
	}

	QString m_Key;
	QString m_Value;
};

class CSection : public QList<CKeyEntry>
{
public:
	CSection()
	{
		setAutoDelete(TRUE);
		m_bChanged = FALSE;
	}

	CSection(const CSection& other);

	BOOL Write();
	BOOL Empty();

	BOOL IsPublic();
	BOOL IsWritable();
	BOOL IsBrowseable();
	BOOL IsAvailable();
  BOOL IsPrintable();

	void SetPublic(BOOL);
	void SetWritable(BOOL);
	void SetBrowseable(BOOL);
	void SetAvailable(BOOL);
	void SetValue(LPCSTR Value, LPCSTR Key, LPCSTR Synonym1 = NULL, LPCSTR Synonym2 = NULL, LPCSTR Antonym = NULL);

	LPCSTR Value(LPCSTR Key, LPCSTR ReplaceKey = NULL);

	QString m_SectionName;
	QString m_FileName;
	BOOL m_bChanged;
};

class CSectionList : public QList<CSection>
{
public:
	CSectionList()
	{
		setAutoDelete(TRUE);
	}
	
	CSectionList(CSectionList& other);
	
	void Copy(CSectionList& other); 
	BOOL Read(LPCSTR FileName);
	LPCSTR FindShare(LPCSTR Path);
	BOOL ShareExists(LPCSTR ShareName);
	LPCSTR Value(LPCSTR Key);
  BOOL IsLoadingPrinters();
  BOOL Write();
};

extern CSectionList gSambaConfiguration;
void DeleteLocalShares(LPCSTR Path);
void RenameLocalShares(LPCSTR OldPath, LPCSTR NewPath);
BOOL FindSectionOffsets(FILE *f, const QString& SectionName, long& StartFileOffset, long& EndFileOffset);

#endif /* __INC_INIFILE_H__ */
