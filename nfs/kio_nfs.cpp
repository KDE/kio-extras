/*  This file is part of the KDE libraries
    Copyright (C) 2000 Alexander Neundorf <alexander.neundorf@rz.tu-ilmenau.de>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream.h>

#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/stat.h>
/*#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>*/
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
/*#ifdef STDC_HEADERS*/
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
/*#endif*/

#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
//#include "getopt.h"

#include "nfs_prot.h"
#include "mount.h"
#include "kio_nfs.h"
#include <kinstance.h>
#include <kdebug.h>
#include <kio/global.h>

#include <qfile.h>

#define MAXHOSTLEN 256

#define MAXFHAGE 60*10   //10 minutes maximum age for file handles

#define NFSPROG ((u_long)100003)
#define NFSVERS ((u_long)2)


using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_nfs" );

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_nfs protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }
  NFSProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  kdDebug(7101) << "NFS: kdemain: Done" << endl;
  return 0;
}

void stripTrailingSlash(QString& path)
{
   //if (path=="/") return;
   if (path=="/") path=""; else
   if (path[path.length()-1]=='/') path.truncate(path.length()-1);
};

void getLastPart(const QString& path, QString& lastPart, QString& rest)
{
   int slashPos=path.findRev("/");
   lastPart=path.mid(slashPos+1,1000);
   rest=path.left(slashPos+1);
};

NFSFileHandle::NFSFileHandle()
:m_isInvalid(FALSE)
{
   m_handle=new char[NFS_FHSIZE+1];
   memset(m_handle,'\0',NFS_FHSIZE+1);
   m_detectTime=time(0);
};

NFSFileHandle::NFSFileHandle(const NFSFileHandle & handle)
:m_isInvalid(FALSE)
{
   m_handle=new char[NFS_FHSIZE+1];
   m_handle[NFS_FHSIZE]='\0';
   memcpy(m_handle,handle.m_handle,NFS_FHSIZE);
   m_isInvalid=handle.m_isInvalid;
   m_detectTime=handle.m_detectTime;
};

NFSFileHandle::~NFSFileHandle()
{
   delete [] m_handle;
}

NFSFileHandle& NFSFileHandle::operator= (const NFSFileHandle& src)
{
   memcpy(m_handle,src.m_handle,NFS_FHSIZE);
   m_isInvalid=src.m_isInvalid;
   m_detectTime=src.m_detectTime;
   return *this;
}

NFSFileHandle& NFSFileHandle::operator= (const char* src)
{
   if (src==0)
   {
      m_isInvalid=TRUE;
      return *this;
   };
   memcpy(m_handle,src,NFS_FHSIZE);
   m_isInvalid=FALSE;
   m_detectTime=time(0);
   return *this;
}

time_t NFSFileHandle::age() const
{
   return (time(0)-m_detectTime);
};

ostream& operator<< (ostream& s, const NFSFileHandle& x)
{
   for (int i =0; i<NFS_FHSIZE; i++)
      s<<hex<<(unsigned int)x[i]<<" ";
   s<<dec;
   return s;
}

NFSProtocol::NFSProtocol (const QCString &pool, const QCString &app )
:SlaveBase( "nfs", pool, app )
,m_client(0)
{
   cerr<<"NFS::NFS: -"<<pool<<"-"<<endl;
};

NFSProtocol::~NFSProtocol()
{
};

NFSFileHandle NFSProtocol::getFileHandle(QString path)
{
   if (m_client==0) openConnection();
   stripTrailingSlash(path);
   kdDebug(7101)<<"getting FH for -"<<path<<"-"<<endl;
   //now the path looks like "/root/some/dir" or "" if it was "/"
   NFSFileHandle parentFH;
   //we didn't find it
   if (path.isEmpty())
   {
      kdDebug(7101)<<"path is empty, invalidating the FH"<<endl;
      parentFH.setInvalid();
      return parentFH;
   };
   //check wether we have this filehandle cached
   //the filehandles of the exported root dirs are always in the cache
   if (m_handleCache.find(path)!=m_handleCache.end())
   {
/*      kdDebug(7101)<<"age: "<<m_handleCache[path].age()<<" max age: "<<MAXFHAGE<<endl;
      if (m_handleCache[path].age()>MAXFHAGE)
      {
         kdDebug(7101)<<"path is in the cache but to old, deleting the FH -"<<m_handleCache[path]<<"-"<<endl;
         m_handleCache.remove(m_handleCache.find(path));
      }
      else
      {*/
      kdDebug(7101)<<"path is in the cache, returning the FH -"<<m_handleCache[path]<<"-"<<endl;
      return m_handleCache[path];
      //};
   };
   QString rest, lastPart;
   getLastPart(path,lastPart,rest);
   kdDebug(7101)<<"splitting path into rest -"<<rest<<"- and lastPart -"<<lastPart<<"-"<<endl;

   parentFH=getFileHandle(rest);
   //f*ck, it's invalid
   if (parentFH.isInvalid())
   {
      kdDebug(7101)<<"the parent FH is invalid"<<endl;
      return parentFH;
   };
   // do the rpc call

   diropargs dirargs;
   diropres dirres;
   memcpy(dirargs.dir.data,(const char*)parentFH,NFS_FHSIZE);
   strcpy(dirargs.name, QFile::encodeName(lastPart).data());

   kdDebug(7101)<<"calling rpc: FH: -"<<parentFH<<"- with name -"<<dirargs.name<<"-"<<endl;

   int clnt_stat = clnt_call(m_client, NFSPROC_LOOKUP,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);

   if ((clnt_stat!=RPC_SUCCESS) || (dirres.status!=NFS_OK))
   {
      //we failed
      kdDebug(7101)<<"lookup of filehandle failed"<<endl;
      parentFH.setInvalid();
      return parentFH;
   };
   //everything went fine up to now :-)
   parentFH=dirres.diropres_u.diropres.file.data;
   kdDebug(7101)<<"filesize: "<<dirres.diropres_u.diropres.attributes.size<<endl;
   m_handleCache.insert(path,parentFH);
   kdDebug(7101)<<"return FH -"<<parentFH<<"-"<<endl;
   return parentFH;
};

