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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <qtextstream.h>
#include <qcstring.h>
#include <qfile.h>

#include "kio_floppy.h"

#include <kinstance.h>
#include <kdebug.h>
#include <kio/global.h>
#include <klocale.h>

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_floppy" );

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_floppy protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }
  kdDebug(7101) << "Floppy: kdemain: starting" << endl;

  FloppyProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  return 0;
}

void getDriveAndPath(const QString& path, QString& drive, QString& rest)
{
   drive="";
   rest="";
   QStringList list=QStringList::split("/",path);
   for (QStringList::Iterator it=list.begin(); it!=list.end(); it++)
   {
      if (it==list.begin())
         drive=(*it)+":";
      else
         rest=rest+"/"+(*it);
   };
};

FloppyProtocol::FloppyProtocol (const QCString &pool, const QCString &app )
:SlaveBase( "floppy", pool, app )
,m_mtool(0)
,m_stdoutBuffer(0)
,m_stderrBuffer(0)
,m_stdoutSize(0)
,m_stderrSize(0)
{
   kdDebug(7101)<<"Floppy::Floppy: -"<<pool<<"-"<<endl;
};

FloppyProtocol::~FloppyProtocol()
{
   if (m_stdoutBuffer!=0)
      delete [] m_stdoutBuffer;
   if (m_stderrBuffer!=0)
      delete [] m_stderrBuffer;
   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   m_mtool=0;
   m_stdoutBuffer=0;
   m_stderrBuffer=0;
};

int FloppyProtocol::readStdout()
{
   kdDebug(7101)<<"Floppy::readStdout"<<endl;
   if (m_mtool==0) return 0;

   char buffer[16*1024];
   int length=::read(m_mtool->stdoutFD(),buffer,16*1024);
   if (length<=0) return 0;

   //+1 gives us room for a terminating 0
   char *newBuffer=new char[length+m_stdoutSize+1];
   kdDebug(7101)<<"Floppy::readStdout(): length: "<<length<<" m_tsdoutSize: "<<m_stdoutSize<<" +1="<<length+m_stdoutSize+1<<endl;
   if (m_stdoutBuffer!=0)
   {
      memcpy(newBuffer, m_stdoutBuffer, m_stdoutSize);
   };
   memcpy(newBuffer+m_stdoutSize, buffer, length);
   m_stdoutSize+=length;
   newBuffer[m_stdoutSize]='\0';

   if (m_stdoutBuffer!=0)
   {
      delete [] m_stdoutBuffer;
   };
   m_stdoutBuffer=newBuffer;
   //kdDebug(7101)<<"Floppy::readStdout(): -"<<m_stdoutBuffer<<"-"<<endl;

   kdDebug(7101)<<"Floppy::readStdout ends"<<endl;
   return length;
};

int FloppyProtocol::readStderr()
{
   kdDebug(7101)<<"Floppy::readStderr"<<endl;
   if (m_mtool==0) return 0;

   char buffer[16*1024];
   int length=::read(m_mtool->stderrFD(),buffer,16*1024);
   kdDebug(7101)<<"Floppy::readStderr(): read "<<length<<" bytes"<<endl;
   if (length==0) return 0;

   //+1 gives us room for a terminating 0
   char *newBuffer=new char[length+m_stderrSize+1];
   memcpy(newBuffer, m_stderrBuffer, m_stderrSize);
   memcpy(newBuffer+m_stderrSize, buffer, length);
   m_stderrSize+=length;
   newBuffer[m_stderrSize]='\0';
   delete [] m_stderrBuffer;
   m_stderrBuffer=newBuffer;
   kdDebug(7101)<<"Floppy::readStderr(): -"<<m_stderrBuffer<<"-"<<endl;

   kdDebug(7101)<<"Floppy::readStdout ends"<<endl;
   return length;
};

void FloppyProtocol::clearBuffers()
{
   kdDebug(7101)<<"Floppy::clearBuffers()"<<endl;
   m_stdoutSize=0;
   m_stderrSize=0;
   if (m_stdoutBuffer!=0)
      delete [] m_stdoutBuffer;
   m_stdoutBuffer=0;
   if (m_stderrBuffer!=0)
      delete [] m_stderrBuffer;
   m_stderrBuffer=0;
   kdDebug(7101)<<"Floppy::clearBuffers() ends"<<endl;
};

