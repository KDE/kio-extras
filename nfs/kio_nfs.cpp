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

#include "kio_nfs.h"

#include <config-runtime.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <arpa/inet.h>

// This is needed on Solaris so that rpc.h defines clnttcp_create etc.
#ifndef PORTMAP
#define PORTMAP
#endif
#include <rpc/rpc.h> // for rpc calls

#include <errno.h>
#include <grp.h>
#include <memory.h>
#include <netdb.h>
#include <pwd.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QLoggingCategory>

#include <KLocalizedString>
#include <kio/global.h>
#include <iostream>

#define fhandle _fhandle
#include "mount.h"

#define MAXHOSTLEN 256

//#define MAXFHAGE 60*15   //15 minutes maximum age for file handles

//this ioslave is for NFS version 2
#define NFSPROG ((u_long)100003)
#define NFSVERS ((u_long)2)

using namespace KIO;
using namespace std;

Q_DECLARE_LOGGING_CATEGORY(LOG_KIO_NFS)
Q_LOGGING_CATEGORY(LOG_KIO_NFS, "kde.kio-nfs")


extern "C" { int Q_DECL_EXPORT kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_nfs protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }
  qCDebug(LOG_KIO_NFS) << "NFS: kdemain: starting";

  NFSProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  return 0;
}

static bool isRoot(const QString& path)
{
   return (path.isEmpty() || (path=="/"));
}

static bool isAbsoluteLink(const QString& path)
{
   //hmm, don't know
   if (path.isEmpty()) return true;
   if (path[0]=='/') return true;
   return false;
}