void NFSProtocol::openConnection()
{
   kdDebug(7101)<<"NFS::openConnection"<<endl;
   int m_sock(0);
   struct sockaddr_in server_addr;
   if (m_currentHost[0] >= '0' && m_currentHost[0] <= '9')
   {
      kdDebug(7101)<<"getting host from ip"<<endl;
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = inet_addr(m_currentHost);
   }
   else
   {
      struct hostent *hp=gethostbyname(m_currentHost);
      if (hp==0)
      {
         kdDebug(7101)<<"getting host from name failed"<<endl;
         exit(1);
      }
      server_addr.sin_family = AF_INET;
      memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
   }

   // create mount deamon client
   m_sock = RPC_ANYSOCK;
   server_addr.sin_port = 0;
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
         clnt_pcreateerror("mount clntudp_create");
         exit(1);
      }
   }
   //m_client->cl_auth = authunix_create_default();
   //TODO how do I detect the gid ?
   m_client->cl_auth = authunix_create("localhost",geteuid(),0,0,0);
   total_timeout.tv_sec = 20;
   total_timeout.tv_usec = 0;

	exports exportlist;
   //now do the stuff
   memset(&exportlist, '\0', sizeof(exportlist));

   int clnt_stat = clnt_call(m_client, MOUNTPROC_EXPORT,(xdrproc_t) xdr_void, NULL,
                         (xdrproc_t) xdr_exports, (char*)&exportlist,total_timeout);
   if (!checkForError(clnt_stat,0,"openConnection")) return;
   kdDebug(7101)<<"NFS::openConnection parsing exportList"<<endl;

   fhstatus fhStatus;
   for(; exportlist!=0;exportlist = exportlist->ex_next)
   {
      kdDebug(7101)<<"found export: "<<exportlist->ex_dir<<endl;

      memset(&fhStatus,0,sizeof(fhStatus));
      clnt_stat = clnt_call(m_client, MOUNTPROC_MNT,(xdrproc_t) xdr_dirpath, (char*)(&(exportlist->ex_dir)),
                            (xdrproc_t) xdr_fhstatus,(char*) &fhStatus,total_timeout);
      if (fhStatus.fhs_status==0)
      {
         NFSFileHandle fh;
         fh=fhStatus.fhstatus_u.fhs_fhandle;
         //TODO remove this ugly hack
         m_handleCache.insert(QString("/")+KIO::encodeFileName(exportlist->ex_dir),fh);
         //m_handleCache.insert(KIO::encodeFileName(exportlist->ex_dir),fh);
         m_exportedDirs.append(KIO::encodeFileName(exportlist->ex_dir));
         cerr<<"appending file -"<<KIO::encodeFileName(exportlist->ex_dir)<<"- with FH: -"<<fhStatus.fhstatus_u.fhs_fhandle<<"-"<<endl;
      }
      else
      {
         kdDebug(7101)<<"fuck mountproc failed"<<endl;
      };

   }
   server_addr.sin_port = 0;
	m_sock = RPC_ANYSOCK;
   m_client = clnttcp_create(&server_addr,NFSPROG,NFSVERS,&m_sock,0,0);
   if (m_client == NULL)
   {
      kdDebug(7101)<<"error occured in creating the client for NFS"<<endl;
   };
   m_client->cl_auth = authunix_create("localhost",geteuid(),0,0,0);
   kdDebug(7101)<<"openConnection succeeded"<<endl;
   connected();
}

