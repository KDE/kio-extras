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
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>


#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <qtextstream.h>
#include <qcstring.h>
#include <qfile.h>
#include <qregexp.h>
#include <qdir.h>

#include "kio_smb.h"

#include <kinstance.h>
#include <kdebug.h>
#include <kio/global.h>
#include <klocale.h>
#include <kconfig.h>

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KLocale::setMainCatalogue("kio_smbro");
  KInstance instance( "kio_smb" );

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_smb protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }
  //kdDebug(KIO_SMB) << "Smb: kdemain: starting" << endl;

  SmbProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  kdDebug(KIO_SMB)<<"exiting normally"<<endl;
  return 0;
}

//bool wasKilled() { return false; }

int makeDirHier(const QString& path)
{
   QString s(path);
   QStringList sl=QStringList::split("/",s);
   s="";
   QDir d;
   for ( QStringList::Iterator it = sl.begin(); it != sl.end(); it++ )
   {
      s+="/"+(*it);
      if ((!d.exists(s)) && (!d.mkdir(s)))
            return -1;
   };
   return 0;
};

void SmbProtocol::getShareAndPath(const KURL& url, QString& share, QString& rest)
{
   QString path=url.path();
   share="";
   rest="";
   m_currentWorkgroup=m_defaultWorkgroup;
   int i=0;
   QString tmpHost;
   QStringList list=QStringList::split("/",path);
   for (QStringList::Iterator it=list.begin(); it!=list.end(); it++)
   {
      if (url.host().isEmpty())  //smb:/wg/host/ - type url
      {
         if (i==0)
            m_currentWorkgroup=(*it);
         else if (i==1)
         {
            tmpHost=(*it);
            setHost(tmpHost,42,"hallo","welt");
         }
         else if (i==2)
            share=(*it);
         else
            rest=rest+"\\"+(*it);
      }
      else
      {
         if (i==0)
            share=(*it);
         else
            rest=rest+"\\"+(*it);
      };
      i++;
   };
   if ((rest.isEmpty()) && (!share.isEmpty()) && (path[path.length()-1]=='/'))
      rest="\\";

   kdDebug(KIO_SMB)<<"getShareAndPath: path: -"<<path<<"-  share: -"<<share<<"-  rest: -"<<rest<<"-"<<endl;
};

QString my_unscramble(const QString& secret)
{
   QString plain="";
   for (uint i=0; i<secret.length()/3; i++)
   {
      QChar qc1 = secret[i*3];
      QChar qc2 = secret[i*3+1];
      QChar qc3 = secret[i*3+2];
      unsigned int a1 = qc1.latin1() - '0';
      unsigned int a2 = qc2.latin1() - 'A';
      unsigned int a3 = qc3.latin1() - '0';
      unsigned int num = ((a1 & 0x3F) << 10) | ((a2& 0x1F) << 5) | (a3 & 0x1F);
      plain[i] = QChar((uchar)((num - 17) ^ 173)); // restore
   }
   return plain;
};

QString my_scramble(const QString& plain)
{
   //taken from Nicola Brodu's smb ioslave
   //it's not really secure, but at
   //least better than storing the plain password
   QString scrambled;
   for (uint i=0; i<plain.length(); i++)
   {
      QChar c = plain[i];
      unsigned int num = (c.unicode() ^ 173) + 17;
      unsigned int a1 = (num & 0xFC00) >> 10;
      unsigned int a2 = (num & 0x3E0) >> 5;
      unsigned int a3 = (num & 0x1F);
      scrambled += (char)(a1+'0');
      scrambled += (char)(a2+'A');
      scrambled += (char)(a3+'0');
   }
   return scrambled;
};

SmbProtocol::SmbProtocol (const QCString &pool, const QCString &app )
:SlaveBase( "smb", pool, app )
,m_stdoutBuffer(0)
,m_stdoutSize(0)
,m_currentHost("")
,m_nmbName("")
,m_ip("")
,m_processes(17,false)
,m_showHiddenShares(true)
//,m_storePasswords(false)
,m_password("")
,m_user("")
,m_defaultWorkgroup("")
,m_currentWorkgroup("")
{
   kdDebug(KIO_SMB)<<"Smb::Smb: -"<<pool<<"-"<<endl;
   m_processes.setAutoDelete(true);
   m_months.insert("Jan",1);
   m_months.insert("Feb",2);
   m_months.insert("Mar",3);
   m_months.insert("Apr",4);
   m_months.insert("May",5);
   m_months.insert("Jun",6);
   m_months.insert("Jul",7);
   m_months.insert("Aug",8);
   m_months.insert("Sep",9);
   m_months.insert("Oct",10);
   m_months.insert("Nov",11);
   m_months.insert("Dec",12);

   KConfig *cfg = new KConfig("kioslaverc", true);
   cfg->setGroup("Browser Settings/SMBro");
   m_user=cfg->readEntry("User","");
   m_defaultWorkgroup=cfg->readEntry("Workgroup","");
   m_currentWorkgroup=m_defaultWorkgroup;
   m_showHiddenShares=cfg->readBoolEntry("ShowHiddenShares",false);
//   m_storePasswords=cfg->readBoolEntry("StorePasswords",false);  //default to false, it's dangerous !

   // unscramble, taken from Nicola Brodu's smb ioslave
   //not really secure, but better than storing the plain password
   QString scrambled = cfg->readEntry( "Password","" );
   m_password=my_unscramble(scrambled);
};

SmbProtocol::~SmbProtocol()
{
   kdDebug(KIO_SMB)<<"Smb::~Smb() xxx"<<endl;
   if (m_stdoutBuffer!=0)
      delete [] m_stdoutBuffer;
   m_processes.clear();
   m_stdoutBuffer=0;
};

int SmbProtocol::readOutput(int fd)
{
   //kdDebug(KIO_SMB)<<"Smb::readStdout"<<endl;
   //if (m_mtool==0) return 0;

   static char buffer[16*1024];
   int length=::read(fd,buffer,16*1024);
   if (length<=0) return length;

   //+1 gives us room for a terminating 0
   char *newBuffer=new char[length+m_stdoutSize+1];
   //kdDebug(KIO_SMB)<<"Smb::readStdout(): length: "<<length<<", m_stdoutSize: "<<m_stdoutSize<<" + 1 = "<<length+m_stdoutSize+1<<endl;
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
   return length;
};

void SmbProtocol::clearBuffer()
{
   m_stdoutSize=0;
   if (m_stdoutBuffer!=0)
      delete [] m_stdoutBuffer;
   m_stdoutBuffer=0;
};

bool SmbProtocol::stopAfterError(const KURL& url, bool notSureWhetherErrorOccured, bool onlyCheckForExistance)
{
   if (wasKilled())
   {
      finished();
      return true;
   };
   if (m_stdoutSize==0)
   {
      //error(KIO::ERR_UNKNOWN,"");
      //error( KIO::ERR_CONNECTION_BROKEN, m_currentHost);
      error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
      return true;
   };

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);

   //no samba service on this host
   if ((outputString.contains("Connection to")) && (outputString.contains("failed"))
       && (outputString.contains("error connecting")) && (outputString.contains("(Connection refused")))
   {
      error( KIO::ERR_COULD_NOT_CONNECT, m_currentHost+i18n("\nThere is probably no SMB service running on this host."));
   }
   else if (outputString.contains("smbclient not found"))
   {
      error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
   }
   //host not found
   else if ((outputString.contains("Connection to")) && (outputString.contains("failed")))
   {
      error( KIO::ERR_COULD_NOT_CONNECT, m_currentHost);
   }
   else if (outputString.contains("ERRDOS - ERRnomem"))
   {
      error( KIO::ERR_INTERNAL_SERVER, m_currentHost);
   }
   //wrong password
   else if (outputString.contains("ERRSRV - ERRbadpw"))
   {
      //we should never get here
      error( KIO::ERR_COULD_NOT_STAT, m_currentHost+i18n("\nInvalid user/password combination."));
   }
   else if ((outputString.contains("ERRDOS")) && (outputString.contains("ERRnoaccess")))
   {
      //we should never get here
      error( KIO::ERR_COULD_NOT_STAT, m_currentHost+i18n("\nInvalid user/password combination."));
   }
   //file not found
   else if ((outputString.contains("ERRDOS")) && (outputString.contains("ERRbadfile")) && (onlyCheckForExistance==false))
   {
      //kdDebug(KIO_SMB)<<"Smb::stopAfterError() contains both, reporting error"<<endl;
      error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL());
   }
   else if (outputString.contains("Broken pipe"))
   {
      error( KIO::ERR_CONNECTION_BROKEN, m_currentHost);
   }
   else if (notSureWhetherErrorOccured)
   {
      return false;
   }
   else
   {
      kdDebug(KIO_SMB)<<"Smb::stopAfterError() -"<<m_stdoutBuffer<<"-"<<endl;
      error( KIO::ERR_UNKNOWN, i18n("Hmm..."));
   };
   return true;
};

