/* This file is part of the KDE project
   Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>

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

   #include <kio/slavebase.h>
   #include <kinstance.h>
   #include <kdebug.h>
   #include <stdlib.h>
   #include <qtextstream.h>
   #include <klocale.h>
   #include <sys/stat.h>
   #include <dcopclient.h>
   #include <qdatastream.h>
   #include <time.h>
   #include <kprocess.h>

   class HelloProtocol : public KIO::SlaveBase
   {
   public:
      HelloProtocol( const QCString& protocol ,const QCString &pool, const QCString &app);
      virtual ~HelloProtocol();
#if 0
      virtual void get( const KURL& url );
#endif
      virtual void stat(const KURL& url);
      virtual void listDir(const KURL& url);
      void listRoot();
   private:
	DCOPClient *m_dcopClient;
	uint mountpointMappingCount();
	bool fullMode;
	QString deviceNode(uint id);
	bool deviceMounted(const QString dev);
	bool deviceMounted(int);
	QString mountPoint(const QString dev);
	QString mountPoint(int);
	QString deviceType(int);
	QStringList deviceList();
	QStringList deviceInfo(const QString name);

	QStringList kmobile_list( QString deviceName );
  };

  extern "C" {
      int kdemain( int, char **argv )
      {
          kdDebug()<<"kdemain for devices"<<endl;
          KInstance instance( "kio_devices" );
          HelloProtocol slave(argv[1],argv[2], argv[3]);
          slave.dispatchLoop();
          return 0;
      }
  }



static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime);
static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime);

HelloProtocol::HelloProtocol( const QCString& protocol, const QCString &pool, const QCString &app): 
		SlaveBase(protocol,  pool, app )
{
	kdDebug()<<"HelloProtocol: Called with slavename:"<<protocol<<endl;
	if (protocol=="system") fullMode=true; else fullMode=false;
	m_dcopClient=new DCOPClient();
	if (!m_dcopClient->attach())
	{
		kdDebug()<<"ERROR WHILE CONNECTING TO DCOPSERVER"<<endl;
	}
}

HelloProtocol::~HelloProtocol()
{
	delete m_dcopClient;
}

void HelloProtocol::stat(const KURL& url)
{
        QStringList     path = QStringList::split('/', url.encodedPathAndQuery(-1), false);
        KIO::UDSEntry   entry;
        QString mime;
	QString mp;

	switch (path.count())
	{
		case 0:
			if (fullMode)
			        createDirEntry(entry, i18n("System"), "system:/", "inode/directory");
			else
			        createDirEntry(entry, i18n("Devices"), "devices:/", "inode/directory");
		        statEntry(entry);
		        finished();
			break;
		default:

                QStringList info=deviceInfo(url.fileName());

                if (info.empty())
                {
                        error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown device: %1").arg(url.fileName()));
                        return;
                }


                QStringList::Iterator it=info.begin();
                if (it!=info.end())
                {
                        QString device=*it; ++it;
                        if (it!=info.end())
                        {
				++it;
				if (it!=info.end())
				{
	                                QString mp=*it; ++it;++it;
	                                if (it!=info.end())
	                                {
	                                        bool mounted=((*it)=="true");
	                                        if (mounted)
	                                        {
//	                                                if (mp=="/") mp="";
	                                                redirection(KURL( mp ));
	                                                finished();
	                                        }
	                                        else
	                                        {
							if (mp.startsWith("file:/"))
							{
			        	        	        KProcess *proc = new KProcess;
        		        	                 	*proc << "kio_devices_mounthelper";
                		                 		*proc << "-m" << url.url();
	                        		         	proc->start(KProcess::Block);
        	                        		 	delete proc;

	        		                        	redirection(KURL( mp ));
        		        	                	finished();
							}
							else
								error(KIO::ERR_SLAVE_DEFINED,i18n("Device not accessible"));


//	                                                error(KIO::ERR_SLAVE_DEFINED,i18n("Device not mounted"));
	                                        }
	                                        return;
					}
                                }
                        }
                }
                error(KIO::ERR_SLAVE_DEFINED,i18n("Illegal data received"));
		return;
		break;
        }

}



void HelloProtocol::listDir(const KURL& url)
{
	kdDebug()<<"HELLO PROTOCOLL::listdir: "<<url.url()<<endl;
	if ((url==KURL("devices:/")) || (url==KURL("system:/")))
		listRoot();
	else
	{
		QStringList info=deviceInfo(url.fileName());

		if (info.empty())
		{
			error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown device %1").arg(url.fileName()));
			return;
		}


		QStringList::Iterator it=info.begin();
		if (it!=info.end())
		{
			QString device=*it; ++it;
			if (it!=info.end())
			{
				++it;
				if (it!=info.end())
				{
					QString mp=*it; ++it;++it;
					if (it!=info.end())
					{
						bool mounted=((*it)=="true");
						if (mounted)
						{
//							if (mp=="/") mp="";
							redirection(KURL( mp ));
							finished();
						}
						else
						{
							if (mp.startsWith("file:/"))
							{
			        	        	        KProcess *proc = new KProcess;
        		        	                 	*proc << "kio_devices_mounthelper";
                		                 		*proc << "-m" << url.url();
	                        		         	proc->start(KProcess::Block);
								int ec=0;
								if (proc->normalExit()) ec=proc->exitStatus();
        	                        		 	delete proc;

								if (ec)
								{
									error(KIO::ERR_SLAVE_DEFINED,i18n("Device not mounted"));
									finished();
								}
								else
								{
		        		                        	redirection(KURL( mp ));
        			        	                	finished();
								}
							}
							else
								error(KIO::ERR_SLAVE_DEFINED,i18n("Device not accessible"));
						}
						return;
					}
				}
			}
		}
		error(KIO::ERR_SLAVE_DEFINED,i18n("Illegal data received"));
	}
}

uint HelloProtocol::mountpointMappingCount()
{
	QByteArray data;
	QByteArray param;
	QCString retType;
	uint count=0;
      if ( m_dcopClient->call( "kded",
		 "mountwatcher", "mountpointMappingCount()", param,retType,data,false ) )
      {
	QDataStream stream1(data,IO_ReadOnly);
	stream1>>count;
      }
      return count;
}

QString HelloProtocol::deviceNode(uint id)
{
	QByteArray data;
	QByteArray param;
	QCString retType;
	QString retVal;
	QDataStream streamout(param,IO_WriteOnly);
	streamout<<id;
	if ( m_dcopClient->call( "kded",
		 "mountwatcher", "devicenode(int)", param,retType,data,false ) )
      {
	QDataStream streamin(data,IO_ReadOnly);
	streamin>>retVal;
      }
      return retVal;

}

bool HelloProtocol::deviceMounted(const QString dev)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        bool retVal=false;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<dev;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "mounted(QString)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}


QStringList HelloProtocol::kmobile_list(const QString deviceName)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QStringList retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<deviceName;
        if ( m_dcopClient->call( "kmobile",
                 "kmobileIface", "kio_devices_deviceInfo(QString)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}


QStringList HelloProtocol::deviceInfo(QString name)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QStringList retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<name;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "basicDeviceInfo(QString)", param,retType,data,false ) )
        {
          QDataStream streamin(data,IO_ReadOnly);
          streamin>>retVal;
        }
	// kmobile support
	if (retVal.isEmpty())
		retVal = kmobile_list(name);

	return retVal;
}


bool HelloProtocol::deviceMounted(int id)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        bool retVal=false;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<id;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "mounted(int)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}


QStringList HelloProtocol::deviceList()
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QStringList retVal;
        QDataStream streamout(param,IO_WriteOnly);
	
	kdDebug()<<"list dir: Fullmode=="<<fullMode<<endl;
	QString dcopFun=fullMode?"basicSystemList()":"basicList()";
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", dcopFun.utf8(), param,retType,data,false ) )
        {
          QDataStream streamin(data,IO_ReadOnly);
          streamin>>retVal;
        }
	else
	{
		retVal.append(QString::fromLatin1("!!!ERROR!!!"));
	}
	// add mobile devices info (kmobile)
	retVal += kmobile_list(QString::null);

      return retVal;
}

QString HelloProtocol::mountPoint(const QString dev)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QString retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<dev;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "mountpoint(QString)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}

QString HelloProtocol::mountPoint(int id)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QString retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<id;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "mountpoint(int)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}



QString HelloProtocol::deviceType(int id)
{
        QByteArray data;
        QByteArray param;
        QCString retType;
        QString retVal;
        QDataStream streamout(param,IO_WriteOnly);
        streamout<<id;
        if ( m_dcopClient->call( "kded",
                 "mountwatcher", "type(int)", param,retType,data,false ) )
      {
        QDataStream streamin(data,IO_ReadOnly);
        streamin>>retVal;
      }
      return retVal;
}



void HelloProtocol::listRoot()
{
	KIO::UDSEntry   entry;
	uint count;

	QStringList list=deviceList();
	count=0;
        for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
	{
		if ((*it)=="!!!ERROR!!!")
		{
                        error(KIO::ERR_SLAVE_DEFINED,i18n("The KDE mountwatcher is not running. Please activate it in Control Center->KDE Components->Service Manager, if you want to use the devices:/ protocol"));
                        return;
		}
// FIXME: look for the real ending
		QString url="devices:/"+(*it); ++it;
		QString name=*it; ++it;
		++it; ++it;
		QString type=*it; ++it; ++it;
		createFileEntry(entry,name,url,type);
		listEntry(entry,false);
		count++;
	}
        totalSize(count);
        listEntry(entry, true);


        // Jobs entry

        // finish
        finished();
}

#if 0
 void HelloProtocol::get( const KURL& url )
 {
/*	mimeType("application/x-desktop");
	QCString output;
	output.sprintf("[Desktop Action Format]\n"
			"Exec=kfloppy\n"
			"Name=Format\n"
			"[Desktop Entry]\n"
			"Actions=Format\n"
			"Dev=/dev/fd0\n"
			"Encoding=UTF-8\n"
			"Icon=3floppy_mount\n"
			"MountPoint=/media/floppy\n"
			"ReadOnly=false\n"
			"Type=FSDevice\n"
			"UnmountIcon=3floppy_unmount\n"
			);
     data(output);
     finished();
 */
  redirection("file:/");
  //finished();
}
#endif