void FloppyProtocol::terminateBuffers()
{
   kdDebug(7101)<<"Floppy::terminateBuffers()"<<endl;
   //append a terminating 0 to be sure
   if (m_stdoutBuffer!=0)
   {
      m_stdoutBuffer[m_stdoutSize]='\0';
   };
   if (m_stderrBuffer!=0)
   {
      m_stderrBuffer[m_stderrSize]='\0';
   };
   kdDebug(7101)<<"Floppy::terminateBuffers() ends"<<endl;
};

void FloppyProtocol::listDir( const KURL& _url)
{
   kdDebug(7101)<<"Floppy::listDir()"<<endl;
   KURL url(_url);
   QString path( QFile::encodeName(url.path()));

   kdDebug(7101)<<"Floppy::listDir() path -"<<path.latin1()<<"-"<<endl;

   if ((path.isEmpty()) || (path=="/"))
   {
      url.setPath("/a/");
      redirection(url);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   getDriveAndPath(path,drive,floppyPath);

   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   args<<"mdir"<<drive;
   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   m_mtool=new Program(args);

   clearBuffers();

   kdDebug(7101)<<"Floppy::listDir(): starting m_mtool"<<endl;
   m_mtool->start();

   int result;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
         if (readStdout()==0)
            loopFinished=true;
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;
   //now mdir has finished
   //let's parse the output
   terminateBuffers();

   if (m_stderrSize!=0)
   {
      error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, "f*ck");
      return;
   };

   QString outputString(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;

   int totalNumber(0);
   int mode(0);
   UDSEntry entry;

   while (!output.atEnd())
   {
      line=output.readLine();
      kdDebug(7101)<<"Floppy::listDir(): line: -"<<line<<"- length: "<<line.length()<<endl;

      if (mode==0) 
      {
         if (line.isEmpty())
         {
            kdDebug(7101)<<"Floppy::listDir(): switching to mode 1"<<endl;
            mode=1;
         };
      }
      else if (mode==1)
      {
         if (line[0]==' ')
         {
            kdDebug(7101)<<"Floppy::listDir(): ende"<<endl;
            totalSize(totalNumber);
            break;
         };
         entry.clear();
         if (createUDSEntry(line,entry))
         {
            //kdDebug(7101)<<"Floppy::listDir(): creating UDSEntry"<<endl;
            listEntry( entry, false);
            totalNumber++;
         };
      };
   };
   listEntry( entry, true ); // ready
   finished();
   kdDebug(7101)<<"Floppy::listDir() ends"<<endl;
};

bool FloppyProtocol::createUDSEntry(const QString line, UDSEntry& entry, bool makeStat, const QString& dirName)
{
   //kdDebug(7101)<<"Floppy::createUDSEntry()"<<endl;
   QString name;
   QString size;
   bool isDir(false);
   QString day,month, year;
   QString hour, minute;

   static QDateTime beginningOfTimes(QDate(1970,1,1),QTime(1,0));

   UDSAtom atom;
   if (line.length()==41)
   {
      int nameLength=line.find(' ');
      if (nameLength>0)
      {
         name=line.mid(0,nameLength);
         QString ext=line.mid(9,3);
         ext=ext.stripWhiteSpace();
         if (!ext.isEmpty())
            name+="."+ext;
      };
      kdDebug(7101)<<"Floppy::createUDSEntry() name 8.3= -"<<name<<"-"<<endl;
   }
   else if (line.length()>41)
   {
      name=line.mid(42);
      kdDebug(7101)<<"Floppy::createUDSEntry() name vfat: -"<<name<<"-"<<endl;
   };
   if (((name==".") || (name=="..")) && (makeStat))
   {
      if (makeStat)
         name=dirName;
      else
         return false;
   };

   if (line.mid(13,5)=="<DIR>")
   {
      //kdDebug(7101)<<"Floppy::createUDSEntry() isDir"<<endl;
      size="1024";
      isDir=true;
   }
   else
   {
      size=line.mid(13,9);
      //kdDebug(7101)<<"Floppy::createUDSEntry() size: -"<<size<<"-"<<endl;
   };

   day=line.mid(26,2);
   month=line.mid(23,2);
   year=line.mid(29,4);
   hour=line.mid(35,2);
   minute=line.mid(38,2);
   //kdDebug(7101)<<"Floppy::createUDSEntry() day: -"<<day<<"-"<<month<<"-"<<year<<"- -"<<hour<<"-"<<minute<<"-"<<endl;

   if (name.isEmpty())
      return false;

   atom.m_uds = KIO::UDS_NAME;
   atom.m_str = name;
   entry.append( atom );

   atom.m_uds = KIO::UDS_SIZE;
   atom.m_long = size.toInt();
   entry.append(atom);

   QDateTime date(QDate(year.toInt(),month.toInt(),day.toInt()),QTime(hour.toInt(),minute.toInt()));
   atom.m_uds = KIO::UDS_MODIFICATION_TIME;
   atom.m_long = beginningOfTimes.secsTo(date);
   entry.append( atom );

   atom.m_uds = KIO::UDS_ACCESS;
   if (isDir)
      atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH| S_IWOTH|S_IWGRP|S_IWUSR  ;
   else
      atom.m_long = S_IRUSR | S_IRGRP | S_IROTH| S_IWOTH|S_IWGRP|S_IWUSR;
   entry.append( atom );

   atom.m_uds = KIO::UDS_FILE_TYPE;
   atom.m_long =(isDir?S_IFDIR:S_IFREG);
   entry.append( atom );

   //kdDebug(7101)<<"Floppy::createUDSEntry() ends"<<endl;
   return true;
};

void FloppyProtocol::stat( const KURL & _url)
{
   kdDebug(7101)<<"Floppy::stat()"<<endl;
   KURL url(_url);
   QString path( QFile::encodeName(url.path()));

   if ((path.isEmpty()) || (path=="/"))
   {
      url.setPath("/a/");
      redirection(url);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   kdDebug(7101)<<"Floppy::stat(): before getDriveAndPath"<<endl;
   getDriveAndPath(path,drive,floppyPath);
   UDSEntry entry;
   UDSAtom atom;

   if (floppyPath.isEmpty())
   {
      kdDebug(7101)<<"Floppy::stat(): floppyPath.isEmpty()"<<endl;
      atom.m_uds = KIO::UDS_NAME;
      atom.m_str = path;
      entry.append( atom );

      kdDebug(7101)<<"Floppy::stat(): setting size"<<endl;
      atom.m_uds = KIO::UDS_SIZE;
      atom.m_long = 1024;
      entry.append(atom);

      atom.m_uds = KIO::UDS_MODIFICATION_TIME;
      atom.m_long = 0;
      entry.append( atom );

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH| S_IWOTH|S_IWGRP|S_IWUSR;
      entry.append( atom );

      atom.m_uds = KIO::UDS_FILE_TYPE;
      atom.m_long =S_IFDIR;
      entry.append( atom );

      statEntry( entry );
      finished();
      kdDebug(7101)<<"Floppy::stat(): ends"<<endl;
      return;
   };
   kdDebug(7101)<<"Floppy::stat(): delete m_mtool"<<endl;
   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   args<<"mdir"<<drive;

   kdDebug(7101)<<"Floppy::stat(): create m_mtool"<<endl;
   m_mtool=new Program(args);

   kdDebug(7101)<<"Floppy::stat(): start m_mtool"<<endl;
   m_mtool->start();

   kdDebug(7101)<<"Floppy::stat(): clearBuffers()"<<endl;
   clearBuffers();

   int result;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
         if (readStdout()==0)
            loopFinished=true;
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   kdDebug(7101)<<"Floppy::stat(): delete m_mtool"<<endl;
   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;
   //now mdir has finished
   //let's parse the output
   kdDebug(7101)<<"Floppy::stat(): terminateBuffers()"<<endl;
   terminateBuffers();

   if (m_stdoutSize==0)
   {
      error( KIO::ERR_DOES_NOT_EXIST, url.path(-1) );
      return;
   }


   kdDebug(7101)<<"Floppy::stat(): parse stuff"<<endl;
   QString outputString(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;
   int lineNumber(0);
   while (!output.atEnd())
   {
      line=output.readLine();
      if (lineNumber==4)
      {
         createUDSEntry(line,entry,true,url.fileName());
         statEntry( entry );
         finished();
         kdDebug(7101)<<"Floppy::stat() ends"<<endl;
         return;
      };
      lineNumber++;
   };
   finished();
}

void FloppyProtocol::mkdir( const KURL& url, int)
{
   kdDebug(7101)<<"FloppyProtocol::mkdir"<<endl;
   QString path( QFile::encodeName(url.path()));

   if ((path.isEmpty()) || (path=="/"))
   {
      KURL newUrl(url);
      newUrl.setPath("/a/");
      redirection(newUrl);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   getDriveAndPath(path,drive,floppyPath);
   if (floppyPath.isEmpty())
   {
      finished();
      return;
   };
   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   //kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   args<<"mmd"<<drive;
   kdDebug(7101)<<"Floppy::mkdir(): executing: mmd -"<<drive<<"-"<<endl;

   m_mtool=new Program(args);
   m_mtool->start();

   clearBuffers();
   int result;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
         if (readStdout()==0)
            loopFinished=true;
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;
   terminateBuffers();

   finished();
}

void FloppyProtocol::del( const KURL& url, bool isfile)
{
   kdDebug(7101)<<"FloppyProtocol::del"<<endl;
   QString path( QFile::encodeName(url.path()));

   if ((path.isEmpty()) || (path=="/"))
   {
      KURL newUrl(url);
      newUrl.setPath("/a/");
      redirection(newUrl);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   getDriveAndPath(path,drive,floppyPath);
   if (floppyPath.isEmpty())
   {
      finished();
      return;
   };

   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   //kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   if (isfile)
      args<<"mdel"<<drive;
   else
      args<<"mrd"<<drive;

   kdDebug(7101)<<"Floppy::del(): executing: mrd -"<<drive<<"-"<<endl;

   m_mtool=new Program(args);
   m_mtool->start();

   clearBuffers();
   int result;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
         if (readStdout()==0)
            loopFinished=true;
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;
   terminateBuffers();

   finished();
};

void FloppyProtocol::rename( const KURL &src, const KURL &dest, bool _overwrite )
{
   QString srcPath( QFile::encodeName(src.path()));
   QString destPath( QFile::encodeName(dest.path()));

   kdDebug(7101)<<"renaming -"<<srcPath<<"- to -"<<destPath<<"-"<<endl;
      
   if ((srcPath.isEmpty()) || (srcPath=="/"))
      srcPath="/a/";

   if ((destPath.isEmpty()) || (destPath=="/"))
      destPath="/a/";

   QString srcDrive;
   QString srcFloppyPath;
   getDriveAndPath(srcPath,srcDrive,srcFloppyPath);
   if (srcFloppyPath.isEmpty())
   {
      finished();
      return;
   };

   QString destDrive;
   QString destFloppyPath;
   getDriveAndPath(destPath,destDrive,destFloppyPath);
   if (destFloppyPath.isEmpty())
   {
      finished();
      return;
   };

   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   //kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!srcFloppyPath.isEmpty())
      srcDrive+=srcFloppyPath;

   if (!destFloppyPath.isEmpty())
      destDrive+=destFloppyPath;

   args<<"mren"<<srcDrive<<destDrive;

   kdDebug(7101)<<"Floppy::move(): executing: mrenmrd -"<<srcDrive<<"  "<<destDrive<<endl;

   m_mtool=new Program(args);
   m_mtool->start();

   clearBuffers();
   int result;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
         if (readStdout()==0)
            loopFinished=true;
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;
   terminateBuffers();

   finished();
};

void FloppyProtocol::get( const KURL& url )
{
   QString path( QFile::encodeName(url.path()));
   kdDebug(7101)<<"get() -"<<path<<"-"<<endl;

   if ((path.isEmpty()) || (path=="/"))
   {
      KURL newUrl(url);
      newUrl.setPath("/a/");
      redirection(newUrl);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   getDriveAndPath(path,drive,floppyPath);
   if (floppyPath.isEmpty())
   {
      finished();
      return;
   };

   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   //kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   args<<"mcopy"<<drive<<"-";

   kdDebug(7101)<<"Floppy::get(): executing: mcopy -"<<drive<<"-"<<endl;

   m_mtool=new Program(args);
   m_mtool->start();

   time_t t_start = time( 0L );
   time_t t_last = t_start;

   clearBuffers();
   int result;
   int bytesRead(0);
   QByteArray array;
   bool loopFinished(false);
   do
   {
      bool stdoutEvent;
      bool stderrEvent;
      result=m_mtool->select(1,0,stdoutEvent, stderrEvent);
      if (stdoutEvent)
      {
         if (m_stdoutBuffer!=0)
            delete [] m_stdoutBuffer;
         m_stdoutBuffer=0;
         m_stdoutSize=0;
         if (readStdout()>0)
         {
            kdDebug(7101)<<"Floppy::get(): m_stdoutSize:"<<m_stdoutSize<<endl;
            bytesRead+=m_stdoutSize;
            array.setRawData(m_stdoutBuffer, m_stdoutSize);
            data( array );
            array.resetRawData(m_stdoutBuffer, m_stdoutSize);

            time_t t = time( 0L );
            if ( t - t_last >= 1 )
            {
               processedSize(bytesRead);
               speed(bytesRead/(t-t_start));
               t_last = t;
            }
         }
         else
         {
            loopFinished=true;
         }
      };
      if (stderrEvent)
         if (readStderr()==0)
            loopFinished=true;
   } while (!loopFinished);

   kdDebug(7101)<<"Floppy::get(): deleting m_mtool"<<endl;
   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;

   kdDebug(7101)<<"Floppy::get(): finishing"<<endl;
   data( QByteArray() );
   finished();
};

void FloppyProtocol::put( const KURL& url, int , bool , bool )
{
   QString path( QFile::encodeName(url.path()));
   kdDebug(7101)<<"put() -"<<path<<"-"<<endl;

   if ((path.isEmpty()) || (path=="/"))
   {
      KURL newUrl(url);
      newUrl.setPath("/a/");
      redirection(newUrl);
      finished();
      return;
   };
   QString drive;
   QString floppyPath;
   getDriveAndPath(path,drive,floppyPath);
   if (floppyPath.isEmpty())
   {
      finished();
      return;
   };

   if (m_mtool!=0)
   {
      m_mtool->closeFDs();
      delete m_mtool;
   };
   //kdDebug(7101)<<"Floppy::stat(): create args"<<endl;
   QStringList args;
   if (!floppyPath.isEmpty())
      drive+=floppyPath;
   args<<"mcopy"<<"-"<<drive;

   kdDebug(7101)<<"Floppy::put(): executing: mcopy -"<<drive<<"-"<<endl;

   m_mtool=new Program(args);
   m_mtool->start();

   clearBuffers();
   int result;
   int bytesRead(0);
   QByteArray array;

   //from file.cc
   // Loop until we got 0 (end of data)
   do
   {
      QByteArray buffer;
      dataReq(); // Request for data
      kdDebug(7101)<<"Floppy::put(): after dataReq()"<<endl;
      result = readData( buffer );
      kdDebug(7101)<<"Floppy::put(): after readData(), read "<<result<<" bytes"<<endl;
      if (result > 0)
      {
         kdDebug(7101)<<"Floppy::put(): writing..."<<endl;
         result=::write(m_mtool->stdinFD(),buffer.data(), buffer.size());
         kdDebug(7101)<<"Floppy::put(): after write(), wrote "<<result<<" bytes"<<endl;
      }
   }
   while ( result > 0 );
   if (result<0)
   {
      perror("writing to stdin");
      error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, "f*ck");
      return;
   };

   kdDebug(7101)<<"Floppy::put(): deleting m_mtool"<<endl;
   m_mtool->closeFDs();
   delete m_mtool;
   m_mtool=0;

   finished();
};

//TODO the partial putting thing is not yet implemented
/*void FloppyProtocol::put( const KURL& url, int _mode, bool _overwrite, bool )
{
    QString destPath( QFile::encodeName(url.path()));
    kdDebug( 7101 ) << "Put -" << destPath <<"-"<<endl;

    stripTrailingSlash(destPath);
    QString parentDir, fileName;
    getLastPart(destPath,fileName, parentDir);
    if (isRoot(parentDir))
    {
       error(ERR_WRITE_ACCESS_DENIED,destPath);
       return;
    };

    FloppyFileHandle destFH;
    destFH=getFileHandle(destPath);
    kdDebug(7101)<<"file handle for -"<<destPath<<"- is "<<destFH<<endl;

    //the file exists and we don't want to overwrite
    if ((!_overwrite) && (!destFH.isInvalid()))
    {
       error(ERR_FILE_ALREADY_EXIST,destPath);
       return;
    };
    //TODO: is this correct ?
    //we have to "create" the file anyway, no matter if it already
    //exists or not
    //if we don't create it new, written text will be, hmm, "inserted"
    //in the existing file, i.e. a file could not become smaller, since
    //write only overwrites or extends, but doesn't remove stuff from a file (aleXXX)

    kdDebug(7101)<<"creating the file -"<<fileName<<"-"<<endl;
    FloppyFileHandle parentFH;
    parentFH=getFileHandle(parentDir);
    //cerr<<"fh for parent dir: "<<parentFH<<endl;
    //the directory doesn't exist
    if (parentFH.isInvalid())
    {
       kdDebug(7101)<<"parent directory -"<<parentDir<<"- does not exist"<<endl;
       error(ERR_DOES_NOT_EXIST,parentDir);
       return;
    };
    createargs createArgs;
    memcpy(createArgs.where.dir.data,(const char*)parentFH,Floppy_FHSIZE);
    QCString tmpName=QFile::encodeName(fileName);
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
    int clnt_stat = clnt_call(m_client, FloppyPROC_CREATE,
                              (xdrproc_t) xdr_createargs, (char*)&createArgs,
                              (xdrproc_t) xdr_diropres, (char*)&dirOpRes,total_timeout);
    if (!checkForError(clnt_stat,dirOpRes.status,fileName)) return;
    //we created the file successfully
    //destFH=getFileHandle(destPath);
    destFH=dirOpRes.diropres_u.diropres.file.data;
    kdDebug(7101)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully"<<endl;
    //cerr<<"with fh "<<destFH<<endl;

    //now we can put
    int result;
    // Loop until we got 0 (end of data)
    writeargs writeArgs;
    memcpy(writeArgs.file.data,(const char*)destFH,Floppy_FHSIZE);
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
       //kdDebug(7101)<<"received "<<result<<" bytes for putting"<<endl;
       char * data=buffer.data();
       int bytesToWrite=buffer.size();
       int writeNow(0);
       if (result > 0)
       {
          do
          {
             if (bytesToWrite>Floppy_MAXDATA)
             {
                writeNow=Floppy_MAXDATA;
             }
             else
             {
                writeNow=bytesToWrite;
             };
             writeArgs.data.data_val=data;
             writeArgs.data.data_len=writeNow;

             int clnt_stat = clnt_call(m_client, FloppyPROC_WRITE,
                                       (xdrproc_t) xdr_writeargs, (char*)&writeArgs,
                                       (xdrproc_t) xdr_attrstat, (char*)&attrStat,total_timeout);
             //kdDebug(7101)<<"written"<<endl;
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
};


void FloppyProtocol::copy( const KURL &src, const KURL &dest, int _mode, bool _overwrite )
{
   //prepare the source
   QString thePath( QFile::encodeName(src.path()));
   stripTrailingSlash(thePath);
   kdDebug( 7101 ) << "Copy to -" << thePath <<"-"<<endl;
   FloppyFileHandle fh=getFileHandle(thePath);
   if (fh.isInvalid())
   {
      error(ERR_DOES_NOT_EXIST,thePath);
      return;
   };

   //create the destination
   QString destPath( QFile::encodeName(dest.path()));
   stripTrailingSlash(destPath);
   QString parentDir, fileName;
   getLastPart(destPath,fileName, parentDir);
   if (isRoot(parentDir))
   {
      error(ERR_ACCESS_DENIED,destPath);
      return;
   };
   FloppyFileHandle destFH;
   destFH=getFileHandle(destPath);
   kdDebug(7101)<<"file handle for -"<<destPath<<"- is "<<destFH<<endl;

   //the file exists and we don't want to overwrite
   if ((!_overwrite) && (!destFH.isInvalid()))
   {
      error(ERR_FILE_ALREADY_EXIST,destPath);
      return;
   };
   //TODO: is this correct ?
   //we have to "create" the file anyway, no matter if it already
   //exists or not
   //if we don't create it new, written text will be, hmm, "inserted"
   //in the existing file, i.e. a file could not become smaller, since
   //write only overwrites or extends, but doesn't remove stuff from a file

   kdDebug(7101)<<"creating the file -"<<fileName<<"-"<<endl;
   FloppyFileHandle parentFH;
   parentFH=getFileHandle(parentDir);
   //the directory doesn't exist
   if (parentFH.isInvalid())
   {
      kdDebug(7101)<<"parent directory -"<<parentDir<<"- does not exist"<<endl;
      error(ERR_DOES_NOT_EXIST,parentDir);
      return;
   };
   createargs createArgs;
   memcpy(createArgs.where.dir.data,(const char*)parentFH,Floppy_FHSIZE);
   QCString tmpName=QFile::encodeName(fileName);
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
   int clnt_stat = clnt_call(m_client, FloppyPROC_CREATE,
                             (xdrproc_t) xdr_createargs, (char*)&createArgs,
                             (xdrproc_t) xdr_diropres, (char*)&dirOpRes,total_timeout);
   if (!checkForError(clnt_stat,dirOpRes.status,destPath)) return;
   //we created the file successfully
   destFH=dirOpRes.diropres_u.diropres.file.data;
   kdDebug(7101)<<"file -"<<fileName<<"- in dir -"<<parentDir<<"- created successfully"<<endl;

   char buf[Floppy_MAXDATA];
   writeargs writeArgs;
   memcpy(writeArgs.file.data,(const char*)destFH,Floppy_FHSIZE);
   writeArgs.beginoffset=0;
   writeArgs.totalcount=0;
   writeArgs.offset=0;
   writeArgs.data.data_val=buf;
   attrstat attrStat;

   readargs readArgs;
   memcpy(readArgs.file.data,fh,Floppy_FHSIZE);
   readArgs.offset=0;
   readArgs.count=Floppy_MAXDATA;
   readArgs.totalcount=Floppy_MAXDATA;
   readres readRes;
   readRes.readres_u.reply.data.data_val=buf;

   int bytesRead(0);
   do
   {
      //first read
      int clnt_stat = clnt_call(m_client, FloppyPROC_READ,
                                (xdrproc_t) xdr_readargs, (char*)&readArgs,
                                (xdrproc_t) xdr_readres, (char*)&readRes,total_timeout);
      if (!checkForError(clnt_stat,readRes.status,thePath)) return;
      if (readArgs.offset==0)
         totalSize(readRes.readres_u.reply.attributes.size);

      bytesRead=readRes.readres_u.reply.data.data_len;
      //kdDebug(7101)<<"read "<<bytesRead<<" bytes"<<endl;
      //then write
      if (bytesRead>0)
      {
         readArgs.offset+=bytesRead;

         writeArgs.data.data_len=bytesRead;

         clnt_stat = clnt_call(m_client, FloppyPROC_WRITE,
                               (xdrproc_t) xdr_writeargs, (char*)&writeArgs,
                               (xdrproc_t) xdr_attrstat, (char*)&attrStat,total_timeout);
         //kdDebug(7101)<<"written"<<endl;
         if (!checkForError(clnt_stat,attrStat.status,destPath)) return;
         writeArgs.offset+=bytesRead;
      };
   } while (bytesRead>0);

   finished();
}*/