static void createVirtualDirEntry(UDSEntry & entry)
{
   entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
   entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
   entry.insert( KIO::UDSEntry::UDS_USER, QString::fromLatin1("root") );
   entry.insert( KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root") );
   //a dummy size
   entry.insert( KIO::UDSEntry::UDS_SIZE, 1024 );
}


static void stripTrailingSlash(QString& path)
{
   //if (path=="/") return;
   if (path == QLatin1String("/"))
       path = "";
   else if (path.endsWith(QLatin1Char('/')))
       path.truncate(path.length()-1);
}

static void getLastPart(const QString& path, QString& lastPart, QString& rest)
{
   int slashPos=path.lastIndexOf('/');
   lastPart=path.mid(slashPos+1);
   rest=path.left(slashPos+1);
}

static QString removeFirstPart(const QString& path)
{
   QString result("");
   if (path.isEmpty()) return result;
   result=path.mid(1);
   int slashPos=result.indexOf('/');
   return result.mid(slashPos+1);
}

NFSFileHandle::NFSFileHandle()
:m_isInvalid(false)
{
   memset(m_handle,'\0',NFS_FHSIZE+1);
//   m_detectTime=time(0);
}

NFSFileHandle::NFSFileHandle(const NFSFileHandle & handle)
:m_isInvalid(false)
{
   m_handle[NFS_FHSIZE]='\0';
   memcpy(m_handle,handle.m_handle,NFS_FHSIZE);
   m_isInvalid=handle.m_isInvalid;
//   m_detectTime=handle.m_detectTime;
}

NFSFileHandle::~NFSFileHandle()
{}

NFSFileHandle& NFSFileHandle::operator= (const NFSFileHandle& src)
{
   memcpy(m_handle,src.m_handle,NFS_FHSIZE);
   m_isInvalid=src.m_isInvalid;
//   m_detectTime=src.m_detectTime;
   return *this;
}

NFSFileHandle& NFSFileHandle::operator= (const char* src)
{
   if (src==0)
   {
      m_isInvalid=true;
      return *this;
   };
   memcpy(m_handle,src,NFS_FHSIZE);
   m_isInvalid=false;
//   m_detectTime=time(0);
   return *this;
}

/*time_t NFSFileHandle::age() const
{
   return (time(0)-m_detectTime);
}*/


NFSProtocol::NFSProtocol (const QByteArray &pool, const QByteArray &app )
:SlaveBase( "nfs", pool, app )
,m_client(0)
,m_sock(-1)
,m_lastCheck(time(0))
{
   qCDebug(LOG_KIO_NFS)<<"NFS::NFS: -"<<pool<<"-";
}

NFSProtocol::~NFSProtocol()
{
   closeConnection();
}

/*This one is currently unused, so it could be removed.
 The intention was to keep handles around, and from time to time
 remove handles which are too old. Alex
 */
/*void NFSProtocol::checkForOldFHs()
{
   qCDebug(LOG_KIO_NFS)<<"checking for fhs older than "<<MAXFHAGE;
   qCDebug(LOG_KIO_NFS)<<"current items: "<<m_handleCache.count();
   NFSFileHandleMap::Iterator it=m_handleCache.begin();
   NFSFileHandleMap::Iterator lastIt=it;
   while (it!=m_handleCache.end())
   {
      qCDebug(LOG_KIO_NFS)<<it.data().age()<<flush;
      if (it.data().age()>MAXFHAGE)
      {
         qCDebug(LOG_KIO_NFS)<<"removing";
         m_handleCache.remove(it);
         if (it==lastIt)
         {
            it=m_handleCache.begin();
            lastIt=it;
         }
         else
            it=lastIt;
      }
      lastIt=it;
      it++;
   };
   qCDebug(LOG_KIO_NFS)<<"left items: "<<m_handleCache.count();
   m_lastCheck=time(0);
}*/

void NFSProtocol::closeConnection()
{
    ::close(m_sock);
   m_sock=-1;
   if (m_client==0) return;
   CLNT_DESTROY(m_client);

   m_client=0;
}

bool NFSProtocol::isExportedDir(const QString& path)
{
   return m_exportedDirs.contains(path.mid(1));
}

/* This one works recursive.
 It tries to get the file handle out of the file handle cache.
 If this doesn't succeed, it needs to do a nfs rpc call
 in order to obtain one.
 */
NFSFileHandle NFSProtocol::getFileHandle(QString path)
{
   if (m_client==0) openConnection();

   //I'm not sure if this is useful
   //if ((time(0)-m_lastCheck)>MAXFHAGE) checkForOldFHs();

   stripTrailingSlash(path);
   qCDebug(LOG_KIO_NFS)<<"getting FH for -"<<path<<"-";
   //now the path looks like "/root/some/dir" or "" if it was "/"
   NFSFileHandle parentFH;
   //we didn't find it
   if (path.isEmpty())
   {
      qCDebug(LOG_KIO_NFS)<<"path is empty, invalidating the FH";
      parentFH.setInvalid();
      return parentFH;
   }
   //check whether we have this filehandle cached
   //the filehandles of the exported root dirs are always in the cache
   if (m_handleCache.find(path)!=m_handleCache.end())
   {
      qCDebug(LOG_KIO_NFS)<<"path is in the cache, returning the FH -"<<m_handleCache[path]<<"-";
      return m_handleCache[path];
   }
   QString rest, lastPart;
   getLastPart(path,lastPart,rest);
   qCDebug(LOG_KIO_NFS)<<"splitting path into rest -"<<rest<<"- and lastPart -"<<lastPart<<"-";

   parentFH=getFileHandle(rest);
   //f*ck, it's invalid
   if (parentFH.isInvalid())
   {
      qCDebug(LOG_KIO_NFS)<<"the parent FH is invalid";
      return parentFH;
   }
   // do the rpc call
   diropargs dirargs;
   diropres dirres;
   memcpy(dirargs.dir.data,(const char*)parentFH,NFS_FHSIZE);
   QByteArray tmpStr=QFile::encodeName(lastPart);
   dirargs.name=tmpStr.data();

   //cerr<<"calling rpc: FH: -"<<parentFH<<"- with name -"<<dirargs.name<<"-"<<endl;

   int clnt_stat = clnt_call(m_client, NFSPROC_LOOKUP,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);

   if ((clnt_stat!=RPC_SUCCESS) || (dirres.status!=NFS_OK))
   {
      //we failed
      qCDebug(LOG_KIO_NFS)<<"lookup of filehandle failed";
      parentFH.setInvalid();
      return parentFH;
   }
   //everything went fine up to now :-)
   parentFH=dirres.diropres_u.diropres.file.data;
   //qCDebug(LOG_KIO_NFS)<<"filesize: "<<dirres.diropres_u.diropres.attributes.size;
   m_handleCache.insert(path,parentFH);
   qCDebug(LOG_KIO_NFS)<<"return FH -"<<parentFH<<"-";
   return parentFH;
}

/* Open connection connects to the mount daemon on the server side.
 In order to do this it needs authentication and calls auth_unix_create().
 Then it asks the mount daemon for the exported shares. Then it tries
 to mount all these shares. If this succeeded for at least one of them,
 a client for the nfs daemon is created.
 */
void NFSProtocol::openConnection()
{
   qCDebug(LOG_KIO_NFS)<<"NFS::openConnection for" << m_currentHost;
   if (m_currentHost.isEmpty())
   {
      error(ERR_UNKNOWN_HOST, QString());
      return;
   }
   struct sockaddr_in server_addr;
   if (m_currentHost[0] >= '0' && m_currentHost[0] <= '9')
   {
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = inet_addr(m_currentHost.toLatin1());
   }
   else
   {
      struct hostent *hp=gethostbyname(m_currentHost.toLatin1());
      if (hp==0)
      {
         error(ERR_UNKNOWN_HOST, m_currentHost);
         return;
      }
      server_addr.sin_family = AF_INET;
      memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
   }

   // create mount daemon client
   closeConnection();
   server_addr.sin_port = 0;
   m_sock = RPC_ANYSOCK;
   m_client=clnttcp_create(&server_addr,MOUNTPROG, MOUNTVERS, &m_sock, 0, 0);
   if (m_client==0)
   {
      server_addr.sin_port = 0;
      m_sock = RPC_ANYSOCK;
      pertry_timeout.tv_sec = 3;
      pertry_timeout.tv_usec = 0;
      m_client = clntudp_create(&server_addr,MOUNTPROG, MOUNTVERS, pertry_timeout, &m_sock);
      if (m_client==0)
      {
         clnt_pcreateerror(const_cast<char *>("mount clntudp_create"));
         error(ERR_COULD_NOT_CONNECT, m_currentHost);
         return;
      }
   }
   QString hostName = QHostInfo::localHostName();
   QString domainName = QHostInfo::localDomainName();
   if (!domainName.isEmpty()) {
      hostName = hostName + QLatin1Char('.') + domainName;
   }
   qCDebug(LOG_KIO_NFS) << "hostname is -" << hostName << "-";
   m_client->cl_auth = authunix_create(hostName.toUtf8().data(), geteuid(), getegid(), 0, 0);
   total_timeout.tv_sec = 20;
   total_timeout.tv_usec = 0;

   exports exportlist;
   //now do the stuff
   memset(&exportlist, '\0', sizeof(exportlist));

   int clnt_stat = clnt_call(m_client, MOUNTPROC_EXPORT,(xdrproc_t) xdr_void, NULL,
                         (xdrproc_t) xdr_exports, (char*)&exportlist,total_timeout);
   if (!checkForError(clnt_stat, 0, m_currentHost.toLatin1())) return;

   fhstatus fhStatus;
   bool atLeastOnceSucceeded(false);
   for(; exportlist!=0;exportlist = exportlist->ex_next) {
      qCDebug(LOG_KIO_NFS) << "found export: " << exportlist->ex_dir;

      memset(&fhStatus, 0, sizeof(fhStatus));
      clnt_stat = clnt_call(m_client, MOUNTPROC_MNT,(xdrproc_t) xdr_dirpath, (char*)(&(exportlist->ex_dir)),
                            (xdrproc_t) xdr_fhstatus,(char*) &fhStatus,total_timeout);
      if (fhStatus.fhs_status==0) {
         atLeastOnceSucceeded=true;
         NFSFileHandle fh;
         fh=fhStatus.fhstatus_u.fhs_fhandle;
         QString fname;
         if ( exportlist->ex_dir[0] == '/' )
            fname = exportlist->ex_dir + 1;
         else
            fname = exportlist->ex_dir;
         m_handleCache.insert(QString("/")+fname,fh);
         m_exportedDirs.append(fname);
         // kDebug() <<"appending file -"<<fname<<"- with FH: -"<<fhStatus.fhstatus_u.fhs_fhandle<<"-";
      }
   }
   if (!atLeastOnceSucceeded)
   {
      closeConnection();
      error(ERR_COULD_NOT_AUTHENTICATE, m_currentHost);
      return;
   }
   server_addr.sin_port = 0;

   //now create the client for the nfs daemon
   //first get rid of the old one
   closeConnection();
   m_sock = RPC_ANYSOCK;
   m_client = clnttcp_create(&server_addr,NFSPROG,NFSVERS,&m_sock,0,0);
   if (m_client == 0)
   {
      server_addr.sin_port = 0;
      m_sock = RPC_ANYSOCK;
      pertry_timeout.tv_sec = 3;
      pertry_timeout.tv_usec = 0;
      m_client = clntudp_create(&server_addr,NFSPROG, NFSVERS, pertry_timeout, &m_sock);
      if (m_client==0)
      {
         clnt_pcreateerror(const_cast<char *>("NFS clntudp_create"));
         error(ERR_COULD_NOT_CONNECT, m_currentHost);
         return;
      }
   }
   m_client->cl_auth = authunix_create(hostName.toUtf8().data(),geteuid(),getegid(),0,0);
   connected();
   qCDebug(LOG_KIO_NFS)<<"openConnection succeeded";
}

void NFSProtocol::listDir( const QUrl& _url)
{
   QUrl url(_url);
   QString path( url.path());

   if (path.isEmpty())
   {
      url.setPath("/");
      redirection(url);
      finished();
      return;
   }
   //open the connection
   if (m_client==0) openConnection();
   //it failed
   if (m_client==0) return;
   if (isRoot(path))
   {
      qCDebug(LOG_KIO_NFS)<<"listing root";
      totalSize( m_exportedDirs.count());
      //in this case we don't need to do a real listdir
      UDSEntry entry;
      for (QStringList::const_iterator it=m_exportedDirs.constBegin(); it!=m_exportedDirs.constEnd(); ++it)
      {
         entry.clear();
         entry.insert( KIO::UDSEntry::UDS_NAME, (*it) );
         qCDebug(LOG_KIO_NFS)<<"listing "<<(*it);
         createVirtualDirEntry(entry);
         listEntry( entry );
      }
      finished();
      return;
   }

   QStringList filesToList;
   qCDebug(LOG_KIO_NFS)<<"getting subdir -"<<path<<"-";
   stripTrailingSlash(path);
   NFSFileHandle fh=getFileHandle(path);
   //cerr<<"this is the fh: -"<<fh<<"-"<<endl;
   if (fh.isInvalid())
   {
      error( ERR_DOES_NOT_EXIST, path);
      return;
   }
   readdirargs listargs;
   memset(&listargs,0,sizeof(listargs));
   listargs.count=1024*16;
   memcpy(listargs.dir.data,fh,NFS_FHSIZE);
   readdirres listres;
   entry* lastEntry = 0;
   do
   {
      memset(&listres,'\0',sizeof(listres));
      // In case that we didn't get all entries we need to set the cookie to the last one we actually received
      if(lastEntry != 0){
          memcpy(listargs.cookie, lastEntry->cookie, NFS_COOKIESIZE);
      }
      int clnt_stat = clnt_call(m_client, NFSPROC_READDIR, (xdrproc_t) xdr_readdirargs, (char*)&listargs,
                                (xdrproc_t) xdr_readdirres, (char*)&listres,total_timeout);
      if (!checkForError(clnt_stat,listres.status,path)) return;
      for (entry *dirEntry=listres.readdirres_u.reply.entries;dirEntry!=0;dirEntry=dirEntry->nextentry)
      {
         if ((QString(".")!=dirEntry->name) && (QString("..")!=dirEntry->name))
            filesToList.append(QFile::decodeName(dirEntry->name));
         lastEntry = dirEntry;
      }
   } while (!listres.readdirres_u.reply.eof);
   totalSize( filesToList.count());

   UDSEntry entry;
   //stat all files in filesToList
   for (QStringList::const_iterator it=filesToList.constBegin(); it!=filesToList.constEnd(); ++it)
   {
      diropargs dirargs;
      diropres dirres;
      memcpy(dirargs.dir.data,fh,NFS_FHSIZE);
      QByteArray tmpStr=QFile::encodeName(*it);
      dirargs.name=tmpStr.data();

      qCDebug(LOG_KIO_NFS)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-";

      int clnt_stat= clnt_call(m_client, NFSPROC_LOOKUP,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);
      if (!checkForError(clnt_stat,dirres.status,(*it))) return;

      NFSFileHandle tmpFH;
      tmpFH=dirres.diropres_u.diropres.file.data;
      m_handleCache.insert(path+'/'+(*it),tmpFH);

      entry.clear();

      entry.insert( KIO::UDSEntry::UDS_NAME, (*it) );

      //is it a symlink ?
      if (S_ISLNK(dirres.diropres_u.diropres.attributes.mode))
      {
         qCDebug(LOG_KIO_NFS)<<"it's a symlink !";
         //cerr<<"fh: "<<tmpFH<<endl;
         nfs_fh nfsFH;
         memcpy(nfsFH.data,dirres.diropres_u.diropres.file.data,NFS_FHSIZE);
         //get the link dest
         readlinkres readLinkRes;
         char nameBuf[NFS_MAXPATHLEN];
         readLinkRes.readlinkres_u.data=nameBuf;
         int clnt_stat=clnt_call(m_client, NFSPROC_READLINK,
                                 (xdrproc_t) xdr_nfs_fh, (char*)&nfsFH,
                                 (xdrproc_t) xdr_readlinkres, (char*)&readLinkRes,total_timeout);
         if (!checkForError(clnt_stat,readLinkRes.status,(*it))) return;
         qCDebug(LOG_KIO_NFS)<<"link dest is -"<<readLinkRes.readlinkres_u.data<<"-";
         QByteArray linkDest(readLinkRes.readlinkres_u.data);
         entry.insert( KIO::UDSEntry::UDS_LINK_DEST, QString::fromLocal8Bit( linkDest ) );

         bool isValid=isValidLink(path,linkDest);
         if (!isValid)
         {
            completeBadLinkUDSEntry(entry,dirres.diropres_u.diropres.attributes);
         }
         else
         {
            if (isAbsoluteLink(linkDest))
            {
               completeAbsoluteLinkUDSEntry(entry,linkDest);
            }
            else
            {
               tmpStr=QDir::cleanPath(path+QString("/")+QString(linkDest)).toLatin1();
               dirargs.name=tmpStr.data();
               tmpFH=getFileHandle(tmpStr);
               memcpy(dirargs.dir.data,tmpFH,NFS_FHSIZE);

               attrstat attrAndStat;

               qCDebug(LOG_KIO_NFS)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-";

               clnt_stat = clnt_call(m_client, NFSPROC_GETATTR,
                                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                                         (xdrproc_t) xdr_attrstat, (char*)&attrAndStat,total_timeout);
               if (!checkForError(clnt_stat,attrAndStat.status,tmpStr)) return;
               completeUDSEntry(entry,attrAndStat.attrstat_u.attributes);
            }
         }
      }
      else
         completeUDSEntry(entry,dirres.diropres_u.diropres.attributes);
      listEntry( entry );
   }
   finished();
}

void NFSProtocol::stat( const QUrl & url)
{
   QString path(url.path());
   stripTrailingSlash(path);
   qCDebug(LOG_KIO_NFS)<<"NFS::stat for -"<<path<<"-";
   QString tmpPath=path;
   if ((tmpPath.length()>1) && (tmpPath[0]=='/')) tmpPath=tmpPath.mid(1);
   // We can't stat root, but we know it's a dir
   if (isRoot(path) || isExportedDir(path))
   {
      UDSEntry entry;

      entry.insert( KIO::UDSEntry::UDS_NAME, path );
      createVirtualDirEntry(entry);
      // no size
      statEntry( entry );
      finished();
      qCDebug(LOG_KIO_NFS)<<"succeeded";
      return;
   }

   NFSFileHandle fh=getFileHandle(path);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,path);
      return;
   }

   diropargs dirargs;
   attrstat attrAndStat;
   memcpy(dirargs.dir.data,fh,NFS_FHSIZE);
   QByteArray tmpStr=QFile::encodeName(path);
   dirargs.name=tmpStr.data();

   qCDebug(LOG_KIO_NFS)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-";

   int clnt_stat = clnt_call(m_client, NFSPROC_GETATTR,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_attrstat, (char*)&attrAndStat,total_timeout);
   if (!checkForError(clnt_stat,attrAndStat.status,path)) return;
   UDSEntry entry;
   entry.clear();

   QString fileName, parentDir;
   getLastPart(path, fileName, parentDir);
   stripTrailingSlash(parentDir);

   entry.insert( KIO::UDSEntry::UDS_NAME, fileName );

   //is it a symlink ?
   if (S_ISLNK(attrAndStat.attrstat_u.attributes.mode))
   {
      qCDebug(LOG_KIO_NFS)<<"it's a symlink !";
      nfs_fh nfsFH;
      memcpy(nfsFH.data,fh,NFS_FHSIZE);
      //get the link dest
      readlinkres readLinkRes;
      char nameBuf[NFS_MAXPATHLEN];
      readLinkRes.readlinkres_u.data=nameBuf;

      int clnt_stat=clnt_call(m_client, NFSPROC_READLINK,
                              (xdrproc_t) xdr_nfs_fh, (char*)&nfsFH,
                              (xdrproc_t) xdr_readlinkres, (char*)&readLinkRes,total_timeout);
      if (!checkForError(clnt_stat,readLinkRes.status,path)) return;
      qCDebug(LOG_KIO_NFS)<<"link dest is -"<<readLinkRes.readlinkres_u.data<<"-";
      QByteArray linkDest(readLinkRes.readlinkres_u.data);
      entry.insert( KIO::UDSEntry::UDS_LINK_DEST, QString::fromLocal8Bit( linkDest ) );

      bool isValid=isValidLink(parentDir,linkDest);
      if (!isValid)
      {
         completeBadLinkUDSEntry(entry,attrAndStat.attrstat_u.attributes);
      }
      else
      {
         if (isAbsoluteLink(linkDest))
         {
            completeAbsoluteLinkUDSEntry(entry,linkDest);
         }
         else
         {

            tmpStr=QDir::cleanPath(parentDir+QString("/")+QString(linkDest)).toLatin1();
            diropargs dirargs;
            dirargs.name=tmpStr.data();
            NFSFileHandle tmpFH;
            tmpFH=getFileHandle(tmpStr);
            memcpy(dirargs.dir.data,tmpFH,NFS_FHSIZE);

            qCDebug(LOG_KIO_NFS)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-";
            clnt_stat = clnt_call(m_client, NFSPROC_GETATTR,
                                  (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                                  (xdrproc_t) xdr_attrstat, (char*)&attrAndStat,total_timeout);
            if (!checkForError(clnt_stat,attrAndStat.status,tmpStr)) return;
            completeUDSEntry(entry,attrAndStat.attrstat_u.attributes);
         }
      }
   }
   else
      completeUDSEntry(entry,attrAndStat.attrstat_u.attributes);
   statEntry( entry );
   finished();
}