void NFSProtocol::listDir( const KURL& _url)
{
   KURL url(_url);
   QString path( QFile::encodeName(url.path()));
   cerr<<"NFS::listDir: path: -"<<path<<"-"<<endl;

   if (path.isEmpty())
   {
      url.setPath("/");
      redirection(url);
   };
   if (path.isEmpty() || (path=="/"))
   {
      if (m_client==0) openConnection();
      kdDebug(7101)<<"listing root"<<endl;
      totalSize( m_exportedDirs.count());

      //in this case we don't need to do a real listdir
      UDSEntry entry;
      for (QStringList::Iterator it=m_exportedDirs.begin(); it!=m_exportedDirs.end(); it++)
      {
         UDSAtom atom;
         entry.clear();
         atom.m_uds = KIO::UDS_NAME;
         atom.m_str = (*it);
         entry.append( atom );
         createVirtualDirEntry(entry);

         listEntry( entry, false);
      };
      listEntry( entry, true ); // ready
      finished();
      kdDebug(7101)<<"listing "<<m_exportedDirs.count()<<" exports"<<endl;

      return;
   }

   QStringList filesToList;
   kdDebug(7101)<<"getting subdir"<<endl;
   stripTrailingSlash(path);
   NFSFileHandle fh=getFileHandle(path);
   cerr<<"this is the fh: -"<<fh<<"-"<<endl;
   if (fh.isInvalid())
   {
      error( ERR_DOES_NOT_EXIST, strdup(path));
      return;
   };
   readdirargs listargs;
   memset(&listargs,0,sizeof(listargs));
   listargs.count=1024*16;
   memcpy(listargs.dir.data,fh,NFS_FHSIZE);
   readdirres listres;
   do
   {
      memset(&listres,'\0',sizeof(listres));
      int clnt_stat = clnt_call(m_client, NFSPROC_READDIR, (xdrproc_t) xdr_readdirargs, (char*)&listargs,
                                (xdrproc_t) xdr_readdirres, (char*)&listres,total_timeout);
      if (!checkForError(clnt_stat,listres.status,"listDir")) return;
      for (entry *dirEntry=listres.readdirres_u.reply.entries;dirEntry!=0;dirEntry=dirEntry->nextentry)
      {
         kdDebug(7101)<<"adding to list: -"<<dirEntry->name<<"-"<<endl;
         filesToList.append(dirEntry->name);
      };
   } while (!listres.readdirres_u.reply.eof);
   totalSize( filesToList.count());

   UDSEntry entry;
   //stat all files in filesToList
   for (QStringList::Iterator it=filesToList.begin(); it!=filesToList.end(); it++)
   {
      UDSAtom atom;
      diropargs dirargs;
      diropres dirres;
      memcpy(dirargs.dir.data,fh,NFS_FHSIZE);
      dirargs.name=(const char*)(*it);

      kdDebug(7101)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-"<<endl;

      int clnt_stat=clnt_call(m_client, NFSPROC_LOOKUP,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);
      //if (!checkForError(clnt_stat,dirres.status)) return;

      NFSFileHandle tmpFH;
      tmpFH=dirres.diropres_u.diropres.file.data;
      m_handleCache.insert(path+"/"+(*it),tmpFH);
      entry.clear();

      atom.m_uds = KIO::UDS_NAME;
      atom.m_str = (*it);
      entry.append( atom );

      kdDebug(7101)<<"checking for symlink"<<endl;
      //it's a symlink
      if (S_ISLNK(dirres.diropres_u.diropres.attributes.mode))
      {
         kdDebug(7101)<<"it's a symlink !"<<endl;
         cerr<<"fh: "<<tmpFH<<endl;
         nfs_fh nfsFH;
         memcpy(nfsFH.data,dirres.diropres_u.diropres.file.data,NFS_FHSIZE);
         //get the link dest
         readlinkres readLinkRes;
         char nameBuf[NFS_MAXPATHLEN];
         readLinkRes.readlinkres_u.data=nameBuf;
         int clnt_stat=clnt_call(m_client, NFSPROC_READLINK,
                                 (xdrproc_t) xdr_nfs_fh, (char*)&nfsFH,
                                 (xdrproc_t) xdr_readlinkres, (char*)&readLinkRes,total_timeout);
         kdDebug(7101)<<"readLinkRes rpc status: "<<clnt_stat<<" nfs status: "<<int(readLinkRes.status)<<endl;
         if (readLinkRes.status==NFS_OK)
         {
            kdDebug(7101)<<"link dest is -"<<readLinkRes.readlinkres_u.data<<"-"<<endl;
            QString linkDest(readLinkRes.readlinkres_u.data);
            atom.m_uds = KIO::UDS_LINK_DEST;
            atom.m_str = linkDest;
            entry.append( atom );
            if (linkDest[0]=='/')
            {
               kdDebug(7101)<<"an absolute link, stating locally...."<<endl;
            };
         }
         else
         {
            /*type = S_IFMT - 1;
            access = S_IRWXU | S_IRWXG | S_IRWXO;

            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long = type;
            entry.append( atom );

            atom.m_uds = KIO::UDS_ACCESS;
            atom.m_long = access;
            entry.append( atom );

            atom.m_uds = KIO::UDS_SIZE;
            atom.m_long = 0L;
            entry.append( atom );*/
         };
      }

      atom.m_uds = KIO::UDS_SIZE;
      atom.m_long = dirres.diropres_u.diropres.attributes.size;
      entry.append( atom );

      atom.m_uds = KIO::UDS_MODIFICATION_TIME;
      atom.m_long = dirres.diropres_u.diropres.attributes.mtime.seconds;
      entry.append( atom );

      atom.m_uds = KIO::UDS_ACCESS_TIME;
      atom.m_long = dirres.diropres_u.diropres.attributes.atime.seconds;
      entry.append( atom );

      atom.m_uds = KIO::UDS_CREATION_TIME;
      atom.m_long = dirres.diropres_u.diropres.attributes.ctime.seconds;
      entry.append( atom );

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = (dirres.diropres_u.diropres.attributes.mode & 07777);
      entry.append( atom );

      atom.m_uds = KIO::UDS_FILE_TYPE;
      atom.m_long =dirres.diropres_u.diropres.attributes.mode & S_IFMT; // extract file type
      entry.append( atom );
	
      atom.m_uds = KIO::UDS_USER;
      uid_t uid = dirres.diropres_u.diropres.attributes.uid;
      QString *temp = usercache.find( uid );
      if ( !temp )
      {
         struct passwd *user = getpwuid( uid );
         if ( user )
         {
            usercache.insert( uid, new QString(user->pw_name) );
            atom.m_str = user->pw_name;
         }
         else
            atom.m_str = "???";
      }
      else
         atom.m_str = *temp;
      entry.append( atom );

      atom.m_uds = KIO::UDS_GROUP;
      gid_t gid = dirres.diropres_u.diropres.attributes.gid;
      temp = groupcache.find( gid );
      if ( !temp )
      {
         struct group *grp = getgrgid( gid );
         if ( grp )
         {
            groupcache.insert( gid, new QString(grp->gr_name) );
            atom.m_str = grp->gr_name;
         }
         else
            atom.m_str = "???";
      }
      else
         atom.m_str = *temp;
      entry.append( atom );

      KIO::UDSEntry::ConstIterator it = entry.begin();
      for( ; it != entry.end(); it++ )
      {
         switch ((*it).m_uds)
         {
         case KIO::UDS_FILE_TYPE:
            kdDebug(7101) << "File Type : " << (mode_t)((*it).m_long) << endl;
            break;
         case KIO::UDS_ACCESS:
            kdDebug(7101) << "Access permissions : " << (mode_t)((*it).m_long) << endl;
            break;
         case KIO::UDS_USER:
            kdDebug(7101) << "User : " << ((*it).m_str.ascii() ) << endl;
            break;
         case KIO::UDS_GROUP:
            kdDebug(7101) << "Group : " << ((*it).m_str.ascii() ) << endl;
            break;
         case KIO::UDS_NAME:
            kdDebug(7101) << "Name : " << ((*it).m_str.ascii() ) << endl;
            //m_strText = decodeFileName( (*it).m_str );
            break;
         case KIO::UDS_URL:
            kdDebug(7101) << "URL : " << ((*it).m_str.ascii() ) << endl;
            break;
         case KIO::UDS_MIME_TYPE:
            kdDebug(7101) << "MimeType : " << ((*it).m_str.ascii() ) << endl;
            break;
         case KIO::UDS_LINK_DEST:
            kdDebug(7101) << "LinkDest : " << ((*it).m_str.ascii() ) << endl;
            break;
         }
      }

      //everything went fine up to now :-)
      //fh=dirres.diropres_u.diropres.file.data;
      listEntry( entry, false);

   };

   listEntry( entry, true ); // ready
   finished();

};