SmbProtocol::SmbReturnCode SmbProtocol::waitUntilStarted(ClientProcess *proc, const QString& password, const char* prompt)
{
   //not ok
   if (proc==0) return SMB_ERROR;
   //ok if already running (shouldn't happen, actually)
   if (proc->startingFinished) return SMB_OK;

   //wait until we get a "smb: \>" and password and stuff
   clearBuffer();
   bool alreadyEnteredPassword(false);

   //we leave this loop if smbclient exits or if smbclient prints the prompt
   while(1)
   {
      bool stdoutEvent;
      proc->select(1,0,&stdoutEvent);
      //if smbclient exits, something went wrong
      int exitStatus=proc->exited();
      if (exitStatus!=-1)
      {
         kdDebug(KIO_SMB)<<"Smb::waitUntilStarted() smbclient exited with exitcode "<<exitStatus<<endl;
         if ((exitStatus==0) && (prompt==0)) //smbmount
            return SMB_OK;
         if (alreadyEnteredPassword)
            return SMB_WRONGPASSWORD;
         return SMB_ERROR;
      };

      if (stdoutEvent)
      {
         readOutput(proc->fd());
         //don't search the whole buffer, only the last 12 bytes
         if (m_stdoutSize>12)
         {
            //check whether it asks for the password
            if (strstr(m_stdoutBuffer+m_stdoutSize-12,"\nPassword:")!=0)
            {
               kdDebug(KIO_SMB)<<"Smb::waitUntilStarted(): received: -"<<m_stdoutBuffer<<"-"<<endl;
               if (password.isEmpty())
               {
                  kdDebug(KIO_SMB)<<"Smb::waitUntilStarted(): feeding empty password"<<endl;
                  ::write(proc->fd(),"\n",1);
               }
               else
                  ::write(proc->fd(),(password+"\n").local8Bit(),password.length()+1);
               //read the echoed \n
               char c;
               ::read(proc->fd(),&c,1);
               alreadyEnteredPassword=true;
            }
            //or whether it prints the prompt :-)
            else if ((prompt!=0) && (strstr(m_stdoutBuffer+m_stdoutSize-12,"smb: \\>")!=0))
            {
               proc->startingFinished=true;
               return SMB_OK;
            };
         };
      };
   };
};

SmbProtocol::SmbReturnCode SmbProtocol::getShareInfo(ClientProcess* shareLister,const QString& password, bool listWgs)
{
   if (shareLister==0) return SMB_ERROR;

   clearBuffer();
   bool alreadyEnteredPassword(false);
   //we leave the loop when smbclient exits
   while (1)
   {
      bool stdoutEvent;
      shareLister->select(1,0,&stdoutEvent);
      if (wasKilled())
         return SMB_OK;
      int exitStatus=shareLister->exited();
      if (exitStatus!=-1)
      {
         if (stdoutEvent) readOutput(shareLister->fd());

         kdDebug(KIO_SMB)<<"Smb::getShareInfo(): smbclient exited with status "<<exitStatus<<endl;
         if (exitStatus!=0)
            kdDebug(KIO_SMB)<<"Smb::getShareInfo(): received: -"<<m_stdoutBuffer<<"-"<<endl;

         if (exitStatus==0)
         {
            kdDebug(KIO_SMB)<<"::getShareInfo() exitStaus==0"<<endl;
            if (m_stdoutBuffer==0) return SMB_OK;
            if ((strstr(m_stdoutBuffer,"ERRDOS - ERRnoaccess")!=0) ||
                ((strstr(m_stdoutBuffer,"NT_STATUS_ACCESS_DENIED")!=0) && (listWgs==false)))
               return SMB_WRONGPASSWORD;
            else
               return SMB_OK; //probably
         }
         else if (alreadyEnteredPassword)
         {
            if (m_stdoutBuffer==0) return SMB_ERROR;
            kdDebug(KIO_SMB)<<"::getShareInfo() in alreadyEnteredPassword"<<endl;
            if (strstr(m_stdoutBuffer,"ERRDOS - ERRnomem")!=0)
            {
               kdDebug(KIO_SMB)<<"::getShareInfo() after strstr(ERRnomem)"<<endl;
               return SMB_ERROR;  //:-(
            }
            else
               return SMB_WRONGPASSWORD;  //:-(
         }
         else
            return SMB_ERROR;  // :-(
      };
      if (stdoutEvent)
      {
         int result=readOutput(shareLister->fd());
         //don't search the whole buffer, only the last 12 bytes
         if ((result>0) && (m_stdoutSize>12))
         {
            if (strstr(m_stdoutBuffer+m_stdoutSize-12,"\nPassword:")!=0)
            {
               kdDebug(KIO_SMB)<<"Smb::getShareInfo() received: -"<<m_stdoutBuffer<<"-"<<endl;
               //everything went fine until now, so we can safely delete
               //what we have so far
               clearBuffer();
               if (password.isEmpty())
               {
                  kdDebug(KIO_SMB)<<"Smb::getShareInfo() feeding empty password"<<endl;
                  ::write(shareLister->fd(),"\n",1);
               }
               else
                  ::write(shareLister->fd(),(password+"\n").local8Bit(),password.length()+1);
               //read the echoed \n
               char c;
               ::read(shareLister->fd(),&c,1);
               alreadyEnteredPassword=true;
            };
         };
      };
   };
};

bool SmbProtocol::getAuth(AuthInfo& auth, const QString& server, const QString& wg, const QString& share, const QString& realm, const QString& user, bool& firstLoop)
{
   auth.url=KURL("smb://"+server.lower());
   auth.username = user;
   auth.keepPassword=true;
   auth.realmValue=realm.lower();

   QString c, cl;
   cl=i18n("Server");
   c=server;
   if (!wg.isEmpty())
   {
      cl+="."+i18n("Workgroup");
      c+="."+wg;
   };
   if (!share.isEmpty())
   {
      cl+="/"+i18n("Share");
      c+="/"+share;
   };
   auth.comment=c;
   auth.commentLabel=cl;
   if (firstLoop)
   {
      firstLoop=false;
      if (checkCachedAuthentication(auth))
         return true;
   };
   if (openPassDlg(auth))
      return true;
   return false;
};