void NFSProtocol::completeAbsoluteLinkUDSEntry(UDSEntry& entry, const QByteArray& path)
{
   //taken from file.cc
   struct stat buff;
   if ( ::stat( path.data(), &buff ) == -1 ) return;

   entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, buff.st_mode & S_IFMT ); // extract file type
   entry.insert( KIO::UDSEntry::UDS_ACCESS, buff.st_mode & 07777 ); // extract permissions
   entry.insert( KIO::UDSEntry::UDS_SIZE, buff.st_size );
   entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, buff.st_mtime );

   uid_t uid = buff.st_uid;

   QString str;
   if ( !m_usercache.contains( uid ) ) {
      struct passwd *user = getpwuid( uid );
      if ( user )
      {
         m_usercache.insert( uid, QString::fromLatin1(user->pw_name) );
         str = user->pw_name;
      }
      else
         str = "???";
   } else
      str = m_usercache.value( uid );

   entry.insert( KIO::UDSEntry::UDS_USER, str );

   gid_t gid = buff.st_gid;

   if ( !m_groupcache.contains( gid ) )
   {
      struct group *grp = getgrgid( gid );
      if ( grp )
      {
         m_groupcache.insert( gid, QString::fromLatin1(grp->gr_name) );
         str = grp->gr_name;
      }
      else
         str = "???";
   }
   else
      str = m_groupcache.value( gid );

   entry.insert( KIO::UDSEntry::UDS_GROUP, str );

   entry.insert( KIO::UDSEntry::UDS_ACCESS_TIME, buff.st_atime );
   entry.insert( KIO::UDSEntry::UDS_CREATION_TIME, buff.st_ctime );
}