void NFSProtocol::createVirtualDirEntry(UDSEntry & entry)
{
   UDSAtom atom;

   atom.m_uds = KIO::UDS_FILE_TYPE;
   atom.m_long = S_IFDIR;
   entry.append( atom );

   atom.m_uds = KIO::UDS_ACCESS;
   atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
   entry.append( atom );

   atom.m_uds = KIO::UDS_USER;
   atom.m_str = "root";
   entry.append( atom );
   atom.m_uds = KIO::UDS_GROUP;
   entry.append( atom );

   //a dummy size
   atom.m_uds = KIO::UDS_SIZE;
   atom.m_long = 1024;
   entry.append( atom );
};

void NFSProtocol::stat( const KURL & url)
{
   QString path( QFile::encodeName(url.path()));
   stripTrailingSlash(path);
   kdDebug(7101)<<"NFS::stat for -"<<path<<"-"<<endl;

   // We can't stat root, but we know it's a dir.
   if ( path.isEmpty() || path == "/" || m_exportedDirs.find(path)!=m_exportedDirs.end())
   {
      UDSEntry entry;
      UDSAtom atom;

      atom.m_uds = KIO::UDS_NAME;
      atom.m_str = path;
      entry.append( atom );
      createVirtualDirEntry(entry);
      // no size
      statEntry( entry );
      finished();
      return;
   }

   NFSFileHandle fh=getFileHandle(path);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,"nfs_stat");
      return;
   };

   diropargs dirargs;
   attrstat attrAndStat;
   memcpy(dirargs.dir.data,fh,NFS_FHSIZE);
   strcpy(dirargs.name, QFile::encodeName(path).data());

   kdDebug(7101)<<"calling rpc: FH: -"<<fh<<"- with name -"<<dirargs.name<<"-"<<endl;

   int clnt_stat = clnt_call(m_client, NFSPROC_GETATTR,
                         (xdrproc_t) xdr_diropargs, (char*)&dirargs,
                         (xdrproc_t) xdr_attrstat, (char*)&attrAndStat,total_timeout);
   if (!checkForError(clnt_stat,attrAndStat.status,QString("stat ")+path)) return;
   UDSEntry entry;
   entry.clear();

   UDSAtom atom;

   atom.m_uds = KIO::UDS_NAME;
   atom.m_str = path;
   entry.append( atom );

      //the mode field of nfs is exactly the same as the one from the local stat()
