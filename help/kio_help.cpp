// $Id$

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <qvaluelist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kshred.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <qfile.h>
#include <limits.h>

#include "kio_help.h"
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <xslt.h>

using namespace KIO;

QString HelpProtocol::langLookup(QString fname)
{
    QStringList search;

    // assemble the local search paths
    const QStringList localDoc = KGlobal::dirs()->resourceDirs("html");

    // look up the different languages
    for (int id=localDoc.count()-1; id >= 0; --id)
    {
        QStringList langs = KGlobal::locale()->languageList();
        langs.append("default");
        langs.append("en");
        QStringList::ConstIterator lang;
        for (lang = langs.begin(); lang != langs.end(); ++lang)
            search.append(QString("%1%2/%3").arg(localDoc[id]).arg(*lang).arg(fname));
    }

    // try to locate the file
    QStringList::Iterator it;
    for (it = search.begin(); it != search.end(); ++it)
    {
        kdDebug() << "Looking for help in: " << *it << endl;

        QFileInfo info(*it);
        if (info.exists() && info.isFile() && info.isReadable())
            return *it;

        QString file = (*it).left((*it).findRev('/')) + "/index.docbook";
        kdDebug() << "Looking for help in: " << file << endl;
        info.setFile(file);
        if (info.exists() && info.isFile() && info.isReadable())
            return *it;
    }

    return QString::null;
}


QString HelpProtocol::lookupFile(QString fname, QString query, bool &redirect)
{
    redirect = false;

    QString anchor, path, result;

    // if we have a query, look if it contains an anchor
    if (!query.isEmpty())
        if (query.left(8) == "?anchor=")
            anchor = query.mid(8).upper();

    path = fname;

    kdDebug() << "lookupFile: path=" << path << " anchor=" << anchor << endl;

    if (!anchor.isEmpty())
    {
        // try to locate .anchors file
        result = langLookup(path + "/.anchors");
        if (!result.isEmpty())
	{
            // parse anchors file to get our real page
            QFile anch(result);
            if (anch.open(IO_ReadOnly))
	    {
                QTextStream ts(&anch);

                QString line;
                QStringList items;
                while (!ts.atEnd())
		{
                    line = ts.readLine();

                    if (line.left(6) == "anchor")
		    {
                        items = QStringList::split(' ', line);
                        if (items[1] == anchor)
			{
                            redirection(KURL(QString("help:%1/%2").arg(path).arg(items[2])));
                            redirect = true;
                            return QString::null;
			}
		    }
		}

                anch.close();
	    }
	}
    }

    result = langLookup(path);
    if (result.isEmpty())
    {
        result = langLookup(path+"/index.html");
        if (!result.isEmpty())
	{
            redirection(KURL(QString("help:%1/index.html").arg(path)));
            redirect = true;
	}
        else
	{
            result = langLookup("khelpcenter/index.html");
            if (!result.isEmpty())
	    {
                redirection(KURL("help:/khelpcenter/index.html"));
                redirect = true;
                return QString::null;
	    }

            // Note: This page should explain how to install
            // docbook!
            result = langLookup("khelpcenter/no-html.html");
            if (!result.isEmpty())
                return result;

            notFound();
            return QString::null;
	}
    }

    return result;
}


void HelpProtocol::notFound()
{
    data(QCString(i18n("<html>The requested help file could not be found. Check if "
                       "you have installed the documentation.</html>").local8Bit()));
    finished();
}

HelpProtocol *slave = 0;

HelpProtocol::HelpProtocol( const QCString &pool, const QCString &app )
  : SlaveBase( "help", pool, app )
{
    slave = this;
}

void HelpProtocol::get( const KURL& url )
{
    kdDebug() << "get: path=" << url.path() << " query=" << url.query() << endl;

    bool redirect;
    QString doc;
    doc = url.path();
    if (doc.at(doc.length() - 1) == '/')
        doc += "index.html";

    infoMessage(i18n("Looking up correct file"));

    doc = lookupFile(doc, url.query(), redirect);

    if (redirect)
    {
        finished();
        return;
    }

    if (doc.isEmpty())
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.url() );
        return;
    }

    KURL target;
    target.setPath(doc);
    if (url.hasHTMLRef())
        target.setHTMLRef(url.htmlRef());

    QString file = target.path();
    if (!KStandardDirs::exists(file) || file.right(4) == "html") {
        file = file.left(file.findRev('/')) + "/index.docbook";
    } else {
        QFileInfo fi(file);
        if (fi.isDir()) {
            file = file + "/index.docbook";
        } else {
            kdDebug() << "emitFile redirection " << url.url().latin1() << endl;
            redirection(target);
            finished();
            return;
        }
    }

    infoMessage(i18n("Preparing document"));

    parsed = transform(file, locate("dtd", "customization/kde-chunk.xsl"));
    if (parsed.isEmpty()) {
        data(QCString(i18n("<html>The requested help file could not be parsed:<br>%1</html>").arg(file).latin1()));
    } else
        emitFile( target );

    finished();
}

void HelpProtocol::emitFile( const KURL& url )
{
    infoMessage(i18n("Looking up section"));

    QString filename = url.path().mid(url.path().findRev('/') + 1);

    int index = parsed.find(QString("<FILENAME filename=\"%1\"").arg(filename));
    if (index == -1) {
        if ( filename == "index.html" ) {
            data( parsed.local8Bit() );
            return;
        }

        data(QCString(i18n("<html>Couldn't find filename %1 in %2</html>").arg(filename).arg(url.url()).local8Bit()));
        return;
    }

    QString filedata = splitOut(parsed, index);
    data(filedata.local8Bit());
}

void HelpProtocol::mimetype( const KURL &)
{
    mimeType("text/html");
    finished();
}
