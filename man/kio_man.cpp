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
        }
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
    : QObject(), SlaveBase("man", pool_socket, app_socket), m_unzippedData(0)
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
    QStringList list = KGlobal::dirs()->findAllResources("manpath", QString("man*/%1.*").arg(title));
    return list[0];
}

void MANProtocol::output(const char *insert)
{
    if (insert)
        output_string += insert;
    if (!insert || output_string.length() > 2000) {
        data(output_string);
        output_string.truncate(0);
    }
}

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

    // see if an index was requested
    if (url.query().isEmpty() && (title.isEmpty() || title == "/"))
    {
        if (section == "index" || section.isEmpty())
            showMainIndex();
        else
            showIndex(section);
        return;
    }

    // tell we are getting the file
    gettingFile(url.url());
    mimeType("text/html");

    QCString filename=QFile::encodeName(findPage(section, title));

    gzFile file = gzopen(filename, "rb");
    if (!file) {
        outputError(i18n("open of %1 failed").arg(filename));
        return;
    }

    delete [] m_unzippedData;
    m_unzippedBufferSize=10;
    m_unzippedData=new char[m_unzippedBufferSize];
    m_unzippedLength = 0;

    char buffer[1024];
    while (!gzeof(file)) {
        int read = gzread(file, buffer, 1024);
        addToBuffer(buffer, read);
    }

    // will call output_real
    scan_man_page(m_unzippedData);

    output(0); // flush

    // tell we are done
    data(QByteArray());
    finished();

    delete [] m_unzippedData;
    m_unzippedData = 0;
}

void MANProtocol::addToBuffer(const char *buffer, int buflen)
{
    //check if the buffer is large enough for the new data
    while (m_unzippedLength+buflen+3>m_unzippedBufferSize)
    {
        //hmm, lets make it bigger
        char *newBuf=new char[m_unzippedBufferSize*3/2+1];
        memcpy(newBuf,m_unzippedData,m_unzippedLength);
        m_unzippedBufferSize=m_unzippedBufferSize*3/2;
        delete [] m_unzippedData;
        m_unzippedData=newBuf;
        m_unzippedData[m_unzippedBufferSize]='\0';
    }
    memcpy(m_unzippedData+m_unzippedLength, buffer, buflen);
    m_unzippedLength+=buflen;
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

  int kdemain( int argc, char **argv )
  {
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
        kdDebug(7107) << "adding manpath " << manpaths[index] << endl;
        KGlobal::dirs()->addResourceDir("manpath", manpaths[index++]);
    }

    manPaths = KGlobal::dirs()->resourceDirs("manpath");
    QStringList::ConstIterator page;
    for (page = manPaths.begin(); page != manPaths.end(); ++page) {
        kdDebug(7107) << *page << endl;
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

    // search for the man pages
    QStringList pages = KGlobal::dirs()->findAllResources("manpath",
                                                          QString("*man%1*/*").arg(section), true);

    // print out the list
    os << "<table>" << endl;
    pages.sort();

    QStringList::ConstIterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
        kdDebug(7107) << "page: " << *page << endl;
        os << "<tr><td>" << *page;
        os << "</td><td>" << endl;
    }

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    data(output);
    finished();
}


