#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdict.h>
#include <qcstring.h>


#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kprocess.h>
#include <klocale.h>


#include "kio_man.h"
#include "kio_man.moc"
#include <zlib.h>
#include <man2html.h>
#include <assert.h>
#include <kfilterbase.h>
#include <kfilterdev.h>

using namespace KIO;

MANProtocol *MANProtocol::_self = 0;

bool parseUrl(const QString& _url, QString &title, QString &section)
{
    section = "";

    QString url = _url;
    if (url.at(0) == '/') {
        if (KStandardDirs::exists(url)) {
            title = url;
            return true;
        } else
            kdDebug(7107) << url << " does not exist" << endl;
    }

    while (url.at(0) == '/')
        url.remove(0,1);

    title = url;

    int pos = url.find('(');
    if (pos < 0)
        return true;

    title = title.left(pos);

    section = url.mid(pos+1);
    section = section.left(section.length()-1);

    return true;
}


MANProtocol::MANProtocol(const QCString &pool_socket, const QCString &app_socket)
    : QObject(), SlaveBase("man", pool_socket, app_socket)
{
    assert(!_self);
    _self = this;
}

MANProtocol *MANProtocol::self() { return _self; }

MANProtocol::~MANProtocol()
{
    _self = 0;
}

QString MANProtocol::findPage(const QString &section, const QString &title)
{
    checkManPaths();
    if (title.at(0) == '/')
        return title;
    QStringList list = KGlobal::dirs()->findAllResources("manpath", QString("man*/%1.*").arg(title));
    return list[0];
}

void MANProtocol::output(const char *insert)
{
    if (insert)
        output_string += insert;
    if (!insert || output_string.length() > 2000) {
        kdDebug(7107) << "output " << output_string << endl;
        data(output_string);
        output_string.truncate(0);
    }
}

// called by man2html
char *read_man_page(const char *filename)
{
    return MANProtocol::self()->readManPage(filename);
}

// called by man2html
void output_real(const char *insert)
{
    MANProtocol::self()->output(insert);
}

void MANProtocol::get(const KURL& url )
{
    kdDebug(7107) << "GET " << url.url() << endl;

    QString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        showMainIndex();
        return;
    }



    // tell we are getting the file
    gettingFile(url.url());
    mimeType("text/html");

    QCString filename=QFile::encodeName(findPage(section, title));

    char *buf = readManPage(filename);
    if (!buf) {
        outputError(i18n("open of %1 failed").arg(title));
        return;
    }
    // will call output_real
    scan_man_page(buf);
    delete [] buf;

    output(0); // flush

    // tell we are done
    data(QByteArray());
    finished();
}

char *MANProtocol::readManPage(const char *_filename)
{
    QCString filename = _filename;

    if (QDir::isRelativePath(filename)) {
        kdDebug(7107) << "relative " << filename << endl;
        filename = QDir::cleanDirPath(lastdir + "/" + filename).utf8();
        if (!KStandardDirs::exists(filename)) { // exists perhaps with suffix
            lastdir = filename.left(filename.findRev('/'));
            QDir mandir(lastdir);
            mandir.setNameFilter(filename.mid(filename.findRev('/') + 1) + ".*");
            filename = lastdir + "/" + QFile::encodeName(mandir.entryList().first());
        }
        kdDebug(7107) << "resolved to " << filename << endl;
    }
    lastdir = filename.left(filename.findRev('/'));

    QFile raw(filename);
    KFilterBase *f = KFilterBase::findFilterByFileName(filename);
    if (!f)
        return 0;

    f->setDevice(&raw);

    KFilterDev *fd = new KFilterDev(f);
    if (!fd->open(IO_ReadOnly)) {
        delete fd;
        return 0;
    }
    char buffer[1025];
    int n;
    QCString text;
    while ( ( n = fd->readBlock(buffer, 1024) ) ) {
        buffer[n] = 0;
        text += buffer;
    }
    kdDebug(7107) << "read " << text.length() << endl;
    fd->close();
    delete fd;
    delete f;

    int l = text.length();
    char *buf = new char[l + 4];
    memcpy(buf + 1, text.data(), l);
    buf[0]=buf[l]='\n';
    buf[l+1]=buf[l+2]='\0';

    return buf;
}

void MANProtocol::outputError(const QString& errmsg)
{
    QCString output;

    QTextStream os(output, IO_WriteOnly);

    os << "<html>" << endl;
    os << i18n("<head><title>Man output</title></head>") << endl;
    os << i18n("<body bgcolor=#ffffff><h1>KDE Man Viewer Error</h1>") << errmsg << "</body>" << endl;
    os << "</html>" << endl;

    data(output);
    finished();
}

void MANProtocol::stat( const KURL& url)
{
    kdDebug(7107) << "ENTERING STAT " << url.url();

    QString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        error(KIO::ERR_MALFORMED_URL, url.url());
        return;
    }

    kdDebug(7107) << "URL " << url.url() << " parsed to title='" << title << "' section=" << section << endl;


    UDSEntry entry;
    UDSAtom atom;

    atom.m_uds = UDS_NAME;
    atom.m_long = 0;
    atom.m_str = title;
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append(atom);

    atom.m_uds = UDS_URL;
    atom.m_long = 0;
    QString newUrl = "man:"+title;
    if (!section.isEmpty())
        newUrl += QString("(%1)").arg(section);
    atom.m_str = newUrl;
    entry.append(atom);

    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/html";
    entry.append(atom);

    statEntry(entry);

    finished();
}