void SmbProtocol::listShares()
{
   kdDebug(KIO_SMB)<<"Smb::listShares() "<<endl;
   ClientProcess *proc=new ClientProcess();
   QCStringList args;
   args<<QCString("-L")+m_nmbName;
   if (!m_user.isEmpty())
      args<<QCString("-U")+m_user.local8Bit();
   if (!m_ip.isEmpty())
      args<<QCString("-I")+m_ip;
   if (!m_currentWorkgroup.isEmpty())
      args<<QCString("-W")+m_currentWorkgroup.local8Bit();
   if (!proc->start("smbclient",args))
   {
      error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
      delete proc;
      return;
   };
   QString password(m_password);
   QString user(m_user);

   SmbReturnCode result(SMB_NOTHING);
   bool firstLoop=true;
   AuthInfo ai;
   //repeat until user/password is ok or the user cancels
   //while (!getShareInfo(proc,password))
   while (result=getShareInfo(proc,password), result==SMB_WRONGPASSWORD)
   {
      kdDebug(KIO_SMB)<<"Smb::listShares() failed with password"<<endl;
      //it failed with the default password
      delete proc;
      proc=0;
      KIO::AuthInfo authInfo;
      if (getAuth(authInfo,QString(m_nmbName),m_currentWorkgroup,"",user+"_"+QString(m_nmbName),user,firstLoop))
      {
         ai=authInfo;
         user = authInfo.username;
         password = authInfo.password;
         proc=new ClientProcess();
         QCStringList tmpArgs;
         tmpArgs<<QCString("-L")+m_nmbName;
         if (!user.isEmpty())
            tmpArgs<<QCString("-U")+user.local8Bit();
         if (!m_ip.isEmpty())
            args<<QCString("-I")+m_ip;
         if (!m_currentWorkgroup.isEmpty())
            tmpArgs<<QCString("-W")+m_currentWorkgroup.local8Bit();
         if (!proc->start("smbclient",tmpArgs))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return;
         };
      }
      else break;
/*    authInfo.url=KURL("smb://"+m_nmbName);
      authInfo.username = user;
      authInfo.keepPassword=true;
      authInfo.realmValue=user+"_"+QString(m_nmbName);
      if (m_currentWorkgroup.isEmpty())
      {
         authInfo.commentLabel=i18n("Server:");
         authInfo.comment=QString(m_nmbName);
      }
      else
      {
         authInfo.commentLabel=i18n("Server, Workgroup:");
         authInfo.comment=QString(m_nmbName)+", "+m_currentWorkgroup;
      };
      bool hasAuth=false;
      if (firstLoop)
      {
         hasAuth=checkCachedAuthentication(authInfo);
         firstLoop=false;
      };
      if ((hasAuth) || (openPassDlg(authInfo)))
      {
         ai=authInfo;
         user = authInfo.username;
         password = authInfo.password;
         proc=new ClientProcess();
         QCStringList tmpArgs;
         tmpArgs<<QCString("-L")+m_nmbName;
         if (!user.isEmpty())
            tmpArgs<<QCString("-U")+user.local8Bit();
         if (!m_ip.isEmpty())
            args<<QCString("-I")+m_ip;
         if (!m_currentWorkgroup.isEmpty())
            tmpArgs<<QCString("-W")+m_currentWorkgroup.local8Bit();
         if (!proc->start("smbclient",tmpArgs))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return;
         };
      }
      else break;*/
   };
   //here smbclient has already exited
   if (proc!=0)
   {
      delete proc;
      proc=0;
   };

   KURL url("smb://"+m_currentHost);
   //no error handling has happened up to now
   if (result==SMB_ERROR)
   {
      stopAfterError(url,false);
      return;
   }
   //this happens only if the user pressed cancel
   else if (result==SMB_WRONGPASSWORD)
   {
      error(ERR_USER_CANCELED,"");
      return;
   };

   if (stopAfterError(url,true))
      return;

   //if we get here, we had success
   if (!ai.username.isEmpty()) //we used the AuthInfo
      cacheAuthentication(ai);

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;

   int totalNumber(0);
   int mode(0);
   UDSEntry entry;

   int shareNamePos(0);
   int typePos(0);
   bool ipcsFound(false);

   while (!output.atEnd())
   {
      line=output.readLine();
      if (mode==0)
      {
         if (line.contains("Sharename"))
         {
            mode=1;
            shareNamePos=line.find("Sharename");
            typePos=line.find("Type");
         };
      }
      else if (mode==1)
      {
         if (line.contains("-----"))
            mode=2;
         else
            return;
      }
      else if (mode==2)
      {
         kdDebug(KIO_SMB)<<"Smb::listShares(): line: -"<<line.local8Bit()<<"-"<<endl;
         if (line.isEmpty())
            break;
         else if (line.mid(typePos,3)=="IPC")
            ipcsFound=true;
         else if (line.mid(typePos,4)=="Disk")
         {
            QString name=line.mid(shareNamePos,typePos-shareNamePos);
            int end(name.length()-1);
            while (name[end]==' ')
               end--;
            name=name.left(end+1);
            if ((m_showHiddenShares) || (name[name.length()-1]!='$'))
            {
               entry.clear();
               UDSAtom atom;

               atom.m_uds = KIO::UDS_NAME;
               atom.m_str =name;
               entry.append( atom );

               atom.m_uds = KIO::UDS_SIZE;
               atom.m_long = 1024;
               entry.append(atom);

               atom.m_uds = KIO::UDS_MODIFICATION_TIME;
               atom.m_long = time(0);
               entry.append( atom );

               atom.m_uds = KIO::UDS_ACCESS;
               atom.m_long=S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
               entry.append( atom );

               atom.m_uds = KIO::UDS_FILE_TYPE;
               atom.m_long =S_IFDIR;
               entry.append( atom );

               listEntry( entry, false);
               totalNumber++;
            };
         };
      };
   };
   totalSize( totalNumber);
   listEntry( entry, true ); // ready

   finished();
};

void SmbProtocol::listDir( const KURL& _url)
{
   kdDebug(KIO_SMB)<<"Smb::listDir() "<<_url.path()<<endl;
//   QString path( _url.path());

   if (_url.url()=="smb://")
   {
      error(ERR_UNKNOWN_HOST,i18n("To access the shares of a host, use smb://hostname\n\
To get a list of all hosts use lan:/ or rlan:/ .\n\
See the KDE Control Center under Network, LANBrowsing for more information."));
      return;
   };

   if (_url.url()=="smb:/")
   {
      listWorkgroups();
      return;
   };

   if (_url.path()[_url.path().length()-1]!='/')
   {
      KURL url(_url);
      url.setPath(_url.path()+"/");
      redirection(url);
      finished();
      return;
   };

   QString share;
   QString smbPath;
   QString wg;
   getShareAndPath(_url,share,smbPath);

   if (m_currentHost.isEmpty())
   {
      kdDebug(KIO_SMB)<<"Smb::listDir() listHosts()"<<endl;
      listHosts();
      return;
   };

   if (share.isEmpty())
   {
      listShares();
      return;
   };

   ClientProcess *proc=getProcess(m_currentHost, share);
   if (proc==0)
   {
      kdDebug(KIO_SMB)<<"Smb::listDir() proc==0"<<endl;
      return;
   };

   QCString command=QCString("dir \"")+smbPath.local8Bit()+QCString("\\*\"\n");
   kdDebug(KIO_SMB)<<"Smb::listDir(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      kdDebug(KIO_SMB)<<"Smb::listDir() could not ::write()"<<endl;
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      return;
   };

   clearBuffer();

   int result;
   bool loopFinished(false);
   do
   {
      int exitStatus=proc->exited();
      if (exitStatus!=-1)
      {
         //this should not happen !
         kdDebug(KIO_SMB)<<"Smb::listDir(): smbclient exited "<<exitStatus<<endl;
         stopAfterError(_url,false);
         return;
      };
      bool stdoutEvent;
      result=proc->select(1,0,&stdoutEvent);
      if (stdoutEvent)
      {
         readOutput(proc->fd());
         //kdDebug(KIO_SMB)<<"Smb::listDir(): read: -"<<m_stdoutBuffer<<"-"<<endl;
         //don't search the whole buffer, only the last 12 bytes
         if (m_stdoutSize>12)
         {
            if (strstr(m_stdoutBuffer+m_stdoutSize-12,"\nsmb: \\>")!=0)
               loopFinished=true;
         };
      };
   } while (!loopFinished);
//   kdDebug(KIO_SMB)<<"Smb::listDir(): read: -"<<m_stdoutBuffer<<"-"<<endl;

   //check the output from smbclient whether an error occured
   if (stopAfterError(_url,true))
      return;

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;

   int totalNumber(0);
   UDSEntry entry;

   while (!output.atEnd())
   {
      line=output.readLine();
//      kdDebug(KIO_SMB)<<"Smb::listDir(): line: -"<<line<<"-"<<endl;
      if (line.isEmpty())
         break;
      StatInfo info=createStatInfo(line);
      if (info.isValid)
      {
         entry.clear();
         createUDSEntry(info,entry);
         //kdDebug(KIO_SMB)<<"Smb::listDir(): creating UDSEntry"<<endl;
         listEntry( entry, false);
         totalNumber++;
      };
   };
   totalSize( totalNumber);
   listEntry( entry, true ); // ready
   finished();
   //kdDebug(KIO_SMB)<<"Smb::listDir() ends"<<endl;
};

void SmbProtocol::mkdir( const KURL& url, int)
{
   kdDebug(KIO_SMB)<<"Smb::mkdir() "<<url.path().local8Bit()<<endl;
   QString path( url.path());

   QString share;
   QString smbPath;
   getShareAndPath(url,share,smbPath);

   //the error was already reported in _stat()
   if (smbPath.isEmpty())
   {
      kdDebug(KIO_SMB)<<"Smb::mkdir() file not found"<<endl;
      return;
   };

   ClientProcess *proc=getProcess(m_currentHost, share);

   QCString command=QCString("mkdir \"")+smbPath.local8Bit()+QCString("\" \n");
   kdDebug(KIO_SMB)<<"Smb::mkdir(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      return;
   };

   clearBuffer();
   bool loopFinished(false);
   //read the terminal echo
   do
   {
      readOutput(proc->fd());
      kdDebug(KIO_SMB)<<"Smb::mkdir() read: -"<<m_stdoutBuffer<<"-"<<endl;
      if (m_stdoutSize>0)
         if (memchr(m_stdoutBuffer,'\n',m_stdoutSize)!=0)
            loopFinished=true;
   } while (!loopFinished);
   clearBuffer();
   finished();

};

void SmbProtocol::del( const KURL& url, bool isfile)
{
   kdDebug(KIO_SMB)<<"Smb::del() "<<url.path().local8Bit()<<endl;
   QString path( url.path());

   QString share;
   QString smbPath;
   getShareAndPath(url,share,smbPath);

   StatInfo info=this->_stat(url);
   //the error was already reported in _stat()
   if ((info.isValid==false) || (smbPath.isEmpty()))
   {
      kdDebug(KIO_SMB)<<"Smb::del() file not found"<<endl;
      return;
   };
   ClientProcess *proc=getProcess(m_currentHost, share);

   QCString command;
   if (isfile)
      command="del \"";
   else
      command="rmdir \"";

   command=command+smbPath.local8Bit()+QCString("\" \n");
   kdDebug(KIO_SMB)<<"Smb::del(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      return;
   };

   clearBuffer();
   bool loopFinished(false);
   //read the terminal echo
   do
   {
      readOutput(proc->fd());
      kdDebug(KIO_SMB)<<"Smb::del() read: -"<<m_stdoutBuffer<<"-"<<endl;
      if (m_stdoutSize>0)
         if (memchr(m_stdoutBuffer,'\n',m_stdoutSize)!=0)
            loopFinished=true;
   } while (!loopFinished);
   clearBuffer();
   finished();
};


