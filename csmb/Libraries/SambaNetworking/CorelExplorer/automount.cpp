/* Name: automount.cpp

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

#include "common.h"
#include "automount.h"
#include <sys/types.h>
#include <sys/stat.h>

CAutoMountList gAutoMountList;

BOOL ReadAutoMountList(CAutoMountList &list)
{
  FILE *f = fopen("/etc/auto.master", "r");


  if (NULL == f)
    return FALSE; // automount not installed

  char buf[100];

  while (!feof(f))
  {
    fgets(buf, sizeof(buf)-1, f);
    if (feof(f))
      break;

    if (buf[0] == '#')
      continue; // comment found

    LPCSTR p = &buf[0];
    QString BaseLocation = ExtractWord(p);

    if (BaseLocation.right(1) != "/")
      BaseLocation += "/";

    QString DescFile = ExtractWord(p);

    FILE *f2 = fopen((LPCSTR)DescFile
#ifdef QT_20
  .latin1()
#endif
    , "r");

    if (NULL != f2)
    {
      char buf2[100];

      while (!feof(f2))
      {
        fgets(buf2, sizeof(buf2)-1, f2);

        if (feof(f2))
          break;

        if (buf2[0] == '#')
          continue; // comment found

        LPCSTR p2 = &buf2[0];

        CAutoMountEntry *e = new CAutoMountEntry;

        QString SubdirectoryName = ExtractWord(p2);

        e->m_MountLocation = BaseLocation + SubdirectoryName;

        ExtractWord(p2, ":"); // skip until ":" is found

        if (*p2 == ':')
          p2++;

        e->m_Device = ExtractWord(p2);

        //printf("Device = %s, MountLocation = %s\n", (LPCSTR)e->m_Device, (LPCSTR)e->m_MountLocation);
        list.append(e);
      }

      fclose(f2);
    }
  }

  fclose(f);

  return TRUE;
}

QString FindAutoMountEntry(const CAutoMountList &list, LPCSTR Device)
{
  QString result;

  // Step1: find (major,minor) or the device in question
  struct stat st;

  if (!stat(Device, &st))
  {
    dev_t Dev = st.st_rdev;

    QListIterator<CAutoMountEntry> it(list);

    for (it.toFirst(); NULL != it.current(); ++it)
    {
      if (!stat((LPCSTR)(it.current()->m_Device
#ifdef QT_20
  .latin1()
#endif

      ), &st) &&
          st.st_rdev == Dev)
      {
        result = it.current()->m_MountLocation;
        break;
      }
    }
  }

  return result;
}

