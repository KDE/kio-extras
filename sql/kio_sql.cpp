/*
   This file is part of the KDB libraries
   Copyright (c) 2000 Praduroux Alessandro <pradu@thekompany.com>
 
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
#include "kio_sql.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream.h>

#include <qapp.h>

#include <kinstance.h>
#include <kdebug.h>
#include <klocale.h>

#include <kio/global.h>

#include <kdb/dbengine.h>
#include <kdb/plugin.h>
#include <kdb/connection.h>
#include <kdb/database.h>
#include <kdb/table.h>
#include <kdb/query.h>
#include <kdb/recordset.h>
#include <kdb/field.h>
#include <kdb/recordsetiterator.h>

#define MIMETYPE "application/x-kugar"

using namespace KIO;

extern "C" {
    int kdemain(int argc, char **argv);
}

              
int kdemain( int argc, char **argv )
{
    KInstance instance( "kio_sql" );
    QApplication app(argc, argv, false);
    
    kdDebug(7111) << "Starting " << getpid() << endl;

    if (argc != 4) {
        cerr << "Usage: kio_sql protocol domain-socket1 domain-socket2" << endl;
        exit(-1);
    }

    KDBProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kdDebug(7111) << "Done" << endl;
    return 0;
}


KDBProtocol::KDBProtocol( const QCString &pool, const QCString &app)
    :SlaveBase( "sql", pool, app )
{
    
    kdDebug(7111) << k_funcinfo << endl;


}


KDBProtocol::~KDBProtocol()
{
    kdDebug(7111) << k_funcinfo << endl;

}

void
KDBProtocol::setHost(const QString& host, int ip, const QString& user, const QString& pass)
{
    kdDebug(7111) << k_funcinfo << endl;

    m_host = host;
    m_port = ip;
    m_user = user;
    m_pwd = pass;
}

static QString makeIndent(int ident)
{
    QString tabs;
    tabs.fill(' ',ident*4);
    return tabs;
}


void
KDBProtocol::get( const KURL& url )
{
    kdDebug( 7111 ) << "=============== GET " << url.prettyURL() << " ===============" << endl;

    mimeType(MIMETYPE);

    //    int type = fileType(url.path());
    int indent = 0;
    KDB_ULONG pos = 0;

    QString filename = url.fileName();
    QString host = url.host();
    int port = url.port();
    QString path = url.path();
    QString user = url.user();
    QString pwd;
    QString plugin;
    QString database;
    QString extra; // needed for check auth ???

    //build the key for password caching
    QStringList l = QStringList::split("/", path);
    if (l.count() > 0)
        plugin = l[0]; // the first part of the path is the plugin

    if (l.count() > 1)
        database = l[1]; //the second is the database

    KURL checkUrl(QString("sql://%1:%2").arg(host).arg(port));

    KDB::Database *base = 0;

    KIO::AuthInfo info;
    info.url = checkUrl;
    info.username = user;
    info.password = pwd;
    info.realmValue = checkUrl.url() + "/" + plugin + "/" + user;
    info.verifyPath = false;
    
    if (!checkCachedAuthentication( info ) ) { // check failed, ask for user/pwd

        info.prompt = i18n("Please enter username and password for %1").arg(host);
        // try until you get a valid password or a cancel
        while (true) {
            
            if ( !openPassDlg(info) ) {
                error(ERR_ACCESS_DENIED,path);
                return;
            }
            
            base = DBENGINE->openDatabase( plugin, host, port, info.username, info.password, database);
            if (base) {
                // ok, got the rigth user/pwd, store them
                cacheAuthentication(info);
                break;
            }
        }
    } else {
        base = DBENGINE->openDatabase( plugin, host, port, info.username, info.password, database);
        if (!base) {
            error(ERR_ACCESS_DENIED,path);
            return;
        }
    }
    
    KDB::RecordsetPtr rset;
    KDB::TablePtr table = base->getTable(filename);
    if (table) {
        rset = table->openRecordset();
    } else {
        KDB::QueryPtr query = base->getQuery(filename);
        rset = query->openRecordset();
    }

    totalSize(rset->count());
    QByteArray content;
    QString str;

    str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    str += "<!DOCTYPE KugarData [\n";
    str += makeIndent(++indent);
    str += "<!ELEMENT KugarData ( Row* )>\n";
    str += makeIndent(indent);
    str += "<!ATTLIST KugarData\n";
    str += makeIndent(++indent);
    str += "Template          CDATA  #REQUIRED>\n";
    str += makeIndent(--indent);
    str += "<!ELEMENT Row EMPTY>\n";
    str += makeIndent(indent);
    str += "<!ATTLIST Row\n";

    KDB::FieldList f = rset->fields();
    KDB::FieldIterator itf(f);

    indent++;
    while (itf.current()) {
        str += makeIndent(indent);
        str += QString("%1       CDATA #REQUIRED\n").arg(itf.current()->name());
        ++itf;
    }
    
    str += makeIndent(--indent);
    str += ">\n";
    str += makeIndent(--indent);
    str += "]>\n\n";
    content.duplicate(str.utf8(), str.length());
    data(content);


    int numFields = f.count();
    str = makeIndent(indent) + QString("<KugarData>\n");
    content.duplicate(str.utf8(), str.length());
    data(content);
    ++indent;
    
    KDB::RecordsetIterator itr = rset->begin();
    KDB::RecordPtr ptr = itr.current();
    do {
        str = makeIndent(indent) + QString("<Row ");
        for (int i = 0;i < numFields; i++) {
            str += QString("%1=\"%2\" ").arg(ptr->field(i).name()).arg(ptr->field(i)->toString());
        }
        
        str += QString("/>\n");
        content.duplicate(str.utf8(), str.length());
        data(content);
        pos++;
        kdDebug(7111) << "position: " << pos << endl; 
        if (!(pos % 100)) // HACK! should be dependent on the table size?
            processedSize(pos);
        
    } while (ptr = itr++);
    
    str = makeIndent(--indent) + "</KugarData>\n";
    content.duplicate(str.utf8(), str.length());
    data(content);

    data(QByteArray());

    kdDebug( 7111 ) << "just before finished()" << endl;
    finished();
    kdDebug( 7111 ) << "=============== FINISHED ===============" << endl;

    KDB::Connection *conn = static_cast<KDB::Connection *>(base->parent()); 
    base->close();
    conn->close();
}



void
KDBProtocol::stat( const KURL& url )
{
    kdDebug(7111) << k_funcinfo << url.prettyURL() << endl;

    UDSEntry entry;

    createUDSEntry(url.directory(), url.fileName(), entry);

    statEntry( entry );
    finished();
}


void
KDBProtocol::listDir( const KURL& url )
{
    kdDebug(7111) << k_funcinfo << url.url() << endl;
    QString host = url.host();
    int port = url.port();
    QString path = url.path();
    QString user = url.user();
    QString pwd;
    QString plugin;
    QString database;
    QString extra;

    // the trailing slash is required, otherwise we will have problems with
    // autentication
    if (path.isEmpty()) {
        QString redir = QString("sql://%1").arg(host);
        if (port) {
            redir += QString(":%1").arg(port);
        }
        redir += "/";
        redirection(redir);
        finished();
        return;
    }

    //build the key for password caching
    QStringList l = QStringList::split("/", path);
    if (l.count() > 0)
        plugin = l[0]; // the first part of the path is the plugin

    if (l.count() > 1)
        database = l[1]; //the second is the database

    kdDebug( 7111 ) << "=============== LIST " << path << " ===============" << endl;

    int type = fileType(path);

    switch (type) {
    case NO_TYPE: // root entry
        {
            QStringList lst = DBENGINE->pluginNames();
            UDSEntry entry;
            for (QStringList::Iterator it = lst.begin(); it != lst.end(); it++) {
                createUDSEntry("/", (*it), entry);
                listEntry(entry, false);
            }
            listEntry(entry, true);
            break;
        }
    case CONNECTION: // list all databases
        {
            // get user & password:
            KDB::Connection *conn = 0L;
            KURL checkUrl(QString("sql://%1:%2").arg(host).arg(port));

            infoMessage(i18n("Connecting to %1").arg(checkUrl.prettyURL()));

            KIO::AuthInfo info;
            info.url = checkUrl;
            info.username = user;
            info.password = pwd;
            info.realmValue = checkUrl.url() + "/" + plugin + "/" + user;
            info.verifyPath = false;
    
            if (!checkCachedAuthentication( info ) ) { // check failed, ask for user/pwd

                info.prompt = i18n("Please enter username and password for %1").arg(host);
                // try until you get a valid password or a cancel
                while (true) {
                    
                    if ( !openPassDlg(info) ) {
                        error(ERR_ACCESS_DENIED,path);
                        return;
                    }
                    
                    conn = DBENGINE->openConnection( plugin, host, port, info.username, info.password);
                    if (conn) {
                        // ok, got the rigth user/pwd, store them
                        cacheAuthentication(info);
                        break;
                    }
                }
            } else {
                conn = DBENGINE->openConnection( plugin, host, port, info.username, info.password);
                if (!conn) {
                    error(ERR_ACCESS_DENIED,path);
                    return;
                }
            }

            KDB::DatabaseIterator it = conn->begin();
            UDSEntry entry;
            while (it.current()) {
                createUDSEntry(path, it.current()->name() ,entry);
                listEntry(entry, false);
                ++it;
            }
            listEntry(entry, true);
            conn->close();
            
            break;
        }
    case DATABASE: //list all queries and tables
        {
            KDB::Database * base = 0L;
            KURL checkUrl(QString("sql://%1:%2").arg(host).arg(port));
            
            kdDebug(7111) << "check auth cache for URL: " << checkUrl.prettyURL() << endl;

            KIO::AuthInfo info;
            info.url = checkUrl;
            info.username = user;
            info.password = pwd;
            info.realmValue = checkUrl.url() + "/" + plugin + "/" + user;
            info.verifyPath = false;
            
            if (!checkCachedAuthentication( info ) ) { // check failed, ask for user/pwd
                
                info.prompt = i18n("Please enter username and password for %1").arg(host);
                // try until you get a valid password or a cancel
                while (true) {
                    
                    if ( !openPassDlg(info) ) {
                        error(ERR_ACCESS_DENIED,path);
                        return;
                    }
                    
                    base = DBENGINE->openDatabase( plugin, host, port, info.username, info.password, database);
                    if (base) {
                        // ok, got the rigth user/pwd, store them
                        cacheAuthentication(info);
                        break;
                    }
                }
            } else {
                base = DBENGINE->openDatabase( plugin, host, port, info.username, info.password, database);
                if (!base) {
                    error(ERR_ACCESS_DENIED,path);
                    return;
                }
            }
            
            QStringList tabs = base->tableNames();
            UDSEntry entry;
            kdDebug(7111) << "Listing tables:" << endl;
            for (QStringList::Iterator it2 = tabs.begin(); it2 != tabs.end(); ++it2) {
                createUDSEntry(TABLE, *it2 ,entry);
                listEntry(entry, false);
            }
            QStringList qry = base->queryNames();
            kdDebug(7111) << "Listing queries:" << endl;
            for (QStringList::Iterator it3 = qry.begin(); it3 != qry.end(); ++it3) {
                createUDSEntry(QUERY, *it3 ,entry);
                listEntry(entry, false);
                ++it3;
            }
            kdDebug(7111) << "EOL" << endl;
            listEntry(entry, true);
            KDB::Connection * conn = static_cast<KDB::Connection *>(base->parent());
            base->close();
            conn->close(); // will cause crashes otherwise
            break;
        }
    case TABLE:
    case QUERY:
        error( KIO::ERR_IS_FILE, path );
        return;
        break;
    default:
        break;
    }

    
    finished();
    kdDebug( 7111 ) << "=============== FINISHED ===============" << endl;
}

void
KDBProtocol::createUDSEntry(QString path, QString filename, UDSEntry &entry)
{
    
    QString full = path + "/" + filename;
    int type = fileType(full);

    kdDebug(7111) << "createUDSEntry:" << endl
                  << "\tPath: " << path << endl;

    createUDSEntry(type, filename, entry);

}


void
KDBProtocol::createUDSEntry(int type, QString filename, UDSEntry &entry)
{
    kdDebug(7111) << "\tFile name: " << filename << endl
                  << "\tFile type: " << type << endl;
    UDSAtom atom;

    atom.m_uds = KIO::UDS_NAME;
    if (filename.isEmpty())
        atom.m_str = "/";
    else
        atom.m_str = filename;
    entry.append( atom );

    atom.m_uds = KIO::UDS_FILE_TYPE;
    switch (type) {
        case NO_TYPE:
        case CONNECTION:
        case DATABASE:
            atom.m_long = S_IFDIR;
            break;
        case TABLE:
        case QUERY:
            atom.m_long = S_IFREG;
            break;
        default:
            break;
    }
    entry.append( atom );

/*
    atom.m_uds = KIO::UDS_SIZE;
    atom.m_long = 0;
    entry.append( atom );

*/

    atom.m_uds = KIO::UDS_MIME_TYPE;
    switch (type) {
        case NO_TYPE:
        case CONNECTION:
        case DATABASE:
            atom.m_str = "inode/directory";
            break;
        case TABLE:
            atom.m_str = MIMETYPE;
            break;
        case QUERY:
            atom.m_str = MIMETYPE;
            break;
        default:
            break;
    }
    entry.append( atom );

    atom.m_uds = UDS_ACCESS;
    atom.m_long = S_IRWXU | S_IRWXG | S_IRWXO;
    entry.append( atom );
}

void
KDBProtocol::del( const KURL& url, bool isfile)
{
    kdDebug(7111) << k_funcinfo << url.prettyURL() << endl;
}



int
KDBProtocol::fileType(QString filename)
{
    QStringList l = QStringList::split("/", filename);

    int ret = NO_TYPE;
    switch (l.count()) {
        case 0: // / entry
            ret = NO_TYPE;
            break;
        case 1: // Plugin
            ret = CONNECTION;
            break;
        case 2: // Database
            ret = DATABASE;
            break;
        case 3: // Table or query??
            ret = TABLE; // test for existence of table in given database
        default:
            break;
    }

    return ret;

}