extern "C"
{

    int kdemain( int argc, char **argv ) {

        KLocale::setMainCatalogue("kdelibs");
        KInstance instance("kio_man");

        kdDebug(7107) <<  "STARTING " << getpid() << endl;

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_man protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        MANProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kdDebug(7107) << "Done" << endl;

        return 0;
    }

}

void MANProtocol::mimetype(const KURL & /*url*/)
{
    mimeType("text/html");
    finished();
}

QString sectionName(const QString& section)
{
    if (section == "1")
        return i18n("User Commands");
    else if (section == "2")
        return i18n("System Calls");
    else if (section == "3")
        return i18n("Subroutines");
    else if (section == "4")
        return i18n("Devices");
    else if (section == "5")
        return i18n("File Formats");
    else if (section == "6")
        return i18n("Games");
    else if (section == "7")
        return i18n("Miscellaneous");
    else if (section == "8")
        return i18n("System Administration");
    else if (section == "9")
        return i18n("Kernel");
    else if (section == "n")
        return i18n("New");

    return QString::null;
}


void MANProtocol::showMainIndex()
{
    QCString output;

    QTextStream os(output, IO_WriteOnly);

    // print header
    os << "<html>" << endl;
    os << i18n("<head><title>UNIX Manual Index</title></head>") << endl;
    os << i18n("<body bgcolor=#ffffff><h1>UNIX Manual Index</h1>") << endl;

    QString sectList = getenv("MANSECT");
    if (sectList.isEmpty())
        sectList = "1:2:3:4:5:6:7:8:9:n";
    QStringList sections = QStringList::split(':', sectList);

    os << "<table>" << endl;

    QStringList::ConstIterator it;
    for (it = sections.begin(); it != sections.end(); ++it)
        os << "<tr><td><a href=\"man:(" << *it << ")\">Section " << *it << "</a></td><td>&nbsp;</td><td> " << sectionName(*it) << "</td></tr>" << endl;

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    data(output);
    finished();
}


void MANProtocol::checkManPaths()
{
    static bool inited = false;

    if (inited)
        return;

    inited = true;

    QStringList manPaths;

    // add paths from /etc/man.conf
    QRegExp manpath("^MANPATH\\s");
    QFile mc("/etc/man.conf");
    if (mc.open(IO_ReadOnly))
    {
        QTextStream is(&mc);

        while (!is.eof())
	{
            QString line = is.readLine();
            if (manpath.find(line, 0) == 0)
	    {
                QString path = line.mid(8).stripWhiteSpace();
                KGlobal::dirs()->addResourceDir("manpath", path);
	    }
	}

        mc.close();
    }

    static const char *manpaths[] = {
                    "/usr/X11/man/",
                    "/usr/X11R6/man/",
                    "/usr/man/",
                    "/usr/local/man/",
                    "/usr/exp/man/",
                    "/usr/openwin/man/",
                    "/usr/tex/man/",
                    "/usr/www/man/",
                    "/usr/lang/man/",
                    "/usr/gnu/man/",
                    "/usr/share/man",
                    "/usr/motif/man/",
                    "/usr/titools/man/",
                    "/usr/sunpc/man/",
                    "/usr/ncd/man/",
                    "/usr/newsprint/man/",
                    NULL };

    int index = 0;
    while (manpaths[index]) {
        KGlobal::dirs()->addResourceDir("manpath", manpaths[index++]);
    }

    // add MANPATH paths
    QString envPath = getenv("MANPATH");
    if (!envPath.isEmpty()) {
        manPaths = QStringList::split(':', envPath);
        for (QStringList::ConstIterator it = manPaths.begin();
             it != manPaths.end(); ++it)
            KGlobal::dirs()->addResourceDir("manpath", *it);
    }

}

void MANProtocol::showIndex(const QString& section)
{
    QCString output;

    QTextStream os(output, IO_WriteOnly);

    // print header
    os << "<html>" << endl;
    os << i18n("<head><title>UNIX Manual Index</title></head>") << endl;
    os << i18n("<body bgcolor=#ffffff><h1>Index for Section %1: %2</h1>").arg(section).arg(sectionName(section)) << endl;

    // compose list of search paths -------------------------------------------------------------

    checkManPaths();
    infoMessage(i18n("Generating Index"));

    // search for the man pages
    QStringList pages = KGlobal::dirs()->
                        findAllResources("manpath",
                                         QString("man%1/*").arg(section), true);

    pages += KGlobal::dirs()->
             findAllResources("manpath",
                              QString("sman%1/*").arg(section), true);

    // print out the list
    os << "<table>" << endl;
    pages.sort();

    QStringList::ConstIterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
        QString fileName = *page;

        // skip compress extension
        if (fileName.right(4) == ".bz2")
        {
            fileName.truncate(fileName.length()-4);
        }
        else if (fileName.right(3) == ".gz")
        {
            fileName.truncate(fileName.length()-3);
        }
        else if (fileName.right(2) == ".Z")
        {
            fileName.truncate(fileName.length()-2);
        }

        // strip section
        int pos = fileName.findRev('.');
        if ((pos > 0) && (fileName.mid(pos).find(section) > 0))
            fileName = fileName.left(pos);

        pos = fileName.findRev('/');
        if (pos > 0)
            fileName = fileName.mid(pos+1);

        if (!fileName.isEmpty()) {
            os << "<tr><td><a href=\"man:" << *page << "\">\n"
               << fileName << "</a></td><td>&nbsp;</td><td> "
               << "no idea yet" << "</td></tr>"  << endl;
        }
    }

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    infoMessage(QString::null);
    data(output);
    finished();
}