//      mode_t type = (dirres.diropres_u.diropres.attributes.mode & S_IFMT); // extract file type
   //mode_t access = (attrAndStat.attrstat_u.attributes.mode & 07777); // extract permissions

   /*unsigned int type=dirres.diropres_u.diropres.attributes.type;
    if (type==NFLINK)
    {
    //check the link a bit more exactly

    };*/

   atom.m_uds = KIO::UDS_SIZE;
   atom.m_long = attrAndStat.attrstat_u.attributes.size;
   entry.append( atom );

   kdDebug(7101)<<"listing name: -"<<path<<"- size: "<<attrAndStat.attrstat_u.attributes.size<<endl;

   atom.m_uds = KIO::UDS_MODIFICATION_TIME;
   atom.m_long = attrAndStat.attrstat_u.attributes.mtime.seconds;
   entry.append( atom );

   atom.m_uds = KIO::UDS_ACCESS_TIME;
   atom.m_long = attrAndStat.attrstat_u.attributes.atime.seconds;
   entry.append( atom );

   atom.m_uds = KIO::UDS_CREATION_TIME;
   atom.m_long = attrAndStat.attrstat_u.attributes.ctime.seconds;
   entry.append( atom );

   atom.m_uds = KIO::UDS_ACCESS;
   atom.m_long = (attrAndStat.attrstat_u.attributes.mode & 07777);
   entry.append( atom );

   atom.m_uds = KIO::UDS_FILE_TYPE;
   atom.m_long =attrAndStat.attrstat_u.attributes.mode & S_IFMT; // extract file type
   entry.append( atom );

   atom.m_uds = KIO::UDS_USER;
   uid_t uid = attrAndStat.attrstat_u.attributes.uid;
   QString *temp = usercache.find( uid );
   if ( !temp )
   {
      struct passwd *user = getpwuid( uid );
      if ( user )
      {
         usercache.insert( uid, new QString(user->pw_name) );
         atom.m_str = user->pw_name;
      }
      else
         atom.m_str = "???";
   }
   else
      atom.m_str = *temp;
   entry.append( atom );

   atom.m_uds = KIO::UDS_GROUP;
   gid_t gid = attrAndStat.attrstat_u.attributes.gid;
   temp = groupcache.find( gid );
   if ( !temp )
   {
      struct group *grp = getgrgid( gid );
      if ( grp )
      {
         groupcache.insert( gid, new QString(grp->gr_name) );
         atom.m_str = grp->gr_name;
      }
      else
         atom.m_str = "???";
   }
   else
      atom.m_str = *temp;
   entry.append( atom );

   //everything went fine up to now :-)
   //fh=dirres.diropres_u.diropres.file.data;

   KIO::UDSEntry::ConstIterator it = entry.begin();
   for( ; it != entry.end(); it++ ) {
      switch ((*it).m_uds) {
      case KIO::UDS_FILE_TYPE:
         kdDebug(7101) << "File Type : " << (mode_t)((*it).m_long) << endl;
         break;
      case KIO::UDS_ACCESS:
         kdDebug(7101) << "Access permissions : " << (mode_t)((*it).m_long) << endl;
         break;
      case KIO::UDS_USER:
         kdDebug(7101) << "User : " << ((*it).m_str.ascii() ) << endl;
         break;
      case KIO::UDS_GROUP:
         kdDebug(7101) << "Group : " << ((*it).m_str.ascii() ) << endl;
         break;
      case KIO::UDS_NAME:
         kdDebug(7101) << "Name : " << ((*it).m_str.ascii() ) << endl;
         //m_strText = decodeFileName( (*it).m_str );
         break;
      case KIO::UDS_URL:
         kdDebug(7101) << "URL : " << ((*it).m_str.ascii() ) << endl;
         break;
      case KIO::UDS_MIME_TYPE:
         kdDebug(7101) << "MimeType : " << ((*it).m_str.ascii() ) << endl;
         break;
      case KIO::UDS_LINK_DEST:
         kdDebug(7101) << "LinkDest : " << ((*it).m_str.ascii() ) << endl;
         break;
      }
   }

   statEntry( entry );
   finished();
}

void NFSProtocol::setHost(const QString& host, int /*port*/, const QString& /*user*/, const QString& /*pass*/)
{
   kdDebug(7101)<<"NFS::setHost: -"<<host<<"-"<<endl;
   if (host.isEmpty())
   {
      error(ERR_UNSUPPORTED_ACTION,"");
      return;
   };
   if (host==m_currentHost) return;
   m_currentHost=host;
   m_handleCache.clear();
   m_exportedDirs.clear();
   //m_sock=RPC_ANYSOCK;
   m_client=0;
}