void SmbProtocol::createUDSEntry(const StatInfo& info, UDSEntry& entry)
{
   UDSAtom atom;
   atom.m_uds = KIO::UDS_NAME;
   atom.m_str = info.name;
   entry.append( atom );

   atom.m_uds = KIO::UDS_SIZE;
   atom.m_long = info.size;
   entry.append(atom);

   atom.m_uds = KIO::UDS_MODIFICATION_TIME;
   atom.m_long = info.time;
   entry.append( atom );

   atom.m_uds = KIO::UDS_ACCESS;
   atom.m_long=info.mode;
   entry.append( atom );

   atom.m_uds = KIO::UDS_FILE_TYPE;
   atom.m_long =(info.isDir?S_IFDIR:S_IFREG);
   entry.append( atom );
};

StatInfo SmbProtocol::createStatInfo(const QString line)
{
   QString name;
   QString size;

   StatInfo info;

   static QDateTime beginningOfTimes(QDate(1970,1,1),QTime(1,0));

//"      A   213123  Mon Mar 12"
   //the \\d+ is required for files bigger than 100.000.000 bytes
   //smbclient 1.9.18p10 has at least 9 for the filesize
   //version 2.0.5 to at least 2.2.0 has at least 8 characters
   //version 2.2.2 has at least 7 characters
   int startOfData=line.find(QRegExp("    [SADR ][SADR ][SADR ] [ \\d][ \\d][ \\d][ \\d][ \\d][ \\d]\\d+  [A-Z][a-z][a-z] [A-Z][a-z][a-z] [ \\d]\\d"));
   //kdDebug(KIO_SMB)<<"createStatInfo: regexp at: "<<startOfData<<endl;
   if (startOfData==-1)
   {
      info.isValid=false;
      return info;
   };

   info.isValid=true;
   name=line.mid(2,startOfData-2);
   //if the file name ends with spaces, we have a problem...
   int end(name.length()-1);
   while (name[end]==' ')
      end--;
   name=name.left(end+1);

   if ((name==".") || (name==".."))
   {
      info.isValid=false;
      return info;
   };

   //kdDebug(KIO_SMB)<<"createStatInfo: name: -"<<name<<"-"<<endl;

   //kdDebug(KIO_SMB)<<"createStatInfo: line(start+5..7): -"<<line.mid(startOfData+5,3)<<"-"<<endl;
   //this offset is required for files bigger than 100.000.000 bytes
   int sizeOffset(0);
   while (line[startOfData+16+sizeOffset]!=' ')
      sizeOffset++;
   if ((line[startOfData+6]=='D') || (line[startOfData+5]=='D') || (line[startOfData+4]=='D'))
   {
      info.isDir=true;
      info.size=1024;
   }
   else
   {
      //kdDebug(KIO_SMB)<<"createStatInfo: line(start+6,11): -"<<line.mid(startOfData+6,11)<<"-"<<endl;
      info.isDir=false;
      size=line.mid(startOfData+7,9+sizeOffset);
      info.size=size.toInt();
      //kdDebug(KIO_SMB)<<"createStatInfo: size: -"<<size<<"-"<<endl;
   };

   info.name=name;

   startOfData=line.find(QRegExp("  [A-Z][a-z][a-z] [A-Z][a-z][a-z] [ \\d]\\d \\d\\d:\\d\\d:\\d\\d \\d\\d\\d\\d"));
   QString tmp;
   //month
   tmp=line.mid(startOfData+ 6,3);
   int month=m_months[tmp];
//   kdDebug(KIO_SMB)<<"month: -"<<tmp<<"-"<<endl;
   tmp=line.mid(startOfData+10,2);
   int day=tmp.toInt();
   tmp=line.mid(startOfData+13,2);
   int hour=tmp.toInt();
   tmp=line.mid(startOfData+16,2);
   int minute=tmp.toInt();
   tmp=line.mid(startOfData+19,2);
   int secs=tmp.toInt();
   tmp=line.mid(startOfData+22,4);
//   kdDebug(KIO_SMB)<<"year: -"<<tmp<<"-"<<endl;
   int year=tmp.toInt();
//   kdDebug(KIO_SMB)<<"sizeOff: "<<sizeOffset<<" mon: "<<month<<" day: "<<day<<" hour: "<<hour<<" minute: "<<minute<<" sec: "<<secs<<" year: "<<year<<endl;

/*
  blah_fasel_long__Long_big_very_big_file__.avi____RDA 644007936  Tue Jul  4 22:49:00 2000
  siehe auch MPG4-Codec.lnk     ____ADA     1452  Tue Dec 19 23:22:17 2000
*/
   QDateTime date(QDate(year,month,day),QTime(hour,minute,secs));
   info.time=beginningOfTimes.secsTo(date);
   //info.time=time(0);

   if (info.isDir)
      info.mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
   else
      info.mode = S_IRUSR | S_IRGRP | S_IROTH;

   //kdDebug(KIO_SMB)<<"Smb::createUDSEntry() ends"<<endl;
   return info;
};

StatInfo SmbProtocol::_stat(const KURL& url, bool onlyCheckForExistance)
{
   //kdDebug(KIO_SMB)<<"Smb::_stat() prettyURL(): -"<<url.prettyURL()<<"-"<<endl;
   kdDebug(KIO_SMB)<<"Smb::_stat() local8() : -"<<url.path().local8Bit()<<"-"<<endl;
   StatInfo info;

   QString path( url.path());
   QString share;
   QString smbPath;
   getShareAndPath(url,share,smbPath);

   //if share is empty, then smbPath is also empty
   if ((smbPath.isEmpty()) || (smbPath=="\\"))
   {
      kdDebug(KIO_SMB)<<"Smb::_stat(): smbPath.isEmpty()"<<endl;
      info.name=path;
      info.size=1024;
      info.time=time(0);
      info.mode=S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH| S_IWOTH|S_IWGRP|S_IWUSR;
      info.isDir=true;
      info.isValid=true;
      return info;
   };

   ClientProcess *proc=getProcess(m_currentHost, share);
   if (proc==0)
   {
      info.isValid=false;
      return info;
   };

   QCString command=QCString("dir \"")+smbPath.local8Bit()+QCString("\"\n");
   kdDebug(KIO_SMB)<<"Smb::_stat(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      info.isValid=false;
      return info;
   };


   clearBuffer();

   int result;
   bool loopFinished(false);
   do
   {
      int exitStatus=proc->exited();
      if (exitStatus!=-1)
      {
         //this should not happen
         stopAfterError(url,false);
         info.isValid=false;
         return info;
      };
      bool stdoutEvent;
      result=proc->select(1,0,&stdoutEvent);
      if (stdoutEvent)
      {
         readOutput(proc->fd());
         //don't search the whole buffer, only the last 12 bytes
         if (m_stdoutSize>12)
         {
            if (strstr(m_stdoutBuffer+m_stdoutSize-12,"\nsmb: \\>")!=0)
            loopFinished=true;
         };
      };
   } while (!loopFinished);
   kdDebug(KIO_SMB)<<"Smb::_stat(): read: -"<<m_stdoutBuffer<<"-"<<endl;

   if (stopAfterError(url,true,onlyCheckForExistance))
   {
      kdDebug(KIO_SMB)<<"stopAfterError() returned true"<<endl;
      info.isValid=false;
      return info;
   };

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;
   int lineNumber(0);
   while (!output.atEnd())
   {
      line=output.readLine();
      if (lineNumber==1)
      {
         if (line.contains("ERRbadfile") || (line.contains("File not found")))
         {
            kdDebug(KIO_SMB)<<"Smb::_stat() file not found"<<endl;
            info.isValid=false;
            break;
         }
         else
         {
            return createStatInfo(line);
         };
      };
      lineNumber++;
   };
   return info;
};

void SmbProtocol::stat( const KURL & url)
{
   kdDebug(KIO_SMB)<<"Smb::stat(): path: -"<<url.path().local8Bit()<<"- url: "<<url.url()<<"-"<<endl;

   if (url.url()=="smb://")
   {
      //kdDebug(KIO_SMB)<<"Smb::stat(): host.isEmpty()"<<endl;
      error(ERR_UNKNOWN_HOST,i18n("\nTo access the shares of a host, use smb://hostname\n\
To get a list of all hosts use lan:/ or rlan:/ .\n\
See the KDE Control Center under Network, LANBrowsing for more information."));
      return;
   };
   StatInfo info=this->_stat(url);
   if (!info.isValid)
      return;

   UDSEntry entry;
   createUDSEntry(info,entry);
   statEntry( entry );
   finished();
}

