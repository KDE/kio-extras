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

#include "kio_smb.h"
#include <kstandarddirs.h>
#include <qcstring.h>
#include <unistd.h>
#include <qdir.h>
#include <kprocess.h>

void SMBSlave::readOutput(KProcess *, char *buffer, int buflen)
{
    mybuf += QString::fromLocal8Bit(buffer, buflen);
}

void SMBSlave::special( const QByteArray & data)
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
            kdDebug(KIO_SMB)<<"special() host -"<< host <<"- share -" << share <<"-"<<endl;
         }

         if (tmp==3) {
             if (!KStandardDirs::makeDir(mountPoint)) {
                 error(KIO::ERR_COULD_NOT_MKDIR, mountPoint);
                 return;
             }
         }
         mybuf.truncate(0);

         KProcess proc;
         proc << "mount";
         proc << "-o guest";
         proc << "-t smbfs";
         proc << remotePath.local8Bit();
         proc << mountPoint.local8Bit();
         connect(&proc, SIGNAL( receivedStdout(KProcess *, char *, int )),
                 SLOT(readOutput(KProcess *, char *, int)));

         if (!proc.start( KProcess::Block, KProcess::AllOutput ))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "mount"+i18n("\nMake sure that the samba package is installed properly on your system."));
            return;
         }
         kdDebug(KIO_SMB) << "smbmount exit " << proc.exitStatus() << endl;
         if (proc.exitStatus() != 0) {
             error( KIO::ERR_COULD_NOT_MOUNT, mybuf);
         }
         finished();
      }
      break;
   case 2:
   case 4:
      {
/*         QString mountPoint;
         stream >> mountPoint;
         ClientProcess proc;
         QCStringList args;
         args<<mountPoint.local8Bit();
         if (!proc.start("smbumount",args))
         {
            error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount"+i18n("\nMake sure that the samba package is installed properly on your system."));
            return;
         }
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
               }
               if (exitStatus!=0)
               {
                  if (m_stdoutSize>0)
                     kdDebug(KIO_SMB)<<"Smb::waitUntilStarted(): received: -"<<m_stdoutBuffer<<"-"<<endl;
                  error( KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount");
               }
               else
                  finished();
               return;
            }
            if (stdoutEvent)
               readOutput(proc.fd());
               }*/
      }
      break;
   default:
      break;
   }
   finished();
}

#include "kio_smb.moc"
