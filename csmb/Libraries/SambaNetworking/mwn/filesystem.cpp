/* Name: filesystem.cpp

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

#include <stdio.h>
#include "filesystem.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "inifile.h"
#include <mntent.h>

CFileSystemArray gFileSystemList; // global filesystem list
time_t gFileSystemListTimestamp; // last update time of global filesystem list

QPixmap *CFileSystemInfo::Pixmap(BOOL bIsBig)	
{
  BOOL bShared = (gSambaConfiguration.FindShare(m_MountedOn) != NULL);

  switch (m_DriveType)
	{
		case keNetworkDrive:
			return LoadPixmap(bIsBig ? keNetworkFileSystemIconBig : keNetworkFileSystemIcon, FALSE, bShared);
		
		case keLocalDrive:
		default:
			return LoadPixmap(bIsBig ? keLocalFileSystemIconBig : keLocalFileSystemIcon, FALSE, bShared);

		case keCdromDrive:
			return LoadPixmap(bIsBig ? keCdromFileSystemIconBig : keCdromFileSystemIcon, FALSE, bShared);
    case keZIPDrive:
			return LoadPixmap(bIsBig ? keZIPDriveIconBig : keZIPDriveIcon, FALSE, bShared);
    case keFloppyDrive:
			return LoadPixmap(bIsBig ? keFloppyDriveIconBig : keFloppyDriveIcon, FALSE, bShared);
	}
}

QPixmap *CFileSystemItem::Pixmap(BOOL bIsBig)
{ 
  return CFileSystemInfo::Pixmap(bIsBig);
}

void CFileSystemItem::GuessDriveType()
{
	if (m_Type == "smbfs" || m_Type == "nfs")
		m_DriveType = keNetworkDrive;
	else
	{
		struct stat st;
		
		int nMajor = 0;

		if (-1 != stat(m_Name, &st))
		{
			if (S_IFCHR == (st.st_mode & S_IFMT) ||
				S_IFBLK == (st.st_mode & S_IFMT))
				nMajor = major(st.st_rdev);
		}
		
		if (nMajor == 11 || m_Type == "iso9660")
			m_DriveType = keCdromDrive;
		else
		{
			if (nMajor == 2)
				m_DriveType = keFloppyDrive;
			else
				m_DriveType = keLocalDrive;
		}
	}
}

////////////////////////////////////////////////////////////////////////////

static BOOL IsReadOnly(LPCSTR MountedOn)
{
	FILE *f = setmntent("/etc/mtab", "r");
	
	if (NULL != f)
	{
		struct mntent *me;

		while (NULL != (me = getmntent(f)))
		{
			if (!strcmp(MountedOn, me->mnt_dir))
			{
				if (Match(me->mnt_opts, "ro*"))
				{
					endmntent(f);
					return TRUE;
				}
			}
		}

		endmntent(f);
		return FALSE;
	}

	return FALSE; // we don't know, so let's assume it is full access...
}

////////////////////////////////////////////////////////////////////////////

void GetFileSystemList(CFileSystemArray *pFileSystemList)
{
  if (NULL == pFileSystemList)
		return;

  pFileSystemList->clear();

	static BOOL bSupportSpacesFlag = TRUE;

  FILE *f;

TryAgain:;

  // In Corel Linux we ship our own version of 'df' command
  // which understands a new '-e' flag. This flag tells df to
  // support spaces in the mount point and share names.
  // Unfortunately, standard 'df' will choke on the unknown flag
  // and will output the error message into the
  // stderr stream. So we first run 'df' with the '-e' flag and
  // redirect stderr to the stdout (so popen can pick it up).
  // If that didn't work, we set bSupportSpacesFlag to FALSE and
  // from now on will use older format without '-e'.

  f = popen(bSupportSpacesFlag ? "df -PTe 2>&1" : "df -PT", "r");

	if (NULL != f)
	{
		int bNotFirstLine = FALSE;

		while (!feof(f))
		{
			char buf[2048];
			CFileSystemInfo fsi;

			fgets(buf, sizeof(buf), f);
			
      if (bSupportSpacesFlag &&
          Match(buf, "*df: invalid option*"))
      {
        bSupportSpacesFlag = FALSE;
        pclose(f);
        goto TryAgain;
      }
      
      LPCSTR p = &buf[0];
			
			if (!feof(f))
			{
				if (bNotFirstLine)
				{
				   fsi.m_Name = ExtractWordEscaped(p);
				   fsi.m_Type = ExtractWord(p);
				   
				   if (fsi.m_Type == "smbfs")
					   fsi.m_Name = MakeSlashesBackward(fsi.m_Name);

				   fsi.m_TotalK = ExtractWord(p);
				   fsi.m_UsedK = ExtractWord(p);
				   fsi.m_AvailableK = ExtractWord(p);
				   fsi.m_PercentUsed = ExtractWord(p);
				   fsi.m_MountedOn = ExtractWordEscaped(p);
					 fsi.m_bIsReadOnly = IsReadOnly(fsi.m_MountedOn);
				   pFileSystemList->Add(fsi);
				}
				else
					bNotFirstLine = TRUE;
			}
		}

		pclose(f);
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL IsReadOnlyFileSystemPath(LPCSTR Path)
{
	int matchindex = -1;
	
	unsigned int matchlen=0;
	int i;

	for (i=0; i < gFileSystemList.count(); i++)
	{
		QString s = gFileSystemList[i].m_MountedOn;
		
		if (s.right(1) != "/")
			s += "/";

		unsigned int len = s.length();

		if (strlen(Path) > len && !strncmp(s, Path, len))
		{
			if (len > matchlen)
			{
				matchlen = len;
				matchindex = i;
			}
		}
	}

	return matchlen ? gFileSystemList[matchindex].m_bIsReadOnly : FALSE;
}

////////////////////////////////////////////////////////////////////////////