void NFSProtocol::completeBadLinkUDSEntry(UDSEntry& entry, fattr& attributes)
{
   // It is a link pointing to nowhere
   completeUDSEntry(entry,attributes);

   entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFMT - 1 );
   entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO );
   entry.insert( KIO::UDSEntry::UDS_SIZE, 0L );
}

void NFSProtocol::completeUDSEntry(UDSEntry& entry, fattr& attributes)
{
   entry.insert( KIO::UDSEntry::UDS_SIZE, attributes.size );
   entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds );
   entry.insert( KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds );
   entry.insert( KIO::UDSEntry::UDS_CREATION_TIME, attributes.ctime.seconds );
   entry.insert( KIO::UDSEntry::UDS_ACCESS, (attributes.mode & 07777) );
   entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, attributes.mode & S_IFMT ); // extract file type

   uid_t uid = attributes.uid;
   QString str;
   if ( !m_usercache.contains( uid ) )
   {
      struct passwd *user = getpwuid( uid );
      if ( user )
      {
         m_usercache.insert( uid, QString::fromLatin1(user->pw_name) );
         str = user->pw_name;
      }
      else
         str = "???";
   }
   else
      str = m_usercache.value( uid );

   entry.insert( KIO::UDSEntry::UDS_USER, str );

   gid_t gid = attributes.gid;

   if ( !m_groupcache.contains( gid ) )
   {
      struct group *grp = getgrgid( gid );
      if ( grp )
      {
         m_groupcache.insert( gid, QString::fromLatin1(grp->gr_name) );
         str = grp->gr_name;
      }
      else
         str = "???";
   }
   else
      str = m_groupcache.value( gid );

   entry.insert( KIO::UDSEntry::UDS_GROUP, str );