void SmbProtocol::put( const KURL& url, int, bool _overwrite, bool)
{
   StatInfo info=this->_stat(url,true);
   if (_overwrite==false && (info.isValid))
   {
      error(ERR_CANNOT_OPEN_FOR_WRITING,url.url());
      return;
   };
   QString share;
   QString smbPath;
   getShareAndPath(url,share,smbPath);

   ClientProcess *proc=getProcess(m_currentHost, share);
   if (proc==0)
      return;

   QCString fifoName;
   fifoName.sprintf("/tmp/kio_smb_%d_%d_%ld",getpid(),getuid(),time(0));
   kdDebug(KIO_SMB)<<"Smb::put() fifoname: -"<<fifoName<<"-"<<endl;
   int result=mkfifo(fifoName,0600);
   if ((result!=0) && (errno!=EEXIST))
   {
      //perror("creating fifo failed: ");
      error(ERR_CANNOT_OPEN_FOR_WRITING,url.path()+i18n("\nCould not create required pipe %1.").arg(fifoName));
      return;
   };
   QCString command=QCString("put ")+fifoName+QCString(" \"")+smbPath.local8Bit()+QCString("\"\n");
   kdDebug(KIO_SMB)<<"Smb::put(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      remove(fifoName);
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      return;
   };
   clearBuffer();
   bool loopFinished(false);
   //read the terminal echo
   do
   {
      readOutput(proc->fd());
      kdDebug(KIO_SMB)<<"Smb::put() read: -"<<m_stdoutBuffer<<"-"<<endl;
      if (m_stdoutSize>0)
      {
         if (memchr(m_stdoutBuffer,'\n',m_stdoutSize)!=0)
            loopFinished=true;
      };
   } while (!loopFinished);

   clearBuffer();
   bool stdoutEvent;
   result=proc->select(0,10*1000,&stdoutEvent);
   if (stdoutEvent)
   {
      readOutput(proc->fd());
      if (strstr(m_stdoutBuffer,fifoName.data())!=0)
      {
         remove(fifoName);
         error(ERR_SLAVE_DEFINED,i18n(
                "To be able to write to a remote Samba share you must patch "
                "and recompile smbclient.\n"
                "Visit http://lisa-home.sourceforge.net/smbclientpatch.html "
                "and follow the instructions there."));
         return;
      };
   };

   int fifoFD=open(fifoName,O_RDWR|O_NONBLOCK);
   if (fifoFD==-1)
   {
      //we failed (we might have no read access to the remote file)
      perror("SmbProtocol::put() open() failed");
      error(ERR_CANNOT_OPEN_FOR_WRITING,url.path()+i18n("\nCould not create required pipe %1.").arg(fifoName));
      remove(fifoName);
      return;
   };

   //now we have it, now we can make it blocking again
   int flags=fcntl(fifoFD,F_GETFL,0);
   if (flags<0)
   {
      //hmm, shouldn't happen
      error(ERR_CANNOT_OPEN_FOR_WRITING,url.path()+i18n("\nCould not fcntl() pipe %1.").arg(fifoName));
      remove(fifoName);
      return;
   };
   flags&=~O_NONBLOCK;
   if (fcntl(fifoFD,F_SETFL,flags)<0)
   {
      //hmm, shouldn't happen
      error(ERR_CANNOT_OPEN_FOR_WRITING,url.path()+i18n("\nCould not fcntl() pipe %1.").arg(fifoName));
      remove(fifoName);
      return;
   };

   kdDebug(KIO_SMB)<<"Smb::put() opened fifo: -"<<command<<"-"<<endl;

   result=0;
   loopFinished=false;
   do
   {
      QByteArray array;
      int exitStatus=proc->exited();
      if (exitStatus!=-1)
      {
         kdDebug(KIO_SMB)<<"Smb::put() proc->exited: "<<exitStatus<<endl;
         //this should not happen !
         stopAfterError(url,false);
         close(fifoFD);
         remove(fifoName);
         return;
         /*loopFinished=true;
         kdDebug(KIO_SMB)<<"Smb::get(): smbclient exited with status "<<exitStatus<<endl;
         break;*/
      };

      dataReq();
      result=readData(array);
      kdDebug(KIO_SMB)<<"Smb::put() readData() result="<<result<<endl;
      if (wasKilled())
         loopFinished=true;
      else if (result>0)
      {
         int bytesLeft=array.size();
         char* buf=array.data();
         while (bytesLeft>0)
         {
            kdDebug(KIO_SMB)<<"Smb::put() bytesLeft="<<bytesLeft<<endl;
            int bytesWritten=::write(fifoFD,buf,bytesLeft);
            kdDebug(KIO_SMB)<<"Smb::put() wrote="<<bytesWritten<<endl;
            if (bytesWritten>0)
            {
               bytesLeft-=bytesWritten;
               buf+=bytesWritten;
            }
            else if (bytesWritten<=0)
            {
               perror("write");
               kdDebug(KIO_SMB)<<"Smb::put() errno="<<errno<<endl;
               loopFinished=true;
               bytesLeft=0;
            };
         };
      }
      else loopFinished=true;
   } while(!loopFinished);

   close(fifoFD);

   clearBuffer();
   result=proc->select(1,0,&stdoutEvent);
   if (stdoutEvent)
      readOutput(proc->fd());

   remove(fifoName);

   kdDebug(KIO_SMB)<<"Smb::put(): received -"<<m_stdoutBuffer<<"-"<<endl;
   if (stopAfterError(url,true))
      return;
   finished();
};


void SmbProtocol::get( const KURL& url )
{
   kdDebug(KIO_SMB)<<"Smb::get() "<<url.path().local8Bit()<<endl;
   QString path( url.path());

   QString share;
   QString smbPath;
   getShareAndPath(url,share,smbPath);

   StatInfo info=this->_stat(url);
   //the error was already reported in _stat()
   if ((info.isValid==false) || (info.isDir))
   {
      kdDebug(KIO_SMB)<<"Smb::get() file not found"<<endl;
      error(ERR_CANNOT_OPEN_FOR_READING,url.url());
      return;
   };

   totalSize( info.size);

   ClientProcess *proc=getProcess(m_currentHost, share);
   if (proc==0)
   {
      return;
   };

   QCString fifoName;
   fifoName.sprintf("/tmp/kio_smb_%d_%d_%ld",getpid(),getuid(),time(0));
   kdDebug(KIO_SMB)<<"Smb::get() fifoname: -"<<fifoName<<"-"<<endl;
   int result=mkfifo(fifoName,0600);
   if ((result!=0) && (errno!=EEXIST))
   {
      //perror("creating fifo failed: ");
      error(ERR_CANNOT_OPEN_FOR_READING,url.path()+i18n("\nCould not create required pipe %1.").arg(fifoName));
      return;
   };

   QCString command=QCString("get \"")+smbPath.local8Bit()+QCString("\" ")+fifoName+"\n";
   kdDebug(KIO_SMB)<<"Smb::get(): executing command: -"<<command<<"-"<<endl;

   if (::write(proc->fd(),command.data(),command.length())<0)
   {
      error(ERR_CONNECTION_BROKEN,m_currentHost);
      return;
   };

   clearBuffer();
   bool loopFinished(false);
   //read the terminal echo
   do
   {
      readOutput(proc->fd());
      kdDebug(KIO_SMB)<<"Smb::get() read: -"<<m_stdoutBuffer<<"-"<<endl;
      if (m_stdoutSize>0)
         if (memchr(m_stdoutBuffer,'\n',m_stdoutSize)!=0)
            loopFinished=true;
   } while (!loopFinished);
   clearBuffer();

   int fifoFD=open(fifoName,O_RDONLY|O_NONBLOCK);
   if (fifoFD==-1)
   {
      //we failed (we might have no read access to the remote file)
      perror("SmbProtocol::get() open() failed");
      error(ERR_CANNOT_OPEN_FOR_READING,url.path()+i18n("\nCould not create required pipe %1.").arg(fifoName));
      remove(fifoName);
      return;

   };

   //now we have it, now we can make it blocking again
   int flags=fcntl(fifoFD,F_GETFL,0);
   if (flags<0)
   {
      //hmm, shouldn't happen
      error(ERR_CANNOT_OPEN_FOR_READING,url.path()+i18n("\nCould not fcntl() pipe %1.").arg(fifoName));
      remove(fifoName);
      return;
   };
   flags&=~O_NONBLOCK;
   if (fcntl(fifoFD,F_SETFL,flags)<0)
   {
      //hmm, shouldn't happen
      error(ERR_CANNOT_OPEN_FOR_READING,url.path()+i18n("\nCould not fcntl() pipe %1.").arg(fifoName));
      remove(fifoName);
      return;
   };

   //now read from the fifo
   //this might evetually block :-(
   //but actually I don't know why this should happen
   //we entered "get some_file /tmp/the_fifo" into smbclient
   //the fifo /tmp/the_fifo exists and the remote some_file exists too,
   //we checked this with the _stat() call, so why should it not work ?
/*   FILE * fifo=fopen(fifoName,"r");
   if (fifo==0)
   {
      //hmm, how should we get here ?
      error(ERR_CANNOT_OPEN_FOR_READING,url.path()+i18n("\nCould not create required pipe %1.").arg(fifoName));
      return;
   };

   int fifoFD=fileno(fifo);*/
   char buf[32*1024];

   kdDebug(KIO_SMB)<<"Smb::get() opened fifo: -"<<command<<"-"<<endl;

   result=0;
   int bytesRead(0);
   loopFinished=false;
   QByteArray array;

   do
   {
      int exitStatus=proc->exited();
      if (exitStatus!=-1)
      {
         //this should not happen !
         stopAfterError(url,false);
         close(fifoFD);
         remove(fifoName);
         return;
         /*loopFinished=true;
         kdDebug(KIO_SMB)<<"Smb::get(): smbclient exited with status "<<exitStatus<<endl;
         break;*/
      };

      struct timeval tv;
      tv.tv_sec=1;
      tv.tv_usec=0;
      fd_set readFDs;
      FD_ZERO(&readFDs);
      FD_SET(fifoFD,&readFDs);
      result=select(fifoFD+1,&readFDs,0,0,&tv);
      if (wasKilled())
         loopFinished=true;
      else if (result==1)
      {
         int i=::read(fifoFD,buf,32*1024);
         if (i==0)
            loopFinished=true;
         else
         {
            //kdDebug(KIO_SMB)<<"Smb::get(): read "<<i<<" bytes now, gives "<<bytesRead<<" overall"<<endl;
            bytesRead+=i;
            array.setRawData(buf, i);
            data( array );
            array.resetRawData(buf,i);

         }
      }
      else
         loopFinished=true;
   } while(!loopFinished);

   close(fifoFD);

   clearBuffer();
   bool stdoutEvent;
   result=proc->select(1,0,&stdoutEvent);
   if (stdoutEvent)
      readOutput(proc->fd());

   remove(fifoName);

   kdDebug(KIO_SMB)<<"Smb::get(): received -"<<m_stdoutBuffer<<"-"<<endl;
   if (stopAfterError(url,true))
      return;

   data( QByteArray() );
   finished();
};