void NFSProtocol::mkdir( const KURL& url, int permissions )
{
   kdDebug(7101)<<"mkdir"<<endl;
   QString thePath( QFile::encodeName(url.path()));
   stripTrailingSlash(thePath);
   QString dirName, parentDir;
   getLastPart(thePath, dirName, parentDir);
   stripTrailingSlash(parentDir);
   kdDebug(7101)<<"path: -"<<thePath<<"- dir: -"<<dirName<<"- parentDir: -"<<parentDir<<"-"<<endl;
   NFSFileHandle fh=getFileHandle(parentDir);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("mkdir ")+thePath);
      return;
   };

   createargs createArgs;
   memcpy(createArgs.where.dir.data,fh,NFS_FHSIZE);
   strcpy(createArgs.where.name, QFile::encodeName(dirName).data());
   createArgs.attributes.mode=permissions;

   diropres dirres;

   int clnt_stat = clnt_call(m_client, NFSPROC_MKDIR,
                         (xdrproc_t) xdr_createargs, (char*)&createArgs,
                         (xdrproc_t) xdr_diropres, (char*)&dirres,total_timeout);
   if (!checkForError(clnt_stat,dirres.status,QString("mkdir ")+thePath)) return;
   finished();
}

bool NFSProtocol::checkForError(int clientStat, int nfsStat, const QString& text)
{
   if (clientStat!=RPC_SUCCESS)
   {
      kdDebug(7101)<<"rpc error: "<<clientStat<<endl;
      error(ERR_CONNECTION_BROKEN,"");
      return FALSE;
   };
   if (nfsStat!=NFS_OK)
   {
      kdDebug(7101)<<"nfs error: "<<nfsStat<<endl;
      if (nfsStat==NFSERR_ACCES) error(ERR_ACCESS_DENIED,QString("checkForError: ")+text);
      else if (nfsStat==NFSERR_NOENT) error(ERR_DOES_NOT_EXIST,QString("checkForError: ")+text);
      else if (nfsStat==NFSERR_EXIST) error(ERR_FILE_ALREADY_EXIST,QString("checkForError: ")+text);
      else if (nfsStat==NFSERR_ISDIR) error(ERR_IS_DIRECTORY,QString("checkForError: ")+text);
      else error(ERR_CONNECTION_BROKEN,QString("checkForError")+text);
      return FALSE;
   };
   return TRUE;
};

void NFSProtocol::del( const KURL& url, bool isfile)
{
   QString thePath( QFile::encodeName(url.path()));
   stripTrailingSlash(thePath);

   QString fileName, parentDir;
   getLastPart(thePath, fileName, parentDir);
   stripTrailingSlash(parentDir);
   kdDebug(7101)<<"path: -"<<thePath<<"- file -"<<fileName<<"- parentDir: -"<<parentDir<<"-"<<endl;
   NFSFileHandle fh=getFileHandle(parentDir);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("del ")+thePath);
      return;
   };

   if (isfile)
   {
      kdDebug( 7101 ) <<  "Deleting file "<< thePath << endl;
      diropargs dirOpArgs;
      memcpy(dirOpArgs.dir.data,fh,NFS_FHSIZE);
      strcpy(dirOpArgs.name, QFile::encodeName(fileName).data());

      nfsstat nfsStat;

      int clnt_stat = clnt_call(m_client, NFSPROC_REMOVE,
                            (xdrproc_t) xdr_diropargs, (char*)&dirOpArgs,
                            (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
      if (!checkForError(clnt_stat,nfsStat,QString("del ")+thePath)) return;
      kdDebug(7101)<<"removing "<<thePath<<" from cache"<<endl;
      m_handleCache.remove(m_handleCache.find(thePath));
      if (m_handleCache.find(thePath)!=m_handleCache.end()) kdDebug(7101)<<"fuck, it's still there"<<endl;
      finished();
   }
   else
   {
      kdDebug( 7101 ) <<  "Deleting directory "<< thePath << endl;
      diropargs dirOpArgs;
      memcpy(dirOpArgs.dir.data,fh,NFS_FHSIZE);
      strcpy(dirOpArgs.name, QFile::encodeName(fileName).data());

      nfsstat nfsStat;

      int clnt_stat = clnt_call(m_client, NFSPROC_RMDIR,
                            (xdrproc_t) xdr_diropargs, (char*)&dirOpArgs,
                            (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
      if (!checkForError(clnt_stat,nfsStat,QString("del ")+thePath)) return;
      kdDebug(7101)<<"removing "<<thePath<<" from cache"<<endl;
      m_handleCache.remove(m_handleCache.find(thePath));
      if (m_handleCache.find(thePath)!=m_handleCache.end()) kdDebug(7101)<<"fuck, it's still there"<<endl;
      finished();
   };
};

void NFSProtocol::chmod( const KURL& url, int permissions )
{
   QString thePath( QFile::encodeName(url.path()));
   kdDebug( 7101 ) <<  "chmod -"<< thePath << "-"<<endl;
   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("chmod ")+thePath);
      return;
   };

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
   if (!checkForError(clnt_stat,nfsStat,QString("chmod ")+thePath)) return;

   finished();
}

void NFSProtocol::get( const KURL& url, bool /* reload */)
{
   QString thePath( QFile::encodeName(url.path()));
   kdDebug(7101) <<  "getting -"<< thePath <<"-"<< endl;
   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("get ")+thePath);
      return;
   };
   readargs readArgs;
   memcpy(readArgs.file.data,fh,NFS_FHSIZE);
   readArgs.offset=0;
   readArgs.count=NFS_MAXDATA;
   readArgs.totalcount=NFS_MAXDATA;
   readres readRes;
   int offset(0);
   speed( 1000 );
   kdDebug(7101)<<"vor clnt_call"<<endl;
   char buf[NFS_MAXDATA];
   readRes.readres_u.reply.data.data_val=buf;

   time_t t_start = time( 0L );
   time_t t_last = t_start;

   QByteArray array;
   do
   {
      int clnt_stat = clnt_call(m_client, NFSPROC_READ,
                            (xdrproc_t) xdr_readargs, (char*)&readArgs,
                            (xdrproc_t) xdr_readres, (char*)&readRes,total_timeout);
      if (!checkForError(clnt_stat,readRes.status,QString("get ")+thePath)) return;
      if (readArgs.offset==0)
         totalSize(readRes.readres_u.reply.attributes.size);

      offset=readRes.readres_u.reply.data.data_len;
      kdDebug(7101)<<"read "<<offset<<" bytes"<<endl;
      readArgs.offset+=offset;
      if (offset>0)
      {
         array.setRawData(readRes.readres_u.reply.data.data_val, offset);
         data( array );
         array.resetRawData(readRes.readres_u.reply.data.data_val, offset);

         time_t t = time( 0L );
         if ( t - t_last >= 1 )
         {
            processedSize(readArgs.offset);
            speed(readArgs.offset/(t-t_start));
            t_last = t;
         }
      };

   } while (offset>0);
   data( QByteArray() );
   finished();
};

