/* Name: inifile.cpp

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

#include "inifile.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "exports.h"

#include <qlist.h>

////////////////////////////////////////////////////////////////////////////

CSectionList gSambaConfiguration;

////////////////////////////////////////////////////////////////////////////

CSection::CSection(const CSection& other)
{
	m_SectionName = other.m_SectionName;
	m_FileName = other.m_FileName;
	m_bChanged = other.m_bChanged;

	QListIterator<CKeyEntry> it(other);
	
	for (; it.current() != NULL; ++it)
		append(new CKeyEntry(*(it.current())));
}

////////////////////////////////////////////////////////////////////////////

BOOL FindSectionOffsets(FILE *f, const QString& SectionName, long& StartFileOffset, long& EndFileOffset)
{
	char buf[2048];

	while (!feof(f))
	{
		long Offset = ftell(f);

		fgets(buf, sizeof(buf), f);
		
		if (feof(f) || strlen(buf) < 1)
			break;

		buf[strlen(buf)-1] = '\0'; // remove trailing '\n'
		
		if (Match(buf,"\\[*\\]*"))
		{
			LPCSTR p = &buf[1];

			if (ExtractWord(p, "]") == SectionName)
			{
				StartFileOffset = Offset;

				while (!feof(f))
				{
					fgets(buf, sizeof(buf), f);

					if (feof(f) || 
						strlen(buf) < 1 ||
						Match(buf,"\\[*\\]*"))
						return TRUE;
					
					if (buf[0] != ';' && buf[0] != '\n')
						EndFileOffset = ftell(f);
				}
			}
		}
	}

	return FALSE; // Section not found
}

////////////////////////////////////////////////////////////////////////////
//
// CSection::Write  - writes changed section into its config file.
//
// We try not to ask users for root password too much.
// If the config file is the standard smb.conf, we allow them to
// share/unshare resources without the password.
// To do this, we send our netserv process a command 'smbsection [sectionname] [size]'.
// If size is not 0, server will issue a '>' prompt for data stream.
// 
//
// If section's config file is not standard smb.conf (for instance, this
// section was #included from smb.conf), then for security reasons we
// don't allow them to do it without root password.
//

BOOL CSection::Write()
{
	if (m_FileName == gSmbConfLocation)
  {
    if (!count())
      return Empty();
    
    QString Send(QByteArray(8192));
    QString s;
    
    s.sprintf("[%s]\n", (LPCSTR)m_SectionName);
    Send += s;
  
    QListIterator<CKeyEntry> it(*this);
  
    for (it.toFirst(); it.current() != NULL; ++it)
    {
      s.sprintf("  %s=%s\n", (LPCSTR)it.current()->m_Key, (LPCSTR)it.current()->m_Value);
      Send += s;
    }

    s.sprintf("smbsection \"%s\" %d", 
              (LPCSTR)m_SectionName, 
              Send.length());

    int fd = GetServerOpenHandle(s);
		
    char buf[2];
		
    if (1 != ::read(fd, buf, 1))
      return FALSE; // server is not ready to accept our data

    buf[1] = '\0';

    ::write(fd, (LPCSTR)Send, Send.length());
    ::read(fd, buf, 1); // read result code: '0' means failure, '1' means success
    
    return buf[0] == '1';
  }
	
  FILE *fi = fopen((LPCSTR)m_FileName, "r");
	
	if (NULL == fi)
		return FALSE;

	char BakFileName[] = "/tmp/~~samba~configXXXXXX";
	mktemp(BakFileName);

	//QString BakFileName = m_FileName + ".tmp";

	FILE *fo = fopen((LPCSTR)BakFileName, "w");

	if (NULL == fo)
	{
		fclose(fi);
		return FALSE;
	}

	// Re-read the source INI file to get proper offset values.
	
	long StartFileOffset, EndFileOffset;
	
	if (!FindSectionOffsets(fi, m_SectionName, StartFileOffset, EndFileOffset))
	{
		fseek(fi, 0L, 2);

		StartFileOffset = ftell(fi);
		EndFileOffset = StartFileOffset;
	}

	//printf("Writing section %s: start offset = %ld, end offset = %ld, file size = %ld\n",(LPCSTR)m_SectionName, StartFileOffset, EndFileOffset, ftell(fi));

	fseek(fi, 0L, 0);

	// First, write all preceding sections (if any)

	if (StartFileOffset > 0)
	{
		char *pBuf = new char[StartFileOffset];
		
		//printf("Writing start %ld bytes\n", StartFileOffset);

		if (StartFileOffset != (long)fread(pBuf, 1, StartFileOffset, fi) ||
			StartFileOffset != (long)fwrite(pBuf, 1, StartFileOffset, fo))
		{
			//printf("Error reading or writing\n");

			delete []pBuf;
			fclose(fi);
			fclose(fo);
			::unlink(BakFileName);
			return FALSE;
		}

		delete []pBuf;
	}

	// Now write section body itself

	if (count() > 0)
	{
		fprintf(fo, "[%s]\n", (LPCSTR)m_SectionName);
		//printf("[%s]\n", (LPCSTR)m_SectionName);
		
		QListIterator<CKeyEntry> it(*this);
		
		for (it.toFirst(); it.current() != NULL; ++it)
		{
			fprintf(fo, "  %s=%s\n", (LPCSTR)it.current()->m_Key, (LPCSTR)it.current()->m_Value);
			
			//printf("  %s=%s\n", (LPCSTR)it.current()->m_Key, (LPCSTR)it.current()->m_Value);
		}
	}

	// And copy the file tail from the source file.

	fseek(fi,0L,2);
	long FileSize = ftell(fi);

	if (EndFileOffset < FileSize)
	{
		fseek(fi, EndFileOffset, 0);
		
		long lCount = FileSize - EndFileOffset;

		//printf("Writing %ld tail bytes\n", lCount);
		
		char *pBuf = new char[lCount];

		if (lCount != (long)fread(pBuf, 1, lCount, fi) ||
			lCount != (long)fwrite(pBuf, 1, lCount, fo))
		{
			//printf("Error reading/writing\n");

			delete []pBuf;
			fclose(fi);
			fclose(fo);
			::unlink((LPCSTR)BakFileName);
			return FALSE;
		}

		delete []pBuf;
	}

	// Finish up...

	fclose(fi);
	fclose(fo);
	
	if (!access((LPCSTR)m_FileName, W_OK))
	{
		BOOL bCopyRetcode = CopyFile(BakFileName, m_FileName);

    ::unlink(BakFileName);

		if (!bCopyRetcode)
			return FALSE; // unable to update file
	}
	else
	{
		struct stat st;
		uid_t uid = getuid();
		gid_t gid = getgid();
		
		if (!stat(m_FileName, &st))
		{
			uid = st.st_uid;
			gid = st.st_gid;
		}

		QString cmd;
		
		cmd.sprintf("mv -f %s %s;chown %u.%u %s", (LPCSTR)BakFileName, (LPCSTR)m_FileName, uid, gid, (LPCSTR)m_FileName);
		
		if (SuperUserExecute("Save Samba settings", cmd, NULL, 0))
		{
			::unlink(BakFileName);
			return FALSE; // unable to update file
		}
	}

	m_bChanged = FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CSection::Empty()
{
	if (m_FileName == gSmbConfLocation)
  {
    QString s;
    s.sprintf("smbsection \"%s\" 0", 
              (LPCSTR)m_SectionName);

    return ServerExecute(s);
  }
	
  FILE *fi = fopen((LPCSTR)m_FileName, "r");
	
	if (NULL == fi)
		return FALSE;

	char BakFileName[] = "/tmp/~~samba~configXXXXXX";
	mktemp(BakFileName);

	FILE *fo = fopen((LPCSTR)BakFileName, "w");

	if (NULL == fo)
	{
		fclose(fi);
		return FALSE;
	}

	// Re-read the source INI file to get proper offset values.
	
	long StartFileOffset, EndFileOffset;
	
	if (!FindSectionOffsets(fi, m_SectionName, StartFileOffset, EndFileOffset))
	{
		fseek(fi, 0L, 2);

		StartFileOffset = ftell(fi);
		EndFileOffset = StartFileOffset;
	}

	fseek(fi, 0L, 0);

	// First, write all preceding sections (if any)

	if (StartFileOffset > 0)
	{
		char *pBuf = new char[StartFileOffset];
		
		if (StartFileOffset != (long)fread(pBuf, 1, StartFileOffset, fi) ||
			StartFileOffset != (long)fwrite(pBuf, 1, StartFileOffset, fo))
		{
			delete []pBuf;
			fclose(fi);
			fclose(fo);
			::unlink(BakFileName);
			return FALSE;
		}

		delete []pBuf;
	}

	// And copy the file tail from the source file.

	fseek(fi,0L,2);
	long FileSize = ftell(fi);

	if (EndFileOffset < FileSize)
	{
		fseek(fi, EndFileOffset, 0);
		
		long lCount = FileSize - EndFileOffset;

		char *pBuf = new char[lCount];

		if (lCount != (long)fread(pBuf, 1, lCount, fi) ||
			lCount != (long)fwrite(pBuf, 1, lCount, fo))
		{
			delete []pBuf;
			fclose(fi);
			fclose(fo);
			::unlink((LPCSTR)BakFileName);
			return FALSE;
		}

		delete []pBuf;
	}

	// Finish up...

	fclose(fi);
	fclose(fo);

	if (!access((LPCSTR)m_FileName, W_OK))
	{
		BOOL bCopyRetcode = CopyFile(BakFileName, m_FileName);

    ::unlink(BakFileName);
		
		if (!bCopyRetcode)
			return FALSE; // unable to update file
	}
	else
	{
		struct stat st;
		uid_t uid = getuid();
		gid_t gid = getgid();
		
		if (!stat(m_FileName, &st))
		{
			uid = st.st_uid;
			gid = st.st_gid;
		}

		QString cmd;
		
		cmd.sprintf("mv -f %s %s;chown %u.%u %s", (LPCSTR)BakFileName, (LPCSTR)m_FileName, uid, gid, (LPCSTR)m_FileName);
		
		if (SuperUserExecute("Save Samba settings", cmd, NULL, 0))
		{
			::unlink(BakFileName);
			return FALSE; // unable to update file
		}
	}

	m_bChanged = FALSE;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// This function searches option list from the end and
// is looking for the first match.
// (We search backwards because SMB.CONF is meant to be parsed
// line by line and can have conflicting statements. The one
// which appears later will override the previous setting).
// We are obviously interested only in the meaningful value.

LPCSTR CSection::Value(LPCSTR Key, LPCSTR ReplaceKey /* = NULL */)
{
	QListIterator<CKeyEntry> it(*this);
	
	for (it.toLast(); it.current() != NULL; --it)
	{
		if (!stricmp(it.current()->m_Key, Key))
		{
			if (NULL != ReplaceKey)
				it.current()->m_Key = ReplaceKey;

			//printf("Found: %s\n", (LPCSTR)(it.current()->m_Value));
			return (LPCSTR)(it.current()->m_Value);
		}
	}
	
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

void CSection::SetValue(LPCSTR Value, 
						LPCSTR Key, 
						LPCSTR Synonym1 /*= NULL*/, 
						LPCSTR Synonym2 /*= NULL */,
						LPCSTR Antonym /*= NULL*/)
{
	QListIterator<CKeyEntry> it(*this);
	
	//printf("SetValue[%s](%s,%s)\n", Value, Key, (LPCSTR)m_SectionName);

	for (it.toLast(); it.current() != NULL; --it)
	{
		if (!stricmp(it.current()->m_Key, Key) ||
			(Synonym1 != NULL && !stricmp(it.current()->m_Key, Synonym1)) ||
			(Synonym2 != NULL && !stricmp(it.current()->m_Key, Synonym2)))
		{
			if (Value == NULL || *Value == '\0')
			{
				remove(it.current());
				m_bChanged = TRUE;
			}
			else
				if (it.current()->m_Value != Value)
				{
					it.current()->m_Value = Value;
					m_bChanged = TRUE;
				}

			return;
		}

		if (Antonym != NULL &&
			!stricmp(it.current()->m_Key, Antonym))
		{
			it.current()->m_Value = !stricmp(Value, "yes") ? "no" : "yes";
			return;
		}
	}
	
   
	if (NULL != Value && *Value != '\0')
	{
		CKeyEntry *pKE = new CKeyEntry;
		pKE->m_Key = Key;
		pKE->m_Value = Value;
   
		append(pKE);
		m_bChanged = TRUE;
		//printf("Added!\n");
	}
}

////////////////////////////////////////////////////////////////////////////
// From J.D. Blair's SAMBA book:
// ...If "public" or "guest ok" is true then guest access will be
// allowed. The access rights of a client connecting as guest will be those
// of the username set in the "guest user" parameter.
// If either is false, no password will be required to access this
// service. The access rights will be those of the account set by the
// "guest user" parameter. Note that then no password will be required
// to access this service. The access rights will be those of the account
// set by the "guest user" parameter. Note that "public" or "guest ok" option
// does not force all users to connect as the guest user. A user will connect
// normally, assuming the rights of the authenticated username, is a valid
// password is supplied. Guest access rights are used if a user attempts a guest
// login...

BOOL CSection::IsPublic()
{
	LPCSTR v = Value("public");
	
	if (v == NULL)
		v = Value("guest ok"); // synonym

	if (v == NULL)
		return FALSE;  // default is "no"

	return !stricmp(v, "yes");
}

////////////////////////////////////////////////////////////////////////////

void CSection::SetPublic(BOOL bIsPublic)
{
	SetValue(bIsPublic ? "yes" : "no", "public", "guest ok");
}

////////////////////////////////////////////////////////////////////////////

BOOL CSection::IsWritable()
{
	Value("writable", "writeable");  // fix common misspelling

	QListIterator<CKeyEntry> it(*this);
	
	for (it.toLast(); it.current() != NULL; --it)
	{
		LPCSTR key = it.current()->m_Key;
		
    if (!stricmp(key, "printable") &&
        !stricmp(it.current()->m_Value, "yes"))
      return TRUE; // printable shares are writable by default
		
		if (!stricmp(key, "writeable") ||
			!stricmp(key, "write ok"))  // synonym
		{
		   return !stricmp(it.current()->m_Value, "yes");
		}

		if (!stricmp(key, "read only"))
		   return stricmp(it.current()->m_Value, "yes");
	}

	return FALSE; // default is "no"
}

////////////////////////////////////////////////////////////////////////////

void CSection::SetWritable(BOOL bIsWritable)
{
	SetValue(bIsWritable ? "yes" : "no", "writeable", "write ok", "read only");
}

////////////////////////////////////////////////////////////////////////////

BOOL CSection::IsBrowseable()
{
	LPCSTR v = Value("browseable");
	
	if (v == NULL)
		return TRUE;  // default is "yes"

	return !stricmp(v, "yes");
}

////////////////////////////////////////////////////////////////////////////

BOOL CSection::IsPrintable()
{
	LPCSTR v = Value("printable");
	
	if (v == NULL)
		return FALSE;  // default is "no"

	return !stricmp(v, "yes");
}

////////////////////////////////////////////////////////////////////////////

void CSection::SetBrowseable(BOOL bIsBrowseable)
{
	SetValue(bIsBrowseable ? "yes" : "no", "browseable");
}

////////////////////////////////////////////////////////////////////////////
// From J.D. Blair's SAMBA book:
// This parameter lets you remove a sevice from availability.
// If "available=no" all attempts to connect to the service will fail.
// Such failures are logged. Using this option preserves the service's
// settings and is usually much more convenient than commenting out the
// service. This parameter is also useful to create om inaccessible
// template service that is used to set defaults for subsequent services
// using the "copy" command. If you do this, don't forget to use the
// "available=yes" in the actual service definition.

BOOL CSection::IsAvailable()
{
	LPCSTR v = Value("available");
	
	if (v == NULL)
		return TRUE;  // default is "yes"

	return !stricmp(v, "yes");
}

////////////////////////////////////////////////////////////////////////////

void CSection::SetAvailable(BOOL bIsAvailable)
{
	SetValue(bIsAvailable ? "yes" : "no", "available");
}

////////////////////////////////////////////////////////////////////////////

static BOOL ReadNextIniSection(FILE *f, CSection *pSection)
{
	char buf[2048];

	while (!feof(f))
	{
		//long Offset = ftell(f);

		fgets(buf, sizeof(buf), f);
		
		if (feof(f) || strlen(buf) < 1)
			break;

		buf[strlen(buf)-1] = '\0'; // remove trailing '\n'
		
		if (Match(buf,"\\[*\\]*"))
		{
			LPCSTR p = &buf[1];
			pSection->m_SectionName = ExtractWord(p, "]");

			while (!feof(f))
			{
				long lOff = ftell(f);

				fgets(buf, sizeof(buf), f);

				if (feof(f) || strlen(buf) < 1)
					break;

				if (Match(buf,"\\[*\\]*")) // oops, next section...
				{
					fseek(f, lOff, 0);
					break;
				}

				if (buf[0] != ';' && buf[0] != '\n')
				{
				   if (Match(buf,"*=*"))
				   {
					   CKeyEntry *pKE = new CKeyEntry;

					   p = &buf[0];
					   pKE->m_Key = ExtractWord(p,"=").stripWhiteSpace();
					   pKE->m_Value = ExtractTail(p," =");
					   
					   pSection->append(pKE);
				   }
				}
			}
			return TRUE;
		}
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

static QString ExpandMacros(LPCSTR p)
{
	QString s;
	
	while (*p != '\0')
	{
		if (*p == '%')
		{
			switch (*++p)
			{
				case 'U':
				case 'u':
					s += gCredentials[0].m_UserName;
				break;

				case 'G':
					s += gCredentials[0].m_Workgroup;
				break;

				case 'M':
				case 'm':
				case 'h':
				case 'L':
					s += GetHostName();
				break;

				case 'I':
					s += GetIPAddress();
				break;

				case 'H':
					s += GetHomeDir(gCredentials[0].m_UserName);
				break;

				case 'g':
					s += GetGroupName(gCredentials[0].m_UserName);
				break;
			}
		}
		else
		{
			s += *p++;
		}
	}
	
	return s;
}

////////////////////////////////////////////////////////////////////////////

BOOL CSectionList::Read(LPCSTR FileName)
{
	FILE *f = fopen(FileName, "r");

	if (NULL == f)
	{
		printf("Cannot read %s\n", FileName);
		return FALSE; // file is unreadable
	}

	CSection *pSection;

	while (1)
	{
		pSection = new CSection;
		pSection->m_FileName = FileName;

		if (ReadNextIniSection(f, pSection))
			append(pSection);
		else
		{
			delete pSection;
			break;
		}
	}

	fclose(f);

RescanIncludes:;

	for (pSection=first(); pSection != NULL; pSection = next())
	{
		if (!stricmp(pSection->m_SectionName,"global"))
		{
			LPCSTR pIncludeFile = pSection->Value("include", "*include");
			
			if (NULL != pIncludeFile)
			{
				Read(ExpandMacros(pIncludeFile));
				goto RescanIncludes;
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

LPCSTR CSectionList::FindShare(LPCSTR Path)
{
	QListIterator<CSection> it(*this);
	
	for (; it.current() != NULL; ++it)
	{
		if (stricmp(it.current()->m_SectionName, "global"))
		{
			if (!strcmp(it.current()->Value("path", NULL), Path))
				return (LPCSTR)(it.current()->m_SectionName);
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

LPCSTR CSectionList::Value(LPCSTR Key)
{
	QListIterator<CSection> it(*this);
	
	for (; it.current() != NULL; ++it)
	{
		LPCSTR ret = it.current()->Value(Key, NULL);
		
		if (ret != NULL)
			return ret;
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////

BOOL CSectionList::ShareExists(LPCSTR ShareName)
{
	QListIterator<CSection> it(*this);
	
	for (; it.current() != NULL; ++it)
	{
		if (!stricmp(it.current()->m_SectionName, ShareName))
			return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CSectionList::IsLoadingPrinters()
{
	QListIterator<CSection> it(*this);
	BOOL bValue = TRUE; // default is "yes"

	for (; it.current() != NULL; ++it)
	{
		if (!stricmp(it.current()->m_SectionName,"global"))
		{
			LPCSTR pValue = it.current()->Value("load printers", NULL);
			
			if (NULL != pValue)
				bValue = !stricmp(pValue, "yes");

      break;
		}
	}

  return bValue;
}

////////////////////////////////////////////////////////////////////////////

CSectionList::CSectionList(CSectionList& other)
{
	setAutoDelete(TRUE);
	Copy(other);	
}

////////////////////////////////////////////////////////////////////////////

void CSectionList::Copy(CSectionList& other)
{
	clear();

	QListIterator<CSection> it(other);
	
	for (; it.current() != NULL; ++it)
	{
		if (it.current()->count() > 0)
			append(new CSection(*it.current()));
	}
}

////////////////////////////////////////////////////////////////////////////

void DeleteLocalShares(LPCSTR Path)
{
	if (gSmbConfLocation.isEmpty())
		ReadConfiguration();

	if (gbNetworkAvailable)
	{
		QListIterator<CSection> it(gSambaConfiguration);

		for (; it.current() != NULL; ++it)
		{
			if (stricmp(it.current()->m_SectionName, "global"))
			{
				if (IsSamePath(it.current()->Value("path"), Path))
				{
					it.current()->Empty();
				}
			}
		}
	}
  
  if (IsNFSShared(Path))
  {
    QString cmd;
    cmd.sprintf("nfsupdate \"%s\"", Path);
    ServerExecute(cmd);
  }
}

////////////////////////////////////////////////////////////////////////////

void RenameLocalShares(LPCSTR OldPath, LPCSTR NewPath)
{
	if (gbNetworkAvailable)
	{
		QListIterator<CSection> it(gSambaConfiguration);
	
		for (; it.current() != NULL; ++it)
		{
			if (stricmp(it.current()->m_SectionName, "global"))
			{
				if (!strcmp(it.current()->Value("path"), OldPath))
				{
					it.current()->SetValue(NewPath, "path");
					it.current()->Write();
				}
			}
		}
	}
  
  if (IsNFSShared(OldPath))
  {
    QString cmd;
    cmd.sprintf("nfsupdate \"%s\" \"%s\"", (LPCSTR)OldPath, (LPCSTR)NewPath);
    ServerExecute(cmd);
  }
}

////////////////////////////////////////////////////////////////////////////


