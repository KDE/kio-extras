/* Name: fileUNC.h

   Description: This file is a part of the libmwn library.

   Author:	Chris Ellison

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


#ifndef _FILEUNC_H
#define _FILEUNC_H

#include <dirent.h>

#ifndef NOCPROTO
#include <stdio.h>

extern "C"
{
	int U_Stat1(const char *pathname, struct stat *buf);
	int U_Stat2(int ver, const char *pathname, struct stat *buf);
	int U_Lstat1(const char *pathname, struct stat *buf);
	int U_Lstat2(int ver, const char* pathname, struct stat *buf);
	int U_Remove(const char *pathname);
	DIR *U_Opendir(const char *pathname);
	int U_Closedir(DIR *dp);
	FILE *U_Fopen(const char *pathname, const char *type);
	FILE *U_Freopen(const char *pathname, const char *type, FILE *fp);
	int U_Fclose(FILE *fp);
}
#endif /* NOCPROTO */

#ifdef __cplusplus

//CFdUncListEntry

class CFdUncListEntry
{
public:
	CFdUncListEntry(unsigned long ulKey, const char* szUNC);
	~CFdUncListEntry();
	unsigned long GetKey();
	const char* GetUNC();
private:
	unsigned long m_ulKey;
	const char*m_szUNC;
};

#ifdef QFile
#undef QFile
#endif

#include "qfile.h"

class U_QFile : public QFile
{
public:

	U_QFile(const char *UNC) : QFile()
	{
		setName(UNC);
	}

	U_QFile() : QFile() {}

	~U_QFile();

	bool U_Open(int);
	bool U_Open(int, FILE *);
	bool U_Open(int, int);
	void U_Close();
	void setName(const char *name);

	bool exists() const;

	static bool exists(const char *fileName);

	static bool remove(const char *fileName);

private:
	QString m_UNCPath;
};

#define QFile U_QFile

#ifdef QFileInfo
#undef QFileInfo
#endif

#ifndef NO_QFILEINFO

#include "qfileinfo.h"

class U_QFileInfo : public QFileInfo
{
public:
	U_QFileInfo() : QFileInfo()
	{
	}

  U_QFileInfo(const char *fileName) : QFileInfo()
	{
		setFile(fileName);
	}

	U_QFileInfo(const QFile &f) : QFileInfo(f)
	{
	}

	~U_QFileInfo();

	U_QFileInfo(const QDir &d, const char *fileName) :
		QFileInfo(d, fileName) {}

	U_QFileInfo( const QFileInfo &fi) : QFileInfo(fi)
	{
	}

#ifdef QT_20
  void setFile(const QString&);
#else
	void	setFile(const char *);
#endif

private:
	QString m_UNCPath;
};

//#include "qlist.h"

//typedef QList<U_QFileInfo> U_QFileInfoList;
//typedef QListIterator<U_QFileInfo> U_QFileInfoListIterator;

#define QFileInfo U_QFileInfo
#endif /* NO_QFILEINFO */

#if (QT_VERSION < 200)
#ifdef KURL
#undef KURL
#endif
#endif

#ifndef KURL_H

#include "kurl.h"

#if (QT_VERSION < 200)
class CCorelURL : public KURL
{
public:

	CCorelURL() : KURL()
	{
	}

	CCorelURL( const char* _url) : KURL(FixURL(_url))
	{
		if (strlen(path()) > 2 && !strncmp(path(), "/\\\\", 3))
		{
			QString NewPath(path()+1);
      setPath(NewPath);
		}

		/*	const char *thishost = host();
		if (strlen(thishost) > 2 && thishost[0] == '\\' && thishost[1] == '\\')
		{
      //QString MakeSlashesForward(const char *);

			//setPath(MakeSlashesForward(host()));
			setPath(host());
			bNoPath = false;
			setHost("");
		}
		*/
	}

	static QString FixURL(const char *_url)
	{
		if (strlen(_url) > 2 && _url[0] == '\\' && _url[1] == '\\')
		{
			return QString("file://localhost/") + _url;
		}
		else
			return QString(_url);
	}

#if (QT_VERSION < 200)
  QString url() const
	{
		QString url = protocol_part.copy();

		if( !host_part.isEmpty())
		{
			url += "://";

			if ( !user_part.isEmpty() )
			{
				url += user_part.data();

				if (!passwd_part.isEmpty())
				{
					url += ":";
					url += passwd_part.data();
				}

				url += "@";
			}

			url += host_part;

			if ( port_number != 0 )
			{
				QString tmp(url.data());
				url.sprintf("%s:%d",tmp.data(),port_number);
			}
		}
		else
			url += "://";

		if(!path_part.isEmpty() && hasPath())
		{
			if (path_part[0] != '/')
			{
				url += "/";
			}

			url += path_part;
		}

		if( !search_part.isNull())
		{
			if(path_part.isEmpty() || !hasPath() )
				url += "/";

			url += "?" + search_part;
		}

		if( !ref_part.isEmpty() )
		{
			if(path_part.isEmpty() || !hasPath() )
				url += "/";
			url += "#" + ref_part;
		}

		return url;
	}
#endif

	~CCorelURL()
	{
	}

#if (QT_VERSION < 200)
	CCorelURL(const char* _protocol, const char* _host, const char* _path, const char* _ref) :
		KURL(_protocol, _host, _path, _ref)
	{
	}
#endif

	CCorelURL(CCorelURL & _base_url, const char* _rel_url) :
		KURL(_base_url, _rel_url)
	{
	}

	CCorelURL &operator=(const KURL &other)
	{
		*((KURL*)this) = other;
		return *this;
	}

	CCorelURL &operator=(const char* _url)
	{
		*((KURL*)this) = _url;
		return *this;
	}

	bool operator==(const KURL &_url) const
	{
		return *((KURL*)this) == _url;
	}

	bool isLocalFile();
};

#define KURL CCorelURL
#endif
#endif

#ifndef __KFILEDIALOG_H__
#include "kfiledialog.h"
#endif
#endif /* __cplusplus */

#endif	/* _FILEUNC_H */
