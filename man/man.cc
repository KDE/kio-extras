/*
 *  man.cpp - part of the KDE Help Center
 *
 *  Copyright (c) 1999 Matthias Elter (me@kde.org)
 *
 *  based on kdehelp code (c) Martin R. Jones
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_TMP
#define _PATH_TMP "/tmp"
#endif

#include "man.h"

#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qdir.h>
#include <qfile.h>

static ManSection *sections[MAN_MAXSECTIONS];
static int numSections = 0;

// maintain a list of man pages available in a section
QString ManSection::searchPath[MAN_MAXPATHS];
int ManSection::numPaths = 0;
int ManSection::sectCount = 0;

ManSection::ManSection(const QString& name)
{
    m_name = name;
    m_numPages = 0;
    m_isRead = 0;

    m_pages.setAutoDelete(true);

    sectCount++;

    if (m_name == "1")
	m_description = i18n("User Commands");
    else if (m_name == "2")
	m_description = i18n("System Calls");
    else if (m_name == "3")
	m_description = i18n("Subroutines");
    else if (m_name == "4")
	m_description = i18n("Devices");
    else if (m_name == "5")
	m_description = i18n("File Formats");
    else if (m_name == "6")
	m_description = i18n("Games");
    else if (m_name == "7")
	m_description = i18n("Miscellaneous");
    else if (m_name == "8")
	m_description = i18n("System Administration");
    else if (m_name == "9")
	m_description = i18n("Kernel");
    else if (m_name == "n")
	m_description = i18n("New");
    else
	m_description = "";

    // setup man search path
    if (numPaths == 0)
    {
	if (QString envPath = getenv(MAN_ENV))
        {
	    QString p;
	    int count = 0;
	    while (!envPath.isEmpty() || count > MAN_MAXPATHS)
	    {
		count++;
		
		p = envPath.left(envPath.find(":"));
		searchPath[numPaths++] = p;
				
		if (envPath.contains(":") > 0)
		    envPath = envPath.remove(0, envPath.find(":")+1);
		else 
		    envPath += ":";
	    }
        }
	else
        {
#ifdef _PATH_MAN
	    searchPath[numPaths++] = _PATH_MAN;
#else
	    searchPath[numPaths++] = "/usr/man";
#endif
	    searchPath[numPaths++] = "/usr/X11R6/man";
	    searchPath[numPaths++] = "/usr/local/man";
        }
    }
}

ManSection::~ManSection()
{
    sectCount--;
    m_pages.clear();
}

// read a list of pages in the section
void ManSection::readSection()
{
    if (!m_isRead)	// only read once
	m_isRead = true;
    else
	return;

    for (int i = 0; i < numPaths; i++)
    {
	QDir dir( searchPath[i], "*", 0, QDir::Dirs );

	if (!dir.exists()) continue;
  
	QStringList dirList = dir.entryList();
	QStringList::Iterator itDir;

	for (itDir = dirList.begin(); !(*itDir).isNull(); ++itDir)
	{
	    if ( (*itDir).at(0) == '.' )
		continue;

	    QString folderName = *itDir;
	    if (folderName.contains("man") > 0)
	    {
		if (folderName.find(m_name) > 2)
		{
		    QString buffer = searchPath[i];
		    buffer += '/';
		    buffer += folderName;
		    readDir(buffer);
		}
	    }
	    else if (folderName.contains("cat") > 0)
	    {
		if (folderName.find(m_name) > 2)
		{
		    QString buffer = searchPath[i];
		    buffer += '/';
		    buffer += folderName;
		    readDir(buffer);
		}
	    }
	}
    }
}

// read the contents of a directory and add to the list of pages
void ManSection::readDir(const QString& dirName )
{
    QDir fileDir(dirName, "*", 0, QDir::Files | QDir::Hidden | QDir::Readable);

    if (!fileDir.exists()) return;

    // does dir contain files
    if (fileDir.count() > 0)
    {
	QStringList fileList = fileDir.entryList();
	QStringList::Iterator itFile;
	for (itFile = fileList.begin(); !(*itFile).isNull(); ++itFile)
	{
	    QString fileName = *itFile;
	    QString file = dirName;
	    file += '/';
	    file += *itFile;
	    
	    // skip compress extension
	    if (fileName.right(3) == ".gz")
            {
		fileName.truncate(fileName.length()-3);
            }
	    else if (fileName.right(2) == ".Z") 
	    {
		fileName.truncate(fileName.length()-2);
	    }

	    if (!fileName.isEmpty())
            {
		fileName.truncate(fileName.findRev("."));
		ManPage *page = new ManPage(fileName, file);
		m_pages.append(page);
            }
        }
    }
}

int ManParser::instance = 0;

ManParser::ManParser()
{
    QString sectList = getenv("MANSECT");
  
    if (sectList.isEmpty())
	sectList = MAN_SECTIONS;
  
    // create the sections
    if (instance == 0)
    {
	numSections = 0;

	QString s;
	int count = 0;
	while (!sectList.isEmpty() || count > MAN_MAXSECTIONS)
	{
	    count++;

	    s = sectList.left(sectList.find(":"));
	    sections[numSections++] = new ManSection(s);
	   
	    if (sectList.contains(":") > 0)
		sectList = sectList.remove(0, sectList.find(":")+1);
	    else 
		sectList += ":";
	}
    }
    instance++;
}

ManParser::~ManParser()
{
    instance--;
    if (instance == 0)
    {
	for (int i = 0; i < numSections; i++)
	    delete sections[i];
    }
}

// read the specified page
// -----------------------
// formats allowed:
// (sec)		reads a list of pages in 'sections'
// page(sec)		reads the specified page from 'sections'
// page			reads the specified page
//
int ManParser::readLocation(const QString& name)
{
   QString tmpName = name;
  
   if (tmpName.at(0) == '(')				// read a list of pages in this section
   {
	QString sec = tmpName.mid(1, tmpName.findRev(")") -1);
	for (int i = 0; i < numSections; i++)
	{
	    if (sections[i]->name() == sec)
	    {
		sections[i]->readSection();
		m_page = "";

		for (ManPage *page = sections[i]->pages()->first(); page != 0; page = sections[i]->pages()->next())
		{
		    QString buffer;
		
		    buffer = page->name();
		    buffer += "(";
		    buffer += sections[i]->name();
		    buffer += ")";
		    m_page += "<cell width=200>&nbsp;";
		    m_page += "<A HREF=man:/" + buffer + ">";
		    m_page += page->name();
		    m_page += "</A>";
		    m_page += "</cell>";
		} 
		m_location = i18n("Unix man pages - Section ");
		m_location += sections[i]->name();
		if (!sections[i]->description().isEmpty())
		{
		    m_location += " - ";
		    m_location += sections[i]->description();
		}
		return 0;
	    }
	}
	return 1;
    }
   else
   {
       m_location = "Error: Invalid URL";
       m_page = "<br>Could not find or access URL: man:/" + name;
   }
    /*else									// read the specified page
    {
	char stdFile[256];
	char errFile[256];
	char sysCmd[256];
	char rmanCmd[256];
	char *ptr;
	  
	sysCmd[0] = '\0';
	  
	sprintf(stdFile, "%s/khelpcenterXXXXXX", _PATH_TMP);	// temp file
	mktemp(stdFile);
	  
	sprintf(errFile, "%s/khelpcenterXXXXXX", _PATH_TMP);	// temp file
	mktemp(errFile);
	  
	sprintf(rmanCmd, "%s/rman -f HTML", locate("exe", "rman"));
debug("==> rmanCmd = %s", rmanCmd);
	  
	// create the system cmd to read the man page
	if ( (ptr = strchr(tmpName, '(')) )
	{
	    if (!strchr(tmpName, ')')) return 1;	// check for closing )
	    *ptr = '\0';
	    ptr++;
	    for (i = 0; i < numSections; i++)	// read which section?
	    {
		if (!strncmp(ptr, sections[i]->getName(),
			     strlen(sections[i]->getName())))
		{
		    pos = i;
		    break;
		}
	    }
	    if ( safeCommand( sections[pos]->getName() ) &&
		 safeCommand( tmpName ) )
            {
		sprintf(sysCmd, "man %s %s < /dev/null 2> %s | %s > %s",
			sections[pos]->getName(),
			tmpName, errFile, rmanCmd, stdFile );
            }
	}
	else if ( safeCommand( tmpName ) )
	{
	    sprintf(sysCmd, "man %s < /dev/null 2> %s | %s > %s",
		    tmpName, errFile, rmanCmd, stdFile);
	}
	  
	if ( sysCmd[0] == '\0' )
        {
	    //Error.Set(ERR_WARNING, i18n("\"man\" system call failed"));
	    return -1;
        }
	  
	// call 'man' to read man page
	int status = system(sysCmd);
	  
	if (status < 0)			// system returns -ve on failure
	{
	    //Error.Set(ERR_WARNING, i18n("\"man\" system call failed"));
	    return 1;
	}
	  
	// open the man page and parse it
	ifstream stream(stdFile);
	  
	if (stream.fail())
	{
	    //Error.Set(ERR_FATAL, i18n("Opening temporary file failed"));
	    return 1;
	}
	  
	// if this file is very short assume the man call failed
	stream.seekg( 0, ios::end );
	if ( stream.tellg() < 5 )
	{
	    stream.close();
	    stream.open( errFile );
	}
	stream.seekg( 0, ios::beg );
	  
	char buffer[256];
	HTMLPage = "";
	  
	while ( !stream.eof() )
	{
	    stream.getline( buffer, 256 );
	    HTMLPage += buffer;
	    if ( HTMLPage.at(HTMLPage.length() - 1) == '-' )
		HTMLPage.truncate( HTMLPage.length() - 1 );
	    else
		HTMLPage.append(' ');
	}
	  
	stream.close();
	posString = name;
	  
	remove(stdFile);	// don't need tmp file anymore
	remove(errFile);	// don't need tmp file anymore
	}*/
    return 0;
}