/*   KIO::UDSEntry::ConstIterator it = entry.begin();
   for( ; it != entry.end(); it++ ) {
      switch (it.key()) {
      case KIO::UDSEntry::UDS_FILE_TYPE:
         qCDebug(LOG_KIO_NFS) << "File Type : " << (mode_t)((*it).m_long);
         break;
      case KIO::UDSEntry::UDS_ACCESS:
         qCDebug(LOG_KIO_NFS) << "Access permissions : " << (mode_t)((*it).m_long);
         break;
      case KIO::UDSEntry::UDS_USER:
         qCDebug(LOG_KIO_NFS) << "User : " << ((*it).m_str.toAscii() );
         break;
      case KIO::UDSEntry::UDS_GROUP:
         qCDebug(LOG_KIO_NFS) << "Group : " << ((*it).m_str.toAscii() );
         break;
      case KIO::UDSEntry::UDS_NAME:
         qCDebug(LOG_KIO_NFS) << "Name : " << ((*it).m_str.toAscii() );
         //m_strText = decodeFileName( (*it).m_str );
         break;
      case KIO::UDSEntry::UDS_URL:
         qCDebug(LOG_KIO_NFS) << "URL : " << ((*it).m_str.toAscii() );
         break;
      case KIO::UDSEntry::UDS_MIME_TYPE:
         qCDebug(LOG_KIO_NFS) << "MimeType : " << ((*it).m_str.toAscii() );
         break;
      case KIO::UDSEntry::UDS_LINK_DEST:
         qCDebug(LOG_KIO_NFS) << "LinkDest : " << ((*it).m_str.toAscii() );
         break;
      }
   }*/
}

void NFSProtocol::setHost(const QString& host, quint16 /*port*/, const QString& /*user*/, const QString& /*pass*/)
{
   qCDebug(LOG_KIO_NFS) << host;
   if (host.isEmpty())
   {
      error(ERR_UNKNOWN_HOST, QString());
      return;
   }
   if (host==m_currentHost) return;
   m_currentHost=host;
   m_handleCache.clear();
   m_exportedDirs.clear();
   closeConnection();
}

void NFSProtocol::mkdir( const QUrl& url, int permissions )
{
   qCDebug(LOG_KIO_NFS)<<"mkdir";
   QString thePath( url.path());
   stripTrailingSlash(thePath);
   QString dirName, parentDir;
   getLastPart(thePath, dirName, parentDir);
   stripTrailingSlash(parentDir);
   qCDebug(LOG_KIO_NFS)<<"path: -"<<thePath<<"- dir: -"<<dirName<<"- parentDir: -"<<parentDir<<"-";
   if (isRoot(parentDir))
   {
      error(ERR_WRITE_ACCESS_DENIED,thePath);
      return;
   }
   NFSFileHandle fh=getFileHandle(parentDir);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   }

   createargs createArgs;
   memcpy(createArgs.where.dir.data,fh,NFS_FHSIZE);
   QByteArray tmpName=QFile::encodeName(dirName);
   createArgs.where.name=tmpName.data();
   if (permissions==-1) createArgs.attributes.mode=0755;
   else createArgs.attributes.mode=permissions;

   diropres dirres;

   int clnt_stat = clnt_call(m_client, NFSPROC_MKDIR,
                         (xdrproc_t) xdr_createargs, (char*)&createArgs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);
   if (!checkForError(clnt_stat,dirres.status,thePath)) return;
   finished();
}

bool NFSProtocol::checkForError(int clientStat, int nfsStat, const QString& text)
{
   if (clientStat!=RPC_SUCCESS)
   {
      qCDebug(LOG_KIO_NFS)<<"rpc error: "<<clientStat;
      //does this mapping make sense ?
      error(ERR_CONNECTION_BROKEN,i18n("An RPC error occurred."));
      return false;
   }
   if (nfsStat!=NFS_OK)
   {
      qCDebug(LOG_KIO_NFS)<<"nfs error: "<<nfsStat;
      switch (nfsStat)
      {
      case NFSERR_PERM:
         error(ERR_ACCESS_DENIED,text);
         break;
      case NFSERR_NOENT:
         error(ERR_DOES_NOT_EXIST,text);
         break;
      //does this mapping make sense ?
      case NFSERR_IO:
         error(ERR_INTERNAL_SERVER,text);
         break;
      //does this mapping make sense ?
      case NFSERR_NXIO:
         error(ERR_DOES_NOT_EXIST,text);
         break;
      case NFSERR_ACCES:
         error(ERR_ACCESS_DENIED,text);
         break;
      case NFSERR_EXIST:
         error(ERR_FILE_ALREADY_EXIST,text);
         break;
      //does this mapping make sense ?
      case NFSERR_NODEV:
         error(ERR_DOES_NOT_EXIST,text);
         break;
      case NFSERR_NOTDIR:
         error(ERR_IS_FILE,text);
         break;
      case NFSERR_ISDIR:
         error(ERR_IS_DIRECTORY,text);
         break;
      //does this mapping make sense ?
      case NFSERR_FBIG:
         error(ERR_INTERNAL_SERVER,text);
         break;
      //does this mapping make sense ?
      case NFSERR_NOSPC:
         error(ERR_INTERNAL_SERVER,i18n("No space left on device"));
         break;
      case NFSERR_ROFS:
         error(ERR_COULD_NOT_WRITE,i18n("Read only file system"));
         break;
      case NFSERR_NAMETOOLONG:
         error(ERR_INTERNAL_SERVER,i18n("Filename too long"));
         break;
      case NFSERR_NOTEMPTY:
         error(ERR_COULD_NOT_RMDIR,text);
         break;
      //does this mapping make sense ?
      case NFSERR_DQUOT:
         error(ERR_INTERNAL_SERVER,i18n("Disk quota exceeded"));
         break;
      case NFSERR_STALE:
         error(ERR_DOES_NOT_EXIST,text);
         break;
      default:
         error(ERR_UNKNOWN,text);
         break;
      }
      return false;
   }
   return true;
}