QCString SmbProtocol::getMasterBrowser()
{
   kdDebug(KIO_SMB)<<"Smb::getMasterBrowser()"<<endl;

   QCString masterBrowser;
   ClientProcess *proc=new ClientProcess();
   QCStringList args;
   args<<QCString("-M")<<QCString("-");

   if (!proc->start("nmblookup",args))
   {
      kdDebug(KIO_SMB)<<"Smb::getMasterBrowser: starting nmblookup failed"<<endl;
   }
   else
   {
      clearBuffer();
      int exitStatus(-1);
      //we leave this loop if nmblookup exits
      while(exitStatus==-1)
      {
         bool stdoutEvent;
         proc->select(1,0,&stdoutEvent);
         exitStatus=proc->exited();
         if (exitStatus!=-1)
         {
            kdDebug(KIO_SMB)<<"Smb::getMasterBrowser() nmblookup exited with exitcode "<<exitStatus<<endl;
         };
         if (stdoutEvent)
         {
            readOutput(proc->fd());
         };
      };
      //now parse the output
      QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
      QTextIStream output(&outputString);
      QString line;

      while (!output.atEnd())
      {
         line=output.readLine();
         if ((line.contains("__MSBROWSE__")) && (line.contains("<")) && (line.contains(">")))
         {
            //this should be the line with the netbios name of the host
            kdDebug(KIO_SMB)<<"Smb::getMasterBrowser() using name from line -"<<line<<"-"<<endl;
            line=line.left(line.find("__MSBROWSE__")-1);
            line=line.stripWhiteSpace();
            masterBrowser="";
            for (int i=0; i<line.length(); i++)
               if ((line[i].isDigit()) || (line[i]=='.'))
                  masterBrowser+=line[i].latin1();
            break;
         };
         clearBuffer();
      };
   };
   kdDebug(KIO_SMB)<<"Smb::getMasterBrowser() ms is  -"<<masterBrowser<<"-"<<endl;

   return masterBrowser;
};

bool SmbProtocol::searchWorkgroups()
{
   QCString masterBrowser=getMasterBrowser();
   QCString nmbName=getNmbName(masterBrowser);
   m_workgroups.clear();

   kdDebug(KIO_SMB)<<"Smb::searchWorkgroups() nmbName: -"<<nmbName<<"- ip: -"<<masterBrowser<<"-"<<endl;
   ClientProcess *proc=new ClientProcess();
   QCStringList args;
   args<<QCString("-L")+nmbName;
   if (!m_user.isEmpty())
      args<<QCString("-U")+m_user.local8Bit();
   args<<QCString("-I")+masterBrowser;
//   if (!m_defaultWorkgroup.isEmpty())
//      args<<QCString("-W")+m_defaultWorkgroup.local8Bit();
   if (!proc->start("smbclient",args))
   {
      kdDebug(KIO_SMB)<<"Smb::searchWorkgroup() could not start smbclient"<<endl;
      delete proc;
      return false;
   };
   QString password(m_password);
   QString user(m_user);

   SmbReturnCode result(SMB_NOTHING);
   AuthInfo ai;
   bool firstLoop=true;
   //repeat until user/password is ok or the user cancels
   while (result=getShareInfo(proc,password,true), result==SMB_WRONGPASSWORD)
   {
      kdDebug(KIO_SMB)<<"Smb::listWorkgroups() failed with password"<<endl;
      //it failed with the default password
      delete proc;
      proc=0;

      KIO::AuthInfo authInfo;
      if (getAuth(authInfo,QString(m_nmbName),m_currentWorkgroup,"",user+"_"+QString(m_nmbName)+"_ListWgs",user,firstLoop))
/*      KIO::AuthInfo authInfo;
      authInfo.username = user;
      if (openPassDlg(authInfo))*/
      {
         ai=authInfo;
         user = authInfo.username;
         password = authInfo.password;
         proc=new ClientProcess();
         QCStringList tmpArgs;
         tmpArgs<<QCString("-L")+nmbName;
         if (!user.isEmpty())
            tmpArgs<<QCString("-U")+user.local8Bit();
         args<<QCString("-I")+masterBrowser;
//         if (!m_workgroup.isEmpty())
//            tmpArgs<<QCString("-W")+m_workgroup.local8Bit();
         if (!proc->start("smbclient",tmpArgs))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return false;
         };
      }
      else break;
   };
   //here smbclient has already exited
   if (proc!=0)
   {
      delete proc;
      proc=0;
   };

   KURL url("smb:/");
   //no error handling has happened up to now
   if (result==SMB_ERROR)
   {
      stopAfterError(url,false);
      return false;
   }
   //this happens only if the user pressed cancel
   else if (result==SMB_WRONGPASSWORD)
   {
      error(ERR_USER_CANCELED,"");
      return false;
   };

   if (stopAfterError(url,true))
      return false;

   //if we get here, we had success
   if (!ai.username.isEmpty()) //we used the AuthInfo
      cacheAuthentication(ai);

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;

   int mode(0);
   int wgPos(0);
   int masterPos(0);

   while (!output.atEnd())
   {
      line=output.readLine();
      if (mode==0)
      {
         if ((line.contains("Workgroup")) && (line.contains("Master")))
         {
            mode=1;
            wgPos=line.find("Workgroup");
            masterPos=line.find("Master");
         };
      }
      else if (mode==1)
      {
         if (line.contains("-----"))
         {
            mode=2;
         }
         else
         {
            return false;
         };
      }
      else if (mode==2)
      {
         kdDebug(KIO_SMB)<<"Smb::searchWorkgroups(): line: -"<<line.local8Bit()<<"-"<<endl;
         if (line.isEmpty())
            break;
         else
         {
            QString name=line.mid(wgPos,masterPos-wgPos);
            int end(name.length()-1);
            while (name[end]==' ')
               end--;
            name=name.left(end+1);

            QString master=line.mid(masterPos);
            end=master.length()-1;
            while (master[end]==' ')
               end--;
            master=master.left(end+1);
            m_workgroups[name.upper()]=master.upper();
         };
      };
   };
   return true;
};