//this one already looks quite good, I think
void NFSProtocol::put( const KURL& url, int _mode, bool _overwrite, bool /*_resume*/ )
{
    QString destPath( QFile::encodeName(url.path()));
    kdDebug( 7101 ) << "Put -" << destPath <<"-"<<endl;
    /*QString dest_part( dest_orig );
    dest_part += ".part";*/

    stripTrailingSlash(destPath);
    NFSFileHandle destFH;
    destFH=getFileHandle(destPath);
    kdDebug(7101)<<"file handle for -"<<destPath<<"- is "<<destFH<<endl;
    if (destFH.isInvalid()) kdDebug(7101)<<"file handle is invalid"<<endl;
    else kdDebug(7101)<<"file handle is valid :-)"<<endl;

    //the file exists and we don't want to overwrite
    if ((!_overwrite) && (!destFH.isInvalid()))
    {
       error(ERR_FILE_ALREADY_EXIST,"");
       return;
    };
    //TODO: is this correct ?
    //we have to "create" the file anyway, no matter if it already
    //exists or not
    //if we don't create it new, written text will be, hmm, "inserted"
    //in the existing file, i.e. a file could not become smaller, since
    //write only overwrites or extends, but doesn't remove stuff from a file (aleXXX)

    QString parentDir, fileName;
    getLastPart(destPath,fileName, parentDir);
    kdDebug(7101)<<"creating the file -"<<fileName<<"-"<<endl;
    NFSFileHandle parentFH;
    parentFH=getFileHandle(parentDir);
    cerr<<"fh for parent dir: "<<parentFH<<endl;
    //the directory doesn't exist
    if (parentFH.isInvalid())
    {
       kdDebug(7101)<<"parent directory -"<<parentDir<<"- does not exist"<<endl;
       error(ERR_DOES_NOT_EXIST,QString("put ")+parentDir);
       return;
    };
    createargs createArgs;
    memcpy(createArgs.where.dir.data,(const char*)parentFH,NFS_FHSIZE);
    strcpy(createArgs.where.name, QFile::encodeName(fileName).data());

    //the mode is apparently ignored if the file already exists
    if (_mode==-1) createArgs.attributes.mode=0644;
    else createArgs.attributes.mode=_mode;
    createArgs.attributes.uid=geteuid();
    kdDebug(7101)<<"effective uid: "<<geteuid()<<endl;
    kdDebug(7101)<<"effective gid: "<<getegid()<<endl;
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
    if (!checkForError(clnt_stat,dirOpRes.status,QString("put create ")+fileName)) return;
    //we created the file successfully
    //destFH=getFileHandle(destPath);
    destFH=dirOpRes.diropres_u.diropres.file.data;
    kdDebug(7101)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully"<<endl;
    cerr<<"with fh "<<destFH<<endl;

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
    kdDebug(7101)<<"starting to put"<<endl;
    do
    {
       QByteArray buffer;
       dataReq(); // Request for data
       result = readData( buffer );
       kdDebug(7101)<<"received "<<result<<" bytes for putting"<<endl;
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
             kdDebug(7101)<<"written"<<endl;
             if (!checkForError(clnt_stat,attrStat.status,QString("put write ")+fileName)) return;
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
};

void NFSProtocol::rename( const KURL &src, const KURL &dest, bool _overwrite )
{
   QString srcPath( QFile::encodeName(src.path()));
   QString destPath( QFile::encodeName(dest.path()));
   kdDebug(7101)<<"renaming -"<<srcPath<<"- to -"<<destPath<<"-"<<endl;
   stripTrailingSlash(srcPath);
   stripTrailingSlash(destPath);
   if (!_overwrite)
   {
      NFSFileHandle testFH;
      testFH=getFileHandle(destPath);
      if (!testFH.isInvalid())
      {
         error(ERR_FILE_ALREADY_EXIST,"");
         return;
      };
   };

   if (srcPath.isEmpty() || srcPath=="/" || m_exportedDirs.find(srcPath)!=m_exportedDirs.end())
   {
      error(ERR_CANNOT_RENAME,"");
      return;
   };
   QString srcFileName, srcParentDir, destFileName, destParentDir;

   getLastPart(srcPath, srcFileName, srcParentDir);
   NFSFileHandle srcFH=getFileHandle(srcParentDir);
   if (srcFH.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("rename ")+srcParentDir);
      return;
   };
   renameargs renameArgs;
   memcpy(renameArgs.from.dir.data,srcFH,NFS_FHSIZE);
   strcpy(renameArgs.from.name, QFile::encodeName(srcFileName).data());

   getLastPart(destPath, destFileName, destParentDir);
   NFSFileHandle destFH=getFileHandle(destParentDir);
   if (destFH.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("rename ")+destParentDir);
      return;
   };
   memcpy(renameArgs.to.dir.data,destFH,NFS_FHSIZE);
   strcpy(renameArgs.to.name, QFile::encodeName(destFileName).data());
   nfsstat nfsStat;

   int clnt_stat = clnt_call(m_client, NFSPROC_RENAME,
                             (xdrproc_t) xdr_renameargs, (char*)&renameArgs,
                             (xdrproc_t) xdr_nfsstat, (char*)&nfsStat,total_timeout);
   if (!checkForError(clnt_stat,nfsStat,QString("rename ")+destPath)) return;
   finished();
};

