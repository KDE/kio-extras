/* This file is part of the KDE project
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>

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
   #include <kservice.h>
   #include <kservicegroup.h>
   #include <kstandarddirs.h>
   class SettingsProtocol : public KIO::SlaveBase
   {
   public:
      enum RunMode { SettingsMode, ProgramsMode };
      SettingsProtocol(const QCString &protocol, const QCString &pool, const QCString &app);
      virtual ~SettingsProtocol();
#if 0
      virtual void get( const KURL& url );
#endif
      virtual void stat(const KURL& url);
      virtual void listDir(const KURL& url);
      void listRoot();
      KServiceGroup::Ptr findGroup(QString relPath);

   private:
	DCOPClient *m_dcopClient;
	RunMode m_runMode;
  };

  extern "C" {
      int kdemain( int, char **argv )
      {
          kdDebug()<<"kdemain for settings kioslave"<<endl;
          KInstance instance( "kio_settings" );
          SettingsProtocol slave(argv[1], argv[2], argv[3]);
          slave.dispatchLoop();
          return 0;
      }
  }



static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime,const QString& iconName);
static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime,const QString& iconName);

SettingsProtocol::SettingsProtocol( const QCString &protocol, const QCString &pool, const QCString &app): SlaveBase( protocol, pool, app )
{
	// Adjusts which part of the K Menu to virtualize.
	if( protocol == "programs" ) m_runMode = ProgramsMode;
	else m_runMode = SettingsMode;

	m_dcopClient=new DCOPClient();
	if (!m_dcopClient->attach())
	{
		kdDebug()<<"ERROR WHILE CONNECTING TO DCOPSERVER"<<endl;
	}
}

SettingsProtocol::~SettingsProtocol()
{
	delete m_dcopClient;
}

KServiceGroup::Ptr SettingsProtocol::findGroup(QString relPath) {
	QString alreadyFound;
	QString nextPart="";;
	QStringList rest;
	kdDebug()<<"Trying harder to find group "<<relPath<<endl;
	if (relPath.startsWith("Settings/")) {
		alreadyFound="Settings/";
		rest=QStringList::split("/",relPath.right(relPath.length()-9));
		kdDebug()<<"Supported root Settings detected"<<endl;
		for (int i=0;i<rest.count();i++)
			kdDebug()<<"Item ("<<*rest.at(i)<<")"<<endl;
	} else {

		return 0;
	}
	while (!rest.isEmpty()) {
		KServiceGroup::Ptr tmp=KServiceGroup::group(alreadyFound);
		if (!tmp || !tmp->isValid()) return 0;
		
       		KServiceGroup::List list = tmp->entries(true, true);

       		KServiceGroup::List::ConstIterator it = list.begin();

		bool found=false;
       		for (; it != list.end(); ++it) {

            		KSycocaEntry * e = *it;

	               	if (e->isType(KST_KServiceGroup)) {

        	            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
                	    if ((g->caption()==rest.front()) || (g->name()==alreadyFound+rest.front())) { 
				kdDebug()<<"Found group with caption "<<g->caption()<<" with real name: "<<g->name()<<endl;
				found=true;
				rest.remove(rest.begin());
				alreadyFound=g->name();
				kdDebug()<<"ALREADY FOUND: "<<alreadyFound<<endl;
				break;
			    }
			}
		}
		if (!found) {
			kdDebug()<<"Group with caption "<<rest.front()<<" not found within "<<alreadyFound<<endl;
			return 0;
		}

		
	}
	return KServiceGroup::group(alreadyFound);
}


void SettingsProtocol::stat(const KURL& url)
{
        QStringList     path = QStringList::split('/', url.encodedPathAndQuery(-1), false);
        KIO::UDSEntry   entry;
        QString mime;
	QString mp;

	QString relPath=url.path();

	switch( m_runMode )
	{
		case( SettingsMode ):
			if (!relPath.startsWith("/Settings")) relPath="Settings"+relPath;
			else relPath=relPath.right(relPath.length()-1);
			break;

		case( ProgramsMode ):
			relPath=relPath.right(relPath.length()-1);
			break;
	}

	kdDebug()<<"SettingsProtocol: stat for: "<<relPath<<endl;
	KServiceGroup::Ptr grp = KServiceGroup::group(relPath);	

	if (!grp || !grp->isValid()) {

		grp=findGroup(relPath);
		if (!grp || !grp->isValid()) {
	                error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown settings folder"));
	                return;
		}
	}

	switch( m_runMode )
	{
		case( SettingsMode ):
			createDirEntry(entry, i18n("Settings"), url.url(), "inode/directory",grp->icon());
			break;

		case( ProgramsMode ):
			createDirEntry(entry, i18n("Programs"), url.url(), "inode/directory",grp->icon());
			break;
	}

	statEntry(entry);
	finished();
	return;

}



void SettingsProtocol::listDir(const KURL& url)
{
	        
	KIO::UDSEntry   entry;
	uint count=0;
	
	QString relPath=url.path();

	switch( m_runMode )
	{
		case( SettingsMode ):
			if (!relPath.startsWith("/Settings")) relPath="Settings"+relPath;
			else relPath=relPath.right(relPath.length()-1);
			break;

		case( ProgramsMode ):
			relPath=relPath.right(relPath.length()-1);
			break;
	};

	if (relPath.at(relPath.length()-1)!='/') relPath+="/";

	kdDebug()<<"SettingsProtocol: "<<relPath<<"***********************"<<endl;
	KServiceGroup::Ptr root = KServiceGroup::group(relPath);
    

	if (!root || !root->isValid()) {

		root=findGroup(relPath);
		if (!root || !root->isValid()) {
	                error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown settings folder"));
	                return;
		}
	}

       KServiceGroup::List list = root->entries(true, true);
	
       KServiceGroup::List::ConstIterator it = list.begin();


       for (; it != list.end(); ++it) {

            KSycocaEntry * e = *it;

 	       if (e->isType(KST_KServiceGroup)) {

        	    KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
	            QString groupCaption = g->caption();

        	    // Avoid adding empty groups.
	            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
        	    if (subMenuRoot->childCount() == 0)
                	continue;

	            // Ignore dotfiles.
        	    if ((g->name().at(0) == '.'))
	                continue;

		    count++;
                    QString relPath=g->relPath();

		    // Do not display the "Settings" menu group in Programs Mode.
		    if( (m_runMode == ProgramsMode) && relPath.startsWith( "Settings" ) )
		    {
			kdDebug() << "SettingsProtocol: SKIPPING entry programs:/" << relPath << endl;
			continue;
		    }

		    switch( m_runMode )
		    {
			case( SettingsMode ):
			    relPath=relPath.right(relPath.length()-9); //Settings/ ==9
			    kdDebug() << "SettingsProtocol: adding entry settings:/" << relPath << endl;
			    createDirEntry(entry, groupCaption, "settings:/"+relPath, "inode/directory",g->icon());
			    break;

			case( ProgramsMode ):
			    kdDebug() << "SettingsProtocol: adding entry programs:/" << relPath << endl;
			    createDirEntry(entry, groupCaption, "programs:/"+relPath, "inode/directory",g->icon());
			    break;
		    }
	            listEntry(entry, false);
	        }
        	else {
	            KService::Ptr s(static_cast<KService *>(e));
        	    //insertMenuItem(s, id++, -1, &suppressGenericNames);
		    QString desktopEntryPath=s->desktopEntryPath();
		    desktopEntryPath="file:"+locate("apps",desktopEntryPath);
		    createFileEntry(entry,s->name(),desktopEntryPath, "application/x-desktop",s->icon());
	            listEntry(entry, false);
	        }
	}

        totalSize(count);
	listEntry(entry, true);
	finished();
}



void addAtom(KIO::UDSEntry& entry, unsigned int ID, long l, const QString& s = QString::null)
{
        KIO::UDSAtom    atom;
        atom.m_uds = ID;
        atom.m_long = l;
        atom.m_str = s;
        entry.append(atom);
}

static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime,const QString& iconName)
{
        entry.clear();
        addAtom(entry, KIO::UDS_NAME, 0, name);
//        addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);//REG);
        addAtom(entry, KIO::UDS_URL, 0, url);
        addAtom(entry, KIO::UDS_ACCESS, 0500);
        addAtom(entry, KIO::UDS_MIME_TYPE, 0, mime);
        addAtom(entry, KIO::UDS_SIZE, 0);
        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "application/x-desktop");
	addAtom(entry, KIO::UDS_CREATION_TIME,1);
	addAtom(entry, KIO::UDS_MODIFICATION_TIME,time(0));
//	addAtom(entry, KIO::UDS_ICON_NAME,0,iconName);

}


static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime,const QString& iconName)
{
        entry.clear();
        addAtom(entry, KIO::UDS_NAME, 0, name);
        addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
        addAtom(entry, KIO::UDS_ACCESS, 0500);
        addAtom(entry, KIO::UDS_MIME_TYPE, 0, mime);
        addAtom(entry, KIO::UDS_URL, 0, url);
        addAtom(entry, KIO::UDS_SIZE, 0);
        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, KIO::UDS_ICON_NAME,0,iconName);
//        addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "application/x-desktop");
}