void SmbProtocol::listWorkgroups()
{
   if (!searchWorkgroups())
      return;

   int totalNumber(0);
   UDSEntry entry;

   for (QMap<QString, QString>::ConstIterator it = m_workgroups.begin(); it != m_workgroups.end(); ++it )
   {
/*      printf( "%s: %s, %s earns %d\n",
              it.key().latin1(),
              it.data().surname().latin1(),
              it.data().forename().latin1(),
              it.data().salary() );*/

      entry.clear();
      UDSAtom atom;

      atom.m_uds = KIO::UDS_NAME;
      atom.m_str =it.key();
      entry.append( atom );

      atom.m_uds = KIO::UDS_SIZE;
      atom.m_long = 1024;
      entry.append(atom);

      atom.m_uds = KIO::UDS_MODIFICATION_TIME;
      atom.m_long = time(0);
      entry.append( atom );

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long=S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
      entry.append( atom );

      atom.m_uds = KIO::UDS_FILE_TYPE;
      atom.m_long =S_IFDIR;
      entry.append( atom );

      listEntry( entry, false);
      totalNumber++;
   }

   totalSize( totalNumber);
   listEntry( entry, true ); // ready

   finished();
};


void SmbProtocol::listHosts()
{
   kdDebug(KIO_SMB)<<"Smb::listHosts() "<<endl;
   //this is the nmb name
   QCString wgMaster=m_workgroups[m_currentWorkgroup.upper()].latin1();

   ClientProcess *proc=new ClientProcess();
   QCStringList args;
   args<<QCString("-L")+wgMaster;
   if (!m_user.isEmpty())
      args<<QCString("-U")+m_user.local8Bit();
//   args<<QCString("-I")+masterBrowser;
   if (!m_currentWorkgroup.isEmpty())
      args<<QCString("-W")+m_currentWorkgroup.local8Bit();
   if (!proc->start("smbclient",args))
   {
      kdDebug(KIO_SMB)<<"Smb::listHosts() could not start smbclient"<<endl;
      delete proc;
      return;
   };
   QString password(m_password);
   QString user(m_user);

   AuthInfo ai;
   bool firstLoop=true;
   SmbReturnCode result(SMB_NOTHING);
   //repeat until user/password is ok or the user cancels
   while (result=getShareInfo(proc,password), result==SMB_WRONGPASSWORD)
   {
      kdDebug(KIO_SMB)<<"Smb::listHosts() failed with password"<<endl;
      //it failed with the default password
      delete proc;
      proc=0;
      KIO::AuthInfo authInfo;
      if (getAuth(authInfo,QString(m_nmbName),m_currentWorkgroup,"",user+QString(m_nmbName)+"_ListHosts",user,firstLoop))
      {
         ai=authInfo;
         user = authInfo.username;
         password = authInfo.password;
         proc=new ClientProcess();
         QCStringList tmpArgs;
         tmpArgs<<QCString("-L")+wgMaster;
         if (!user.isEmpty())
            tmpArgs<<QCString("-U")+user.local8Bit();
//         args<<QCString("-I")+masterBrowser;
         if (!m_currentWorkgroup.isEmpty())
            tmpArgs<<QCString("-W")+m_currentWorkgroup.local8Bit();
         if (!proc->start("smbclient",tmpArgs))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return;
         };
      }
      else break;
   };
   //here smbclient has already exited
   if (proc!=0)
   {
      delete proc;
      proc=0;
   };

   KURL url("smb:/");
   //no error handling has happened up to now
   if (result==SMB_ERROR)
   {
      stopAfterError(url,false);
      return;
   }
   //this happens only if the user pressed cancel
   else if (result==SMB_WRONGPASSWORD)
   {
      error(ERR_USER_CANCELED,"");
      return;
   };

   if (stopAfterError(url,true))
      return;

   //if we get here, we had success
   if (!ai.username.isEmpty()) //we used the AuthInfo
      cacheAuthentication(ai);

   QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
   QTextIStream output(&outputString);
   QString line;

   int totalNumber(0);
   int mode(0);
   UDSEntry entry;

   int serverPos(0);
   int commentPos(0);

   while (!output.atEnd())
   {
      line=output.readLine();
      if (mode==0)
      {
         if ((line.contains("Server")) && (line.contains("Comment")))
         {
            mode=1;
            serverPos=line.find("Server");
            commentPos=line.find("Comment");
         };
      }
      else if (mode==1)
      {
         if (line.contains("-----"))
         {
            mode=2;
         }
         else
         {
            return;
         };
      }
      else if (mode==2)
      {
         kdDebug(KIO_SMB)<<"Smb::searchWorkgroups(): line: -"<<line.local8Bit()<<"-"<<endl;
         if (line.isEmpty())
            break;
         else
         {
            QString name=line.mid(serverPos,commentPos-serverPos);
            int end(name.length()-1);
            while (name[end]==' ')
               end--;
            name=name.left(end+1);

            entry.clear();
            UDSAtom atom;

            atom.m_uds = KIO::UDS_NAME;
            atom.m_str =name;
            entry.append( atom );

            atom.m_uds = KIO::UDS_SIZE;
            atom.m_long = 1024;
            entry.append(atom);

            atom.m_uds = KIO::UDS_MODIFICATION_TIME;
            atom.m_long = time(0);
            entry.append( atom );

            atom.m_uds = KIO::UDS_ACCESS;
            atom.m_long=S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
            entry.append( atom );

            atom.m_uds = KIO::UDS_FILE_TYPE;
            atom.m_long =S_IFDIR;
            entry.append( atom );

            listEntry( entry, false);
            totalNumber++;
         };
      };
   };
   totalSize( totalNumber);
   listEntry( entry, true ); // ready

   finished();
};

QCString SmbProtocol::getNmbName(QCString ipString)
{
   kdDebug(KIO_SMB)<<"Smb::getNmbname: ip is -"<<ipString<<"-"<<endl;
   //if we have the ip address, do a nmblookup -A address
   //and use the name <20>
   ClientProcess *proc=new ClientProcess();
   QCStringList args;
   args<<QCString("-A")<<ipString;
   QCString nmbName="";
   if (!proc->start("nmblookup",args))
   {
      kdDebug(KIO_SMB)<<"Smb::getMasterBrowser: starting nmblookup failed"<<endl;
   }
   else
   {
      clearBuffer();
      int exitStatus(-1);
      //we leave this loop if nmblookup exits
      while(exitStatus==-1)
      {
         bool stdoutEvent;
         proc->select(1,0,&stdoutEvent);
         //if smbclient exits, something went wrong
         exitStatus=proc->exited();
         if (exitStatus!=-1)
         {
            kdDebug(KIO_SMB)<<"Smb::getNmbName() nmblookup exited with exitcode "<<exitStatus<<endl;
         };
         if (stdoutEvent)
         {
            readOutput(proc->fd());
         }
      };
      //now parse the output
      QString outputString = QString::fromLocal8Bit(m_stdoutBuffer);
      QTextIStream output(&outputString);
      QString line;

      while (!output.atEnd())
      {
         line=output.readLine();
         if ((line.contains("<ACTIVE>")) && (line.contains("<20>")) && (!line.contains("<GROUP>")))
         {
            //this should be the line with the netbios name of the host
            kdDebug(KIO_SMB)<<"Smb::getNmbName() using name from line -"<<line<<"-"<<endl;
            line=line.left(line.find('<'));
            line=line.stripWhiteSpace();
            nmbName=line.local8Bit();
            break;
         };
      };
      clearBuffer();
   };
   delete proc;
   return nmbName;
};

void SmbProtocol::setHost(const QString& host, int /*port*/, const QString& /*user*/, const QString& /*pass*/)
{
   kdDebug(KIO_SMB)<<"Smb::setHost: -"<<host<<"- curr: -"<<m_currentHost<<"-"<<endl;
/* if (host.isEmpty())
   {
      error(ERR_UNKNOWN_HOST,i18n("To access the shares of a host, use smb://hostname\n\
To get a list of all hosts use lan:/ or rlan:/ .\n\
See the KDE Control Center under Network, LANBrowsing for more information."));
      return;
   };*/
   if (host==m_currentHost)
      return;
   QCString nmbName=host.local8Bit();
   QCString ipString("");
   //try to find the netbios name of this host
   //first try to get the ip address of the host
   struct hostent *hp=gethostbyname(host.local8Bit());
   if (hp==0)
   {
      //if this fails, we should assume that the given host name
      //is already the netbios name
      kdDebug(KIO_SMB)<<"Smb::setHost: gethostbyname returned 0"<<endl;
   }
   else
   {
      in_addr ip;
      memcpy(&ip, hp->h_addr, hp->h_length);
      ipString=inet_ntoa(ip);
      QCString tmp=getNmbName(ipString);
      if (!tmp.isEmpty())
         nmbName=tmp;
   };
   kdDebug(KIO_SMB)<<"Smb::setHost() nmbName is -"<<nmbName<<"-"<<endl;

   if (host==m_currentHost) return;
   m_ip=ipString;
   m_currentHost=host;
   m_nmbName=nmbName;
   m_processes.clear();
}