void NFSProtocol::copy( const KURL &src, const KURL &dest, int _mode, bool _overwrite )
{
   //prepare the source
   QString thePath( QFile::encodeName(src.path()));
   kdDebug( 7101 ) << "Copy to -" << thePath <<"-"<<endl;
   NFSFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,QString("copy ")+thePath);
      return;
   };

   //create the destination
   QString destPath( QFile::encodeName(dest.path()));
   stripTrailingSlash(destPath);
   NFSFileHandle destFH;
   destFH=getFileHandle(destPath);
   kdDebug(7101)<<"file handle for -"<<destPath<<"- is "<<destFH<<endl;
   if (destFH.isInvalid()) kdDebug(7101)<<"file handle is invalid"<<endl;
   else kdDebug(7101)<<"file handle is valid :-)"<<endl;

   //the file exists and we don't want to overwrite
   if ((!_overwrite) && (!destFH.isInvalid()))
   {
      error(ERR_FILE_ALREADY_EXIST,"");
      return;
   };
   //TODO: is this correct ?
   //we have to "create" the file anyway, no matter if it already
   //exists or not
   //if we don't create it new, written text will be, hmm, "inserted"
   //in the existing file, i.e. a file could not become smaller, since
   //write only overwrites or extends, but doesn't remove stuff from a file

   QString parentDir, fileName;
   getLastPart(destPath,fileName, parentDir);
   kdDebug(7101)<<"creating the file -"<<fileName<<"-"<<endl;
   NFSFileHandle parentFH;
   parentFH=getFileHandle(parentDir);
   //the directory doesn't exist
   if (parentFH.isInvalid())
   {
      kdDebug(7101)<<"parent directory -"<<parentDir<<"- does not exist"<<endl;
      error(ERR_DOES_NOT_EXIST,QString("copy ")+parentDir);
      return;
   };
   createargs createArgs;
   memcpy(createArgs.where.dir.data,(const char*)parentFH,NFS_FHSIZE);
   strcpy(createArgs.where.name, QFile::encodeName(fileName).data());
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
   if (!checkForError(clnt_stat,dirOpRes.status,QString("copy create ")+destPath)) return;
   //we created the file successfully
   destFH=dirOpRes.diropres_u.diropres.file.data;
   kdDebug(7101)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully"<<endl;

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
      if (!checkForError(clnt_stat,readRes.status,QString("copy read ")+thePath)) return;
      if (readArgs.offset==0)
         totalSize(readRes.readres_u.reply.attributes.size);

      bytesRead=readRes.readres_u.reply.data.data_len;
      kdDebug(7101)<<"read "<<bytesRead<<" bytes"<<endl;
      //then write
      if (bytesRead>0)
      {
         readArgs.offset+=bytesRead;

         writeArgs.data.data_len=bytesRead;

         clnt_stat = clnt_call(m_client, NFSPROC_WRITE,
                               (xdrproc_t) xdr_writeargs, (char*)&writeArgs,
                               (xdrproc_t) xdr_attrstat, (char*)&attrStat,total_timeout);
         kdDebug(7101)<<"written"<<endl;
         if (!checkForError(clnt_stat,attrStat.status,QString("copy write ")+destPath)) return;
         writeArgs.offset+=bytesRead;
      };
   } while (bytesRead>0);

   finished();
}