void addAtom(KIO::UDSEntry& entry, unsigned int ID, long l, const QString& s = QString::null)
{
        KIO::UDSAtom    atom;
        atom.m_uds = ID;
        atom.m_long = l;
        atom.m_str = s;
        entry.append(atom);
}

static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime)
{
        entry.clear();
        addAtom(entry, KIO::UDS_NAME, 0, name);
        addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);//REG);
        addAtom(entry, KIO::UDS_URL, 0, url);
        addAtom(entry, KIO::UDS_ACCESS, 0500);
       if (mime.startsWith("icon:")) {
                kdDebug()<<"setting prefered icon:"<<mime.right(mime.length()-5)<<endl;
                addAtom(entry,KIO::UDS_ICON_NAME,0,mime.right(mime.length()-5));
                addAtom(entry,KIO::UDS_MIME_TYPE,0,"inode/directory");
        }
	else
	        addAtom(entry, KIO::UDS_MIME_TYPE, 0, mime);
        addAtom(entry, KIO::UDS_SIZE, 0);
        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, KIO::UDS_CREATION_TIME,1);
	addAtom(entry, KIO::UDS_MODIFICATION_TIME,time(0));
}


static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime)
{
        entry.clear();
        addAtom(entry, KIO::UDS_NAME, 0, name);
        addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
        addAtom(entry, KIO::UDS_ACCESS, 0500);
	kdDebug()<<"DEVICES: "<<mime<<endl;
	if (mime.startsWith("icon:")) {
		kdDebug()<<"setting prefered icon:"<<mime.right(mime.length()-5)<<endl;
		addAtom(entry,KIO::UDS_ICON_NAME,0,mime.right(mime.length()-5));
		addAtom(entry,KIO::UDS_MIME_TYPE,0,"inode/directory");	
	}
        else {
		addAtom(entry, KIO::UDS_MIME_TYPE, 0, mime);
	}
        addAtom(entry, KIO::UDS_URL, 0, url);
        addAtom(entry, KIO::UDS_SIZE, 0);
        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "inode/directory");

//        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "application/x-desktop");
}