void NFSProtocol::del( const QUrl& url, bool isfile)
{
   QString thePath( url.path());
   stripTrailingSlash(thePath);

   QString fileName, parentDir;
   getLastPart(thePath, fileName, parentDir);
   stripTrailingSlash(parentDir);
   qCDebug(LOG_KIO_NFS)<<"del(): path: -"<<thePath<<"- file -"<<fileName<<"- parentDir: -"<<parentDir<<"-";
   if (isRoot(parentDir))
   {
      error(ERR_ACCESS_DENIED,thePath);
      return;
   }

   NFSFileHandle fh=getFileHandle(parentDir);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   }

   if (isfile)
   {
      qCDebug(LOG_KIO_NFS)<<"Deleting file "<<thePath;
      diropargs dirOpArgs;
      memcpy(dirOpArgs.dir.data,fh,NFS_FHSIZE);
      QByteArray tmpName=QFile::encodeName(fileName);
      dirOpArgs.name=tmpName.data();

      nfsstat nfsStat;

      int clnt_stat = clnt_call(m_client, NFSPROC_REMOVE,
                            (xdrproc_t) xdr_diropargs, (char*)&dirOpArgs,
                            (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
      if (!checkForError(clnt_stat,nfsStat,thePath)) return;
      qCDebug(LOG_KIO_NFS)<<"removing "<<thePath<<" from cache";
      m_handleCache.erase(m_handleCache.find(thePath));
      finished();
   }
   else
   {
      qCDebug(LOG_KIO_NFS)<<"Deleting directory "<<thePath;
      diropargs dirOpArgs;
      memcpy(dirOpArgs.dir.data,fh,NFS_FHSIZE);
      QByteArray tmpName=QFile::encodeName(fileName);
      dirOpArgs.name=tmpName.data();

      nfsstat nfsStat;

      int clnt_stat = clnt_call(m_client, NFSPROC_RMDIR,
                            (xdrproc_t) xdr_diropargs, (char*)&dirOpArgs,
                            (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
      if (!checkForError(clnt_stat,nfsStat,thePath)) return;
      qCDebug(LOG_KIO_NFS)<<"removing "<<thePath<<" from cache";
      m_handleCache.erase(m_handleCache.find(thePath));
      finished();
   }
}

void NFSProtocol::chmod( const QUrl& url, int permissions )
{
   QString thePath(url.path());
   stripTrailingSlash(thePath);
   qCDebug(LOG_KIO_NFS) <<  "chmod -"<< thePath << "-";
   if (isRoot(thePath) || isExportedDir(thePath))
   {
      error(ERR_ACCESS_DENIED,thePath);
      return;
   }

   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   }

   sattrargs sAttrArgs;
   memcpy(sAttrArgs.file.data,fh,NFS_FHSIZE);
   sAttrArgs.attributes.uid=(unsigned int)-1;
   sAttrArgs.attributes.gid=(unsigned int)-1;
   sAttrArgs.attributes.size=(unsigned int)-1;
   sAttrArgs.attributes.atime.seconds=(unsigned int)-1;
   sAttrArgs.attributes.atime.useconds=(unsigned int)-1;
   sAttrArgs.attributes.mtime.seconds=(unsigned int)-1;
   sAttrArgs.attributes.mtime.useconds=(unsigned int)-1;

   sAttrArgs.attributes.mode=permissions;

   nfsstat nfsStat;

   int clnt_stat = clnt_call(m_client, NFSPROC_SETATTR,
                         (xdrproc_t) xdr_sattrargs, (char*)&sAttrArgs,
                         (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
   if (!checkForError(clnt_stat,nfsStat,thePath)) return;

   finished();
}

void NFSProtocol::get( const QUrl& url )
{
   QString thePath(url.path());
   qCDebug(LOG_KIO_NFS)<<"get() -"<<thePath<<"-";
   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   }
   readargs readArgs;
   memcpy(readArgs.file.data,fh,NFS_FHSIZE);
   readArgs.offset=0;
   readArgs.count=NFS_MAXDATA;
   readArgs.totalcount=NFS_MAXDATA;
   readres readRes;
   int offset(0);
   char buf[NFS_MAXDATA];
   readRes.readres_u.reply.data.data_val=buf;

   QByteArray array;
   do
   {
      int clnt_stat = clnt_call(m_client, NFSPROC_READ,
                            (xdrproc_t) xdr_readargs, (char*)&readArgs,
                            (xdrproc_t) xdr_readres, (char*)&readRes,total_timeout);
      if (!checkForError(clnt_stat,readRes.status,thePath)) return;
      if (readArgs.offset==0)
         totalSize(readRes.readres_u.reply.attributes.size);

      offset=readRes.readres_u.reply.data.data_len;
      //qCDebug(LOG_KIO_NFS)<<"read "<<offset<<" bytes";
      readArgs.offset+=offset;
      if (offset>0)
      {
         array = QByteArray::fromRawData(readRes.readres_u.reply.data.data_val, offset);
         data( array );
         array.clear();

         processedSize(readArgs.offset);
      }

   } while (offset>0);
   data( QByteArray() );
   finished();
}

//TODO the partial putting thing is not yet implemented
void NFSProtocol::put( const QUrl& url, int _mode, KIO::JobFlags flags )
{
    QString destPath( url.path());
    qCDebug(LOG_KIO_NFS) << "Put -" << destPath <<"-";
    /*QString dest_part( dest_orig );
    dest_part += ".part";*/

    stripTrailingSlash(destPath);
    QString parentDir, fileName;
    getLastPart(destPath,fileName, parentDir);
    if (isRoot(parentDir))
    {
       error(ERR_WRITE_ACCESS_DENIED,destPath);
       return;
    }

    NFSFileHandle destFH;
    destFH=getFileHandle(destPath);
    qCDebug(LOG_KIO_NFS)<<"file handle for -"<<destPath<<"- is "<<destFH;

    //the file exists and we don't want to overwrite
    if ((!(flags & KIO::Overwrite)) && (!destFH.isInvalid()))
    {
       error(ERR_FILE_ALREADY_EXIST,destPath);
       return;
    }
    //TODO: is this correct ?
    //we have to "create" the file anyway, no matter if it already
    //exists or not
    //if we don't create it new, written text will be, hmm, "inserted"
    //in the existing file, i.e. a file could not become smaller, since
    //write only overwrites or extends, but doesn't remove stuff from a file (aleXXX)

    qCDebug(LOG_KIO_NFS)<<"creating the file -"<<fileName<<"-";
    NFSFileHandle parentFH;
    parentFH=getFileHandle(parentDir);
    //cerr<<"fh for parent dir: "<<parentFH<<endl;
    //the directory doesn't exist
    if (parentFH.isInvalid())
    {
       qCDebug(LOG_KIO_NFS)<<"parent directory -"<<parentDir<<"- does not exist";
       error(ERR_DOES_NOT_EXIST,parentDir);
       return;
    }
    createargs createArgs;
    memcpy(createArgs.where.dir.data,(const char*)parentFH,NFS_FHSIZE);
    QByteArray tmpName=QFile::encodeName(fileName);
    createArgs.where.name=tmpName.data();

    //the mode is apparently ignored if the file already exists
    if (_mode==-1) createArgs.attributes.mode=0644;
    else createArgs.attributes.mode=_mode;
    createArgs.attributes.uid=geteuid();
    createArgs.attributes.gid=getegid();
    //this is required, otherwise we are not able to write shorter files
    createArgs.attributes.size=0;
    //hmm, do we need something here ? I don't think so
    createArgs.attributes.atime.seconds=(unsigned int)-1;
    createArgs.attributes.atime.useconds=(unsigned int)-1;
    createArgs.attributes.mtime.seconds=(unsigned int)-1;
    createArgs.attributes.mtime.useconds=(unsigned int)-1;

    diropres dirOpRes;
    int clnt_stat = clnt_call(m_client, NFSPROC_CREATE,
                              (xdrproc_t) xdr_createargs, (char*)&createArgs,
                              (xdrproc_t) xdr_diropres, (char*)&dirOpRes,total_timeout);
    if (!checkForError(clnt_stat,dirOpRes.status,fileName)) return;
    //we created the file successfully
    //destFH=getFileHandle(destPath);
    destFH=dirOpRes.diropres_u.diropres.file.data;
    qCDebug(LOG_KIO_NFS)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully";
    //cerr<<"with fh "<<destFH<<endl;

    //now we can put
    int result;
    // Loop until we got 0 (end of data)
    writeargs writeArgs;
    memcpy(writeArgs.file.data,(const char*)destFH,NFS_FHSIZE);
    writeArgs.beginoffset=0;
    writeArgs.totalcount=0;
    writeArgs.offset=0;
    attrstat attrStat;
    int bytesWritten(0);
    qCDebug(LOG_KIO_NFS)<<"starting to put";
    do
    {
       QByteArray buffer;
       dataReq(); // Request for data
       result = readData( buffer );
       //qCDebug(LOG_KIO_NFS)<<"received "<<result<<" bytes for putting";
       char * data=buffer.data();
       int bytesToWrite=buffer.size();
       int writeNow(0);
       if (result > 0)
       {
          do
          {
             if (bytesToWrite>NFS_MAXDATA)
             {
                writeNow=NFS_MAXDATA;
             }
             else
             {
                writeNow=bytesToWrite;
             };
             writeArgs.data.data_val=data;
             writeArgs.data.data_len=writeNow;

             int clnt_stat = clnt_call(m_client, NFSPROC_WRITE,
                                       (xdrproc_t) xdr_writeargs, (char*)&writeArgs,
                                       (xdrproc_t) xdr_attrstat, (char*)&attrStat,total_timeout);
             //qCDebug(LOG_KIO_NFS)<<"written";
             if (!checkForError(clnt_stat,attrStat.status,fileName)) return;
             bytesWritten+=writeNow;
             writeArgs.offset=bytesWritten;

             //adjust the pointer
             data=data+writeNow;
             //decrease the rest
             bytesToWrite-=writeNow;
          } while (bytesToWrite>0);
       }
    } while ( result > 0 );
    finished();
}

void NFSProtocol::rename( const QUrl &src, const QUrl &dest, KIO::JobFlags _flags )
{
   QString srcPath( src.path());
   QString destPath( dest.path());
   stripTrailingSlash(srcPath);
   stripTrailingSlash(destPath);
   qCDebug(LOG_KIO_NFS)<<"renaming -"<<srcPath<<"- to -"<<destPath<<"-";

   if (isRoot(srcPath) || isExportedDir(srcPath))
   {
      error(ERR_CANNOT_RENAME,srcPath);
      return;
   }

   if (!(_flags & KIO::Overwrite))
   {
      NFSFileHandle testFH;
      testFH=getFileHandle(destPath);
      if (!testFH.isInvalid())
      {
         error(ERR_FILE_ALREADY_EXIST,destPath);
         return;
      }
   }

   QString srcFileName, srcParentDir, destFileName, destParentDir;

   getLastPart(srcPath, srcFileName, srcParentDir);
   NFSFileHandle srcFH=getFileHandle(srcParentDir);
   if (srcFH.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,srcParentDir);
      return;
   }
   renameargs renameArgs;
   memcpy(renameArgs.from.dir.data,srcFH,NFS_FHSIZE);
   QByteArray tmpName=QFile::encodeName(srcFileName);
   renameArgs.from.name=tmpName.data();

   getLastPart(destPath, destFileName, destParentDir);
   NFSFileHandle destFH=getFileHandle(destParentDir);
   if (destFH.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,destParentDir);
      return;
   }
   memcpy(renameArgs.to.dir.data,destFH,NFS_FHSIZE);
   QByteArray tmpName2=QFile::encodeName(destFileName);
   renameArgs.to.name=tmpName2.data();
   nfsstat nfsStat;

   int clnt_stat = clnt_call(m_client, NFSPROC_RENAME,
                             (xdrproc_t) xdr_renameargs, (char*)&renameArgs,
                             (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
   if (!checkForError(clnt_stat,nfsStat,destPath)) return;
   finished();
}

void NFSProtocol::copy( const QUrl &src, const QUrl &dest, int _mode, KIO::JobFlags _flags )
{
   //prepare the source
   QString thePath( src.path());
   stripTrailingSlash(thePath);
   qCDebug(LOG_KIO_NFS) << "Copy to -" << thePath <<"-";
   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   };

   //create the destination
   QString destPath( dest.path());
   stripTrailingSlash(destPath);
   QString parentDir, fileName;
   getLastPart(destPath,fileName, parentDir);
   if (isRoot(parentDir))
   {
      error(ERR_ACCESS_DENIED,destPath);
      return;
   }
   NFSFileHandle destFH;
   destFH=getFileHandle(destPath);
   qCDebug(LOG_KIO_NFS)<<"file handle for -"<<destPath<<"- is "<<destFH;

   //the file exists and we don't want to overwrite
   if ((!(_flags & KIO::Overwrite)) && (!destFH.isInvalid()))
   {
      error(ERR_FILE_ALREADY_EXIST,destPath);
      return;
   }
   //TODO: is this correct ?
   //we have to "create" the file anyway, no matter if it already
   //exists or not
   //if we don't create it new, written text will be, hmm, "inserted"
   //in the existing file, i.e. a file could not become smaller, since
   //write only overwrites or extends, but doesn't remove stuff from a file

   qCDebug(LOG_KIO_NFS)<<"creating the file -"<<fileName<<"-";
   NFSFileHandle parentFH;
   parentFH=getFileHandle(parentDir);
   //the directory doesn't exist
   if (parentFH.isInvalid())
   {
      qCDebug(LOG_KIO_NFS)<<"parent directory -"<<parentDir<<"- does not exist";
      error(ERR_DOES_NOT_EXIST,parentDir);
      return;
   };
   createargs createArgs;
   memcpy(createArgs.where.dir.data,(const char*)parentFH,NFS_FHSIZE);
   QByteArray tmpName=QFile::encodeName(fileName);
   createArgs.where.name=tmpName.data();
   if (_mode==-1) createArgs.attributes.mode=0644;
   else createArgs.attributes.mode=_mode;
   createArgs.attributes.uid=geteuid();
   createArgs.attributes.gid=getegid();
   createArgs.attributes.size=0;
   createArgs.attributes.atime.seconds=(unsigned int)-1;
   createArgs.attributes.atime.useconds=(unsigned int)-1;
   createArgs.attributes.mtime.seconds=(unsigned int)-1;
   createArgs.attributes.mtime.useconds=(unsigned int)-1;

   diropres dirOpRes;
   int clnt_stat = clnt_call(m_client, NFSPROC_CREATE,
                             (xdrproc_t) xdr_createargs, (char*)&createArgs,
                             (xdrproc_t) xdr_diropres, (char*)&dirOpRes,total_timeout);
   if (!checkForError(clnt_stat,dirOpRes.status,destPath)) return;
   //we created the file successfully
   destFH=dirOpRes.diropres_u.diropres.file.data;
   qCDebug(LOG_KIO_NFS)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully";

   char buf[NFS_MAXDATA];
   writeargs writeArgs;
   memcpy(writeArgs.file.data,(const char*)destFH,NFS_FHSIZE);
   writeArgs.beginoffset=0;
   writeArgs.totalcount=0;
   writeArgs.offset=0;
   writeArgs.data.data_val=buf;
   attrstat attrStat;

   readargs readArgs;
   memcpy(readArgs.file.data,fh,NFS_FHSIZE);
   readArgs.offset=0;
   readArgs.count=NFS_MAXDATA;
   readArgs.totalcount=NFS_MAXDATA;
   readres readRes;
   readRes.readres_u.reply.data.data_val=buf;

   int bytesRead(0);
   do
   {
      //first read
      int clnt_stat = clnt_call(m_client, NFSPROC_READ,
                                (xdrproc_t) xdr_readargs, (char*)&readArgs,
                                (xdrproc_t) xdr_readres, (char*)&readRes,total_timeout);
      if (!checkForError(clnt_stat,readRes.status,thePath)) return;
      if (readArgs.offset==0)
         totalSize(readRes.readres_u.reply.attributes.size);

      bytesRead=readRes.readres_u.reply.data.data_len;
      //qCDebug(LOG_KIO_NFS)<<"read "<<bytesRead<<" bytes";
      //then write
      if (bytesRead>0)
      {
         readArgs.offset+=bytesRead;

         writeArgs.data.data_len=bytesRead;

         clnt_stat = clnt_call(m_client, NFSPROC_WRITE,
                               (xdrproc_t) xdr_writeargs, (char*)&writeArgs,
                               (xdrproc_t) xdr_attrstat, (char*)&attrStat,total_timeout);
         //qCDebug(LOG_KIO_NFS)<<"written";
         if (!checkForError(clnt_stat,attrStat.status,destPath)) return;
         writeArgs.offset+=bytesRead;
      }
   } while (bytesRead>0);

   finished();
}

//TODO why isn't this even called ?
void NFSProtocol::symlink( const QString &target, const QUrl &dest, KIO::JobFlags )
{
   qCDebug(LOG_KIO_NFS)<<"symlinking ";
   QString destPath=dest.path();
   stripTrailingSlash(destPath);

   QString parentDir, fileName;
   getLastPart(destPath,fileName, parentDir);
   qCDebug(LOG_KIO_NFS)<<"symlinking "<<parentDir<<" "<<fileName<<" to "<<target;
   NFSFileHandle fh=getFileHandle(parentDir);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,parentDir);
      return;
   }
   if (isRoot(parentDir))
   {
      error(ERR_ACCESS_DENIED,destPath);
      return;
   }

   qCDebug(LOG_KIO_NFS)<<"tach";
   QByteArray tmpStr=target.toLatin1();
   symlinkargs symLinkArgs;
   symLinkArgs.to=tmpStr.data();
   memcpy(symLinkArgs.from.dir.data,(const char*)fh,NFS_FHSIZE);
   QByteArray tmpStr2=QFile::encodeName(destPath);
   symLinkArgs.from.name=tmpStr2.data();

   nfsstat nfsStat;
   int clnt_stat = clnt_call(m_client, NFSPROC_SYMLINK,
                             (xdrproc_t) xdr_symlinkargs, (char*)&symLinkArgs,
                             (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
   if (!checkForError(clnt_stat,nfsStat,destPath)) return;

   finished();

}

bool NFSProtocol::isValidLink(const QString& parentDir, const QString& linkDest)
{
   qCDebug(LOG_KIO_NFS)<<"isValidLink: parent: "<<parentDir<<" link: "<<linkDest;
   if (linkDest.isEmpty()) return false;
   if (isAbsoluteLink(linkDest))
   {
      qCDebug(LOG_KIO_NFS)<<"is an absolute link";
      return QFile::exists(linkDest);
   }
   else
   {
      qCDebug(LOG_KIO_NFS)<<"is a relative link";
      QString absDest=parentDir+'/'+linkDest;
      qCDebug(LOG_KIO_NFS)<<"pointing abs to "<<absDest;
      absDest=removeFirstPart(absDest);
      qCDebug(LOG_KIO_NFS)<<"removed first part "<<absDest;
      absDest=QDir::cleanPath(absDest);
      qCDebug(LOG_KIO_NFS)<<"simplified to "<<absDest;
      if (absDest.indexOf("../")==0)
         return false;

      qCDebug(LOG_KIO_NFS)<<"is inside the nfs tree";
      absDest=parentDir+'/'+linkDest;
      absDest=QDir::cleanPath(absDest);
      qCDebug(LOG_KIO_NFS)<<"getting file handle of "<<absDest;
      NFSFileHandle fh=getFileHandle(absDest);
      return (!fh.isInvalid());
   }
   return false;
}

