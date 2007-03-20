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

#ifndef KIO_NFS_H
#define KIO_NFS_H

#include <kio/slavebase.h>
#include <kio/global.h>

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#define PORTMAP  //this seems to be required to compile on Solaris
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

class NFSFileHandle
{
   public:
      NFSFileHandle();
      NFSFileHandle(const NFSFileHandle & handle);
      ~NFSFileHandle();
      NFSFileHandle& operator= (const NFSFileHandle& src);
      NFSFileHandle& operator= (const char* src);
      operator const char* () const {return m_handle;}
      bool isInvalid() const {return m_isInvalid;}
      void setInvalid() {m_isInvalid=true;}
//      time_t age() const;
   protected:
      char m_handle[NFS_FHSIZE+1];
      bool m_isInvalid;
//      time_t m_detectTime;
};

//ostream& operator<<(ostream&, const NFSFileHandle&);

typedef QMap<QString,NFSFileHandle> NFSFileHandleMap;


class NFSProtocol : public KIO::SlaveBase
{
   public:
      NFSProtocol (const QByteArray &pool, const QByteArray &app );
      virtual ~NFSProtocol();

      virtual void openConnection();
      virtual void closeConnection();

      virtual void setHost( const QString& host, int port, const QString& user, const QString& pass );

      virtual void put( const KUrl& url, int _mode,bool _overwrite, bool _resume );
      virtual void get( const KUrl& url );
      virtual void listDir( const KUrl& url);
      virtual void symlink( const QString &target, const KUrl &dest, bool );
      virtual void stat( const KUrl & url);
      virtual void mkdir( const KUrl& url, int permissions );
      virtual void del( const KUrl& url, bool isfile);
      virtual void chmod(const KUrl& url, int permissions );
      virtual void rename(const KUrl &src, const KUrl &dest, bool overwrite);
      virtual void copy( const KUrl& src, const KUrl &dest, int mode, bool overwrite );
   protected:
//      void createVirtualDirEntry(KIO::UDSEntry & entry);
      bool checkForError(int clientStat, int nfsStat, const QString& text);
      bool isExportedDir(const QString& path);
      void completeUDSEntry(KIO::UDSEntry& entry, fattr& attributes);
      void completeBadLinkUDSEntry(KIO::UDSEntry& entry, fattr& attributes);
      void completeAbsoluteLinkUDSEntry(KIO::UDSEntry& entry, const QByteArray& path);
      bool isValidLink(const QString& parentDir, const QString& linkDest);
//      bool isAbsoluteLink(const QString& path);
      
      NFSFileHandle getFileHandle(QString path);

      NFSFileHandleMap m_handleCache;
      QHash<long, QString> m_usercache;      // maps long ==> QString *
      QHash<long, QString> m_groupcache;

      QStringList m_exportedDirs;
      QString m_currentHost;
      CLIENT *m_client;
      CLIENT *m_nfsClient;
      timeval total_timeout;
      timeval pertry_timeout;
      int m_sock;
      time_t m_lastCheck;
      void checkForOldFHs();
};

#endif
