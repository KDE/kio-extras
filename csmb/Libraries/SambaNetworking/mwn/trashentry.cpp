/* Name: trashentry.cpp

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

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h> // for access()
#include <dirent.h> // for opendir() etc.
#include "trashentry.h"
#include "copymove.h" 
#include <time.h>
#include "qmessagebox.h"
#include <sys/param.h> // for MAXPATHLEN
////////////////////////////////////////////////////////////////////////////

size_t CTrashEntryItem::GetFileSize() const
{
	return (size_t)m_FileSize;
}

////////////////////////////////////////////////////////////////////////////

void CTrashEntryItem::Init()
{
	int nIndex = m_OriginalLocation.findRev('/');
  setText(1, m_OriginalLocation.left(nIndex));
  setText(0, m_OriginalLocation.mid(nIndex+1, m_OriginalLocation.length()));
  setText(2, FormatDateTime(m_DateDeleted));
  QString FileAttributes;
  FileModeToString(FileAttributes, m_OriginalFileMode);
  setText(3, FileAttributes);
  setText(4, SizeInKilobytes((unsigned long)m_FileSize) + "  ");
	
  InitPixmap();
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CTrashEntryItem::text(int column) const
{
  switch (column)
  {
    case -1:
      return "MyComputer";

    case 0: // Original short filename
    case 1: // Original location
    case 2: // Date deleted
    case 3: // File attributes
    case 4: // File size
      return CListViewItem::text(column);
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////

QTKEYTYPE CTrashEntryItem::key(int nColumn, bool ascending) const
{
  static QString s;

  /* Name, Location, Date Deleted, Attributes, Size */

  if (nColumn == 4) /* Size */
  {
    if (IsFolder())
      s = QString("\1") + text(0);
    else
      s.sprintf("%.10ld_%s", m_FileSize, (LPCSTR)text(0));
  }
  else
  {
    if (nColumn == 2)
      s.sprintf("%.10ld_%s", m_DateDeleted, (LPCSTR)text(0));
    else
    {
      s = IsFolder() ? "\1" : "";
      s += text(nColumn);
    }
  }

  return (QTKEYTYPE)s;
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CTrashEntryItem::Pixmap(BOOL bIsBig)
{
  return DefaultFilePixmap(bIsBig, 
                           text(3), 
                           m_OriginalFileMode, 
                           m_FileName, 
                           IsLink(), 
                           IsFolder());
}

////////////////////////////////////////////////////////////////////////////