ClientProcess* SmbProtocol::getProcess(const QString& host, const QString& share)
{
   QString key=host+share;
   ClientProcess* proc=m_processes[key];
   kdDebug(KIO_SMB)<<"Smb::getProcess(): key: -"<<key<<"-"<<endl;
   if (proc!=0)
   {
      //oops, we still have it in the dict, but it already exited !
      //if the process exits anywhere else, we will detect this with
      //process->exited(), but not delete and remove the process.
      //this will be done the next time we come here, and we always come here :-)
      if (proc->exited()!=-1)
      {
         //we have autoDelete==true, so we don't need to delete proc explicitly
         m_processes.remove(key);
         proc=0;
         kdDebug(KIO_SMB)<<"Smb::getProcess(): process exited !"<<endl;
      }
   }
   if (proc!=0)
   {
      kdDebug(KIO_SMB)<<"Smb::getProcess(): found"<<endl;
      return proc;
   }
   //otherwise create the process
   proc=new ClientProcess();

   QCStringList args;
   args<<QCString("//")+m_nmbName+QCString("/")+share.local8Bit();
   if (!m_currentWorkgroup.isEmpty())
      args<<QCString("-W")+m_currentWorkgroup.local8Bit();
   if (!m_user.isEmpty())
      args<<QCString("-U")+m_user.local8Bit();
   if (!m_ip.isEmpty())
      args<<QCString("-I")+m_ip;

   if (!proc->start("smbclient",args))
   {
      error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
      return 0;
   };
   QString password(m_password);
   QString user(m_user);

   SmbReturnCode result(SMB_NOTHING);
   AuthInfo ai;
   bool firstLoop=true;
   //repeat until user/password is ok or the user cancels
   //although I hate stuff like the comma-operator
   //IMHO it is still better than while((result=waitUntilStarted())==SMB:WRONGPASSWORD)
   while (result=waitUntilStarted(proc,password,"smb: \\>"), result==SMB_WRONGPASSWORD)
   {
      kdDebug(KIO_SMB)<<"Smb::getProcess: failed with password"<<endl;
      //it failed with the default password
      delete proc;
      proc=0;
      KIO::AuthInfo authInfo;
      if (getAuth(authInfo,QString(m_nmbName),m_currentWorkgroup,share,user+"_"+share+QString(m_nmbName),user,firstLoop))
      {
         ai=authInfo;
         user = authInfo.username;
         password = authInfo.password;
         proc=new ClientProcess();
         QCStringList tmpArgs;
         tmpArgs<<QString("//"+host+"/"+share).local8Bit();
         if (!m_currentWorkgroup.isEmpty())
            tmpArgs<<QCString("-W")+m_currentWorkgroup.local8Bit();
         if (!user.isEmpty())
            tmpArgs<<QCString("-U")+user.local8Bit();
         if (!m_ip.isEmpty())
            args<<QCString("-I")+m_ip;
         if (!proc->start("smbclient",tmpArgs))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbclient"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return 0;
         }
      }
      else
      {
         //we don't want to care in the calling code
         error(ERR_USER_CANCELED,"");
         return 0;
      };
   };
   if (result==SMB_ERROR)
   {
      KURL url("smb://"+host+"/"+share);
      stopAfterError(url,false);
      return 0;
   };
   //finally we got it :-)
   kdDebug(KIO_SMB)<<"Smb::getProcess: succeeded"<<endl;
   m_processes.insert(key,proc);
   //if we get here, we had success
   if (!ai.username.isEmpty()) //we used the AuthInfo
      cacheAuthentication(ai);
   return proc;
};


void SmbProtocol::special( const QByteArray & data)
{
   kdDebug(KIO_SMB)<<"Smb::special()"<<endl;
   int tmp;
   QDataStream stream(data, IO_ReadOnly);
   stream >> tmp;
   //mounting and umounting are both blocking, "guarded" by a SIGALARM in the future
   switch (tmp)
   {
   case 1:
   case 3:
      {
         QString remotePath, mountPoint, user, password;
         stream >> remotePath >> mountPoint >> user >> password;
         QStringList sl=QStringList::split("/",remotePath);
         QString share,host;
         if (sl.count()>=2)
         {
            host=(*sl.at(0)).mid(2);
            share=(*sl.at(1));
            kdDebug(KIO_SMB)<<"special() host -"<<host.latin1()<<"- share -"<<share.latin1()<<"-"<<endl;
         };

         if (tmp==3)
            makeDirHier(mountPoint);
         password=m_password;
         user=m_user;
         ClientProcess *proc=new ClientProcess();
         QCStringList args;
         args<<remotePath.local8Bit()<<mountPoint.local8Bit();
         kdDebug(KIO_SMB)<<"Smb::special() rem: -"<<remotePath.local8Bit()<<"- mount: -"<<mountPoint.local8Bit()<<"-"<<endl;

         QCString opts="-o";
         if (!user.isEmpty())
         {
            opts+="username=";
            opts+=user.local8Bit();
            kdDebug(KIO_SMB)<<"Smb::special() user -"<<user.local8Bit()<<"-"<<endl;
         };
         if (!password.isEmpty())
         {
            opts+=",password=";
            opts+=password.local8Bit();
         };
         if (opts!="-o")
         {
            args<<opts;
            kdDebug(KIO_SMB)<<"Smb::special() adding opts"<<endl;
         };
         kdDebug(KIO_SMB)<<"Smb::special() opts-"<<opts<<"-"<<endl;
         if (!proc->start("smbmount",args))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbmount"+i18n("\nMake sure that the samba package is installed properly on your system."));
            delete proc;
            return;
         };

         bool firstLoop=true;
         AuthInfo ai;
         SmbReturnCode result(SMB_NOTHING);
         //repeat until user/password is ok or the user cancels
         //although I hate stuff like the comma-operator
         //IMHO it is still better than while((result=waitUntilStarted())==SMB:WRONGPASSWORD)
         while (result=waitUntilStarted(proc,password,0), result==SMB_WRONGPASSWORD)
         {
            kdDebug(KIO_SMB)<<"Smb::getProcess: failed with password"<<endl;
            //it failed with the default password
            delete proc;
            proc=0;
            KIO::AuthInfo authInfo;
            if (getAuth(authInfo,host,"",share,user+"_"+share+host,user,firstLoop))
/*            KIO::AuthInfo authInfo;
            authInfo.username = user;
            if (openPassDlg(authInfo))*/
            {
               ai=authInfo;
               user = authInfo.username;
               password = authInfo.password;

               proc=new ClientProcess();
               QCStringList tmpArgs;
               tmpArgs<<remotePath.local8Bit()<<mountPoint.local8Bit();
               kdDebug(KIO_SMB)<<"Smb::special() rem: -"<<remotePath.local8Bit()<<"- mount: -"<<mountPoint.local8Bit()<<"-"<<endl;

               QCString opts="-o";
               if (!user.isEmpty())
               {
                  opts+="username=";
                  opts+=user.local8Bit();
                  kdDebug(KIO_SMB)<<"Smb::special() user -"<<user.local8Bit()<<"-"<<endl;
               };
               if (!password.isEmpty())
               {
                  opts+=",password=";
                  opts+=password.local8Bit();
               };
               if (opts!="-o")
               {
                  tmpArgs<<opts;
                  kdDebug(KIO_SMB)<<"Smb::special() adding opts"<<endl;
               };
               if (!proc->start("smbmount",tmpArgs))
               {
                  error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbmount"+i18n("\nMake sure that the samba package is installed properly on your system."));
                  delete proc;
                  return;
               };
            }
            else
            {
               //we don't want to care in the calling code
               error(ERR_USER_CANCELED,"");
               return;
            };
         };
         if (result==SMB_ERROR)
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "wrlg sudgäajäl jtlw4jrt");
            return;
         };
         delete proc;
         //if we get here, we had success
         if (!ai.username.isEmpty())
            cacheAuthentication(ai);
      }
      break;
   case 2:
   case 4:
      {
         QString mountPoint;
         stream >> mountPoint;
         ClientProcess proc;
         QCStringList args;
         args<<mountPoint.local8Bit();
         if (!proc.start("smbumount",args))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount"+i18n("\nMake sure that the samba package is installed properly on your system."));
            return;
         };
         clearBuffer();
         while (1)  //until smbumount exits
         {
            bool stdoutEvent;
            proc.select(1,0,&stdoutEvent);
            int exitStatus=proc.exited();
            if (exitStatus!=-1)
            {
               kdDebug(KIO_SMB)<<"Smb::waitUntilStarted() smbclient exited with exitcode "<<exitStatus<<endl;
               if (tmp==4)
               {
                  QDir dir(mountPoint);
                  dir.cdUp();
                  dir.rmdir(mountPoint);
                  QString p=dir.path();
                  dir.cdUp();
                  dir.rmdir(p);
               };
               if (exitStatus!=0)
               {
                  if (m_stdoutSize>0)
                     kdDebug(KIO_SMB)<<"Smb::waitUntilStarted(): received: -"<<m_stdoutBuffer<<"-"<<endl;
                  error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount");
               }
               else
                  finished();
               return;
            };
            if (stdoutEvent)
               readOutput(proc.fd());
         };
      }
      break;
   default:
      break;
   }
   finished();
};

