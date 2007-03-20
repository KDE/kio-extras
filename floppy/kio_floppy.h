/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KIO_FLOPPY_H
#define KIO_FLOPPY_H

#include <kio/slavebase.h>
#include <kio/global.h>

#include "program.h"

#include <QString>

struct StatInfo
{
   StatInfo():name(""),time(0),size(0),mode(0),freeSpace(0),isDir(false),isValid(false) {;}
   QString name;
   time_t time;
   int size;
   int mode;
   int freeSpace;
   bool isDir:1;
   bool isValid:1;
};


class FloppyProtocol : public KIO::SlaveBase
{
   public:
      FloppyProtocol (const QByteArray &pool, const QByteArray &app );
      virtual ~FloppyProtocol();

      virtual void listDir( const KUrl& url);
      virtual void stat( const KUrl & url);
      virtual void mkdir( const KUrl& url, int);
      virtual void del( const KUrl& url, bool isfile);
      virtual void rename(const KUrl &src, const KUrl &dest, bool overwrite);
      virtual void get( const KUrl& url );
      virtual void put( const KUrl& url, int _mode,bool overwrite, bool _resume );
      //virtual void copy( const KUrl& src, const KUrl &dest, int, bool overwrite );
   protected:
      Program *m_mtool;
      int readStdout();
      int readStderr();

      StatInfo createStatInfo(const QString line, bool makeStat=false, const QString& dirName="");
      void createUDSEntry(const StatInfo& info, KIO::UDSEntry& entry);
      StatInfo _stat(const KUrl& _url);
      int freeSpace(const KUrl& url);

      bool stopAfterError(const KUrl& url, const QString& drive);
      void errorMissingMToolsProgram(const QString& name);

      void clearBuffers();
      void terminateBuffers();
      char *m_stdoutBuffer;
      char *m_stderrBuffer;
      int m_stdoutSize;
      int m_stderrSize;
};

#endif