BOOL GetTrashEntryList(LPCSTR Path, CTrashEntryArray& list)
{
	QString FullName(Path);
			
	if (FullName.right(1) != "/")
		FullName += "/";
	
	if (access(Path, X_OK))
	{
		return TRUE; // no access!
	}
	
	DIR *thisDir = opendir(Path);
	
	if (thisDir == NULL)
	{
		//printf("Unable to opendir %s\n", Path);
		return FALSE;
	}

	struct dirent *p;

	while ((p = readdir(thisDir)) != NULL)
	{
    if (p->d_name[0] != '.' ||
        strlen(p->d_name) != 14 || 
        strcmp(p->d_name+9, ".info"))
      continue;
		
    FILE *f = fopen(FullName + p->d_name, "r");
    
    if (NULL == f)
      continue;
    
    CTrashEntryInfo tei;
    
    tei.m_FileName = FullName + QString(p->d_name).left(9);

    char linebuf[MAXPATHLEN+1];
    
    fgets(linebuf, sizeof(linebuf)-1, f); // Original location
    linebuf[strlen(linebuf)-1] = '\0';
    
    // Remove excessive slash

    if (strlen(linebuf) > 1 && linebuf[strlen(linebuf)-1] == '/')
      linebuf[strlen(linebuf)-1] = '\0';
    
    tei.m_OriginalLocation = linebuf;
    
    int nIndex = tei.m_OriginalLocation.findRev('/');

    tei.m_FileName += tei.m_OriginalLocation.mid(nIndex, tei.m_OriginalLocation.length());
    
    fgets(linebuf, sizeof(linebuf)-1, f); // Date deleted
    linebuf[strlen(linebuf)-1] = '\0';
    sscanf(linebuf, "%lx", (unsigned long *)&tei.m_DateDeleted);
    
    fgets(linebuf, sizeof(linebuf)-1, f); // File size
    linebuf[strlen(linebuf)-1] = '\0';
    sscanf(linebuf, "%lx", (unsigned long *)&tei.m_FileSize);
    
    fgets(linebuf, sizeof(linebuf)-1, f); // File mode
    linebuf[strlen(linebuf)-1] = '\0';
    sscanf(linebuf, "%lx", (unsigned long *)&tei.m_OriginalFileMode);
    
    fgets(linebuf, sizeof(linebuf)-1, f); // mtime
    linebuf[strlen(linebuf)-1] = '\0';
    sscanf(linebuf, "%lx", (unsigned long *)&tei.m_OriginalModifyDate);
    
    fgets(linebuf, sizeof(linebuf)-1, f); // ctime
  
    if (linebuf[strlen(linebuf)-1] == '\n')   
      linebuf[strlen(linebuf)-1] = '\0';

    sscanf(linebuf, "%lx", (unsigned long *)&tei.m_OriginalCreationDate);
    
    list.Add(tei);

    fclose(f);
	}

	closedir(thisDir);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CheckTrashFolder(QString Path, time_t SinceWhen, int nOldChildCount)
{
	BOOL retcode = FALSE;

  if (Path.right(1) != "/")
		Path += "/";
  
  //printf("CheckTrashFolder(%s)\n", (LPCSTR)Path);

  DIR *thisDir = opendir(Path);

  if (thisDir == NULL)
  {
    return TRUE;
  }

  struct dirent *p;

  int nCount=0;

  while ((p = readdir(thisDir)) != NULL)
  {
    if (p->d_name[0] != '.' ||
        strlen(p->d_name) != 14 || 
        strcmp(p->d_name+9, ".info"))
      continue;
    
    QString Folder = Path + QString(p->d_name).left(9) + "/";
    QString InfoFileName = Path + p->d_name;

    FILE *f = fopen(InfoFileName, "r");
    
    if (NULL == f)
      retcode = TRUE;
    else
    {
      char linebuf[MAXPATHLEN+1];
      fgets(linebuf, sizeof(linebuf)-1, f); // Original location
      fclose(f);
      linebuf[strlen(linebuf)-1] = '\0';
      
      if (linebuf[strlen(linebuf)-1] == '/')
        linebuf[strlen(linebuf)-1] = '\0';

      QString FileName = linebuf;
      int nIndex = FileName.findRev('/');
      FileName = FileName.mid(nIndex, FileName.length());
      
      struct stat st;

      if (lstat(Folder+FileName, &st) < 0)
      {
        retcode = TRUE;
        unlink(InfoFileName);
        rmdir(Folder);
      }

      nCount++;
    }
  }
	
  closedir(thisDir);

  if (nOldChildCount != -1 &&
      nCount != nOldChildCount)
  {
    //printf("nCount = %d, old count = %d\n", nCount, nOldChildCount);
    retcode = TRUE; 
  }

  if (!retcode)
  {
    struct stat st;

    if (stat(Path, &st))
      retcode = TRUE;
    else
    {
			time_t timenow = time(NULL);

      retcode = 
        (st.st_ctime > SinceWhen && st.st_ctime < timenow) ||
			  (st.st_mtime > SinceWhen && st.st_mtime < timenow);
    }
  }

  return retcode;
}

////////////////////////////////////////////////////////////////////////////

void EmptyTrash()
{
  QString TrashPath = GetHomeDir();

  if (TrashPath.right(1) != "/")
    TrashPath += "/";

  TrashPath += "Desktop/Trash/";

  DIR *thisDir = opendir(TrashPath);

  if (thisDir == NULL)
    return; // Oops...
  
  struct dirent *p;
	QStrList list;
  QString URL;
	
  while ((p = readdir(thisDir)) != NULL)
  {
    if (p->d_name[0] != '.' ||
        strlen(p->d_name) != 14 || 
        strcmp(p->d_name+9, ".info"))
      continue;
		
		MakeURL(TrashPath + p->d_name, NULL, URL);
    list.append(URL);

    QString s = TrashPath + QString(p->d_name).left(9);
    
    if (!access(s, W_OK))
    {
      MakeURL(s, NULL, URL);
      list.append(URL);
    }
  }

  closedir(thisDir);
  
  /* Display warning. This is a special case,
  so we don't use default warning mechanism */

  QString msg;
  
  BOOL bOneFile = (2 == list.count());

  if (bOneFile)
    msg = LoadString(knCONFIRM_ONE_ITEM_DELETE_QUESTION);
  else
    msg.sprintf(LoadString(knCONFIRM_MULTIPLE_FILE_DELETE_QUESTION), list.count() / 2);

  QMessageBox mb(LoadString(2 == list.count() ? knCONFIRM_FILE_DELETE : knCONFIRM_MULTIPLE_FILE_DELETE), 
                 (LPCSTR)msg, 
                 QMessageBox::Warning, 
                 QMessageBox::Yes | QMessageBox::Default, 
                 QMessageBox::No, 0);
  
  mb.setIconPixmap(*LoadPixmap(keConfirmDeletePermanentIcon));

  if (mb.exec() == QMessageBox::Yes)
    StartDelete(list, FALSE, TRUE); // nuke it, silently....
}

////////////////////////////////////////////////////////////////////////////

BOOL IsTrashEmpty()
{
  QString TrashPath = GetHomeDir();

  if (TrashPath.right(1) != "/")
    TrashPath += "/";

  TrashPath += "Desktop/Trash/";

  DIR *thisDir = opendir(TrashPath);

  if (NULL == thisDir)
    return TRUE; // Oops...
  
  struct dirent *p;
	QStrList list;
  QString URL;
	
  BOOL bEmpty = TRUE;

  while ((p = readdir(thisDir)) != NULL)
  {
    if (p->d_name[0] != '.' ||
        strlen(p->d_name) != 14 || 
        strcmp(p->d_name+9, ".info"))
      continue;
	  	
    bEmpty = FALSE;
    break;
  }

  closedir(thisDir);
    
  return bEmpty;
}
