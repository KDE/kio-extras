/*  This file is part of the KDE libraries
    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>

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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qcstring.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <klocale.h>
#include <kmimetype.h>

#include "kio_man.h"
#include "kio_man.moc"
#include <zlib.h>
#include "man2html.h"
#include <assert.h>
#include <kfilterbase.h>
#include <kfilterdev.h>

using namespace KIO;

MANProtocol *MANProtocol::_self = 0;

#define SGML2ROFF_DIRS "/usr/lib/sgml"

/*
 * Drop trailing ".section[.gz]" from name
 */
static
void stripExtension( QString *name )
{
    int pos = name->length();

    if ( name->find(".gz", -3) != -1 )
        pos -= 3;
    else if ( name->find(".z", -2, false) != -1 )
        pos -= 2;
    else if ( name->find(".bz2", -4) != -1 )
        pos -= 4;
    else if ( name->find(".bz", -3) != -1 )
        pos -= 3;

    if ( pos > 0 )
        pos = name->findRev('.', pos-1);

    if ( pos > 0 )
        name->truncate( pos );
}

static
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
    common_dir = KGlobal::dirs()->findResourceDir( "html", "en/common/kde-common.css" );
    section_names << "1" << "2" << "3" << "3n" << "3p" << "4" << "5" << "6" << "7"
    << "8" << "9" << "l" << "n";
}

MANProtocol *MANProtocol::self() { return _self; }

MANProtocol::~MANProtocol()
{
    _self = 0;
}

void MANProtocol::parseWhatIs( QMap<QString, QString> &i, QTextStream &t, const QString &mark )
{
    QRegExp re( mark );
    QString l;
    while ( !t.atEnd() )
    {
	l = t.readLine();
	int pos = re.search( l );
	if (pos != -1)
	{
	    QString names = l.left(pos);
	    QString descr = l.mid(pos + re.matchedLength());
	    while ((pos = names.find(",")) != -1)
	    {
		i[names.left(pos++)] = descr;
		while (names[pos] == ' ')
		    pos++;
		names = names.mid(pos);
	    }
	    i[names] = descr;
	}
    }
}

bool MANProtocol::addWhatIs(QMap<QString, QString> &i, const QString &name, const QString &mark)
{
    QFile f(name);
    if (!f.open(IO_ReadOnly))
        return false;
    QTextStream t(&f);
    parseWhatIs( i, t, mark );
    return true;
}

QMap<QString, QString> MANProtocol::buildIndexMap(const QString &section)
{
    QMap<QString, QString> i;
    QStringList man_dirs = manDirectories();
    // Supplementary places for whatis databases
    man_dirs << "/var/cache/man" << "/var/catman";
    QStringList names;
    names << "whatis.db" << "whatis";
    QString mark = "\\s+\\(" + section + "[a-z]*\\)\\s+-\\s+";

    for ( QStringList::ConstIterator it_dir = man_dirs.begin();
          it_dir != man_dirs.end();
          it_dir++ )
    {
        if ( QFile::exists( *it_dir ) ) {
    	    QStringList::ConstIterator it_name;
            for ( it_name = names.begin();
	          it_name != names.end();
	          it_name++ )
            {
	        if (addWhatIs(i, (*it_dir) + "/" + (*it_name), mark))
		    break;
	    }
            if ( it_name == names.end() ) {
                KProcess proc;
                proc << "whatis" << "-M" << (*it_dir) << "-w" << "*";
                myStdStream = QString::null;
                connect( &proc, SIGNAL( receivedStdout(KProcess *, char *, int ) ),
                         SLOT( slotGetStdOutput( KProcess *, char *, int ) ) );
                proc.start( KProcess::Block, KProcess::Stdout );
                QTextStream t( &myStdStream, IO_ReadOnly );
                parseWhatIs( i, t, mark );
            }
        }
    }
    return i;
}

QStringList MANProtocol::manDirectories()
{
    checkManPaths();
    //
    // Build a list of man directories including translations
    //
    QStringList man_dirs;

    for ( QStringList::ConstIterator it_dir = m_manpath.begin();
          it_dir != m_manpath.end();
          it_dir++ )
    {
        // Translated pages in "<mandir>/<lang>" if the directory
        // exists
        QStringList languages = KGlobal::locale()->languageList();

        for (QStringList::ConstIterator it_lang = languages.begin();
             it_lang != languages.end();
             it_lang++ )
        {
            if ( !(*it_lang).isEmpty() && (*it_lang) != QString("C") ) {
                QString dir = (*it_dir) + '/' + (*it_lang);

                struct stat sbuf;

                if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    man_dirs += dir;
                }
            }
        }

        // Untranslated pages in "<mandir>"
        man_dirs += (*it_dir);
    }
    return man_dirs;
}

QStringList MANProtocol::findPages(const QString &section,
                                   const QString &title,
                                   bool full_path)
{
    QStringList list;

    if (title.at(0) == '/') {
       list.append(title);
       return list;
    }

    QStringList man_dirs = manDirectories();
    //
    // Find pages
    //
    for ( QStringList::ConstIterator it_dir = man_dirs.begin();
          it_dir != man_dirs.end();
          it_dir++ )
    {
        QString man_dir = (*it_dir);

        //
        // Find man sections in this directory
        //
        QStringList sect_list;

        if ( !section.isEmpty() && section != QString("*") ) {
            //
            // Section given as argument
            //
            sect_list += section;
        }
        else {
            //
            // Sections = all sub directories "man*" and "sman*"
            //
            DIR *dp = ::opendir( QFile::encodeName( man_dir ) );

            if ( !dp )
                continue;

            struct dirent *ep;

            QString man = QString("man");
            QString sman = QString("sman");

            while ( (ep = ::readdir( dp )) != 0L ) {
                QString file = QFile::decodeName( ep->d_name );
		QString sect = QString::null;

                if ( file.startsWith( man ) )
		  sect = file.mid(3);
		else if (file.startsWith(sman))
		  sect = file.mid(4);

		// Only add sect if not already contained, avoid duplicates
		if (!sect_list.contains(sect))
		  sect_list += sect;
            }

            ::closedir( dp );
        }

        //
        // Find man pages in the sections listed above
        //
        for ( QStringList::ConstIterator it_sect = sect_list.begin();
              it_sect != sect_list.end();
              it_sect++ )
        {
            QString dir = man_dir + QString("/man") + (*it_sect) + '/';
            QString sdir = man_dir + QString("/sman") + (*it_sect) + '/';

	    findManPagesInSection(dir, title, full_path, list);
	    findManPagesInSection(sdir, title, full_path, list);
        }
    }

    return list;
}

void MANProtocol::findManPagesInSection(const QString &dir, const QString &title, bool full_path, QStringList &list)
{
            bool title_given = !title.isEmpty();

            DIR *dp = ::opendir( QFile::encodeName( dir ) );

            if ( !dp )
    return;

            struct dirent *ep;

            while ( (ep = ::readdir( dp )) != 0L ) {
                if ( ep->d_name[0] != '.' ) {

                    QString name = QFile::decodeName( ep->d_name );

                    // check title if we're looking for a specific page
                    if ( title_given ) {
                        if ( !name.startsWith( title ) ) {
                            continue;
                        }
                        else {
                            // beginning matches, do a more thorough check...
                            QString tmp_name = name;
                            stripExtension( &tmp_name );
                            if ( tmp_name != title )
                                continue;
                        }
                    }

                    if ( full_path )
                        name.prepend( dir );

                    list += name ;
                }
            }
            ::closedir( dp );
}

void MANProtocol::output(const char *insert)
{
    if (insert)
        output_string += insert;
    if (!insert || output_string.length() > 2000) {
        // TODO find out the language of the man page and put the right common dir in
        output_string.replace( "KDE_COMMON_DIR", QString( "file:%1/en/common" ).arg( common_dir ).local8Bit());
        //kdDebug(7107) << "output " << output_string << endl;
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

    // see if an index was requested
    if (url.query().isEmpty() && (title.isEmpty() || title == "/" || title == "."))
    {
        if (section == "index" || section.isEmpty())
            showMainIndex();
        else
            showIndex(section);
        return;
    }

    // tell the mimetype
    mimeType("text/html");

    QStringList foundPages=findPages(section, title);
    if (foundPages.count()==0)
    {
       outputError(i18n("No man page matching to %1 found. You can extend the search path by setting the environment variable MANPATH before starting KDE.").arg(title));
    }
    else if (foundPages.count()>1)
    {
       outputMatchingPages(foundPages);
    }
    //yes, we found exactly one man page
    else
    {
       QCString filename=QFile::encodeName(foundPages[0]);
       char *buf = readManPage(filename);

       if (!buf)
       {
          outputError(i18n("Open of %1 failed.").arg(title));
          finished();
          return;
       }
       // will call output_real
       scan_man_page(buf);
       delete [] buf;

       output(0); // flush

       // tell we are done
       data(QByteArray());
    };
    finished();
}

void MANProtocol::slotGetStdOutput(KProcess* /* p */, char *s, int len)
{
  myStdStream += QString::fromLocal8Bit(s, len);
}

char *MANProtocol::readManPage(const char *_filename)
{
    QCString filename = _filename;

    char *buf = NULL;

    /* Determine type of man page file by checking its path. Determination by
     * MIME type with KMimeType doesn't work reliablely. E.g., Solaris 7:
     * /usr/man/sman7fs/pcfs.7fs -> text/x-csrc : WRONG
     * If the path name constains the string sman, assume that it's SGML and
     * convert it to roff format (used on Solaris). */
    //QString file_mimetype = KMimeType::findByPath(QString(filename), 0, false)->name();
    if (filename.contains("sman", false)) //file_mimetype == "text/html" || )
      {
	myStdStream = "";
	KProcess proc;

	/* Determine path to sgml2roff, if not already done. */
	getProgramPath();
	proc << mySgml2RoffPath << filename;

	QApplication::connect(&proc, SIGNAL(receivedStdout (KProcess *, char *, int)),
			      this, SLOT(slotGetStdOutput(KProcess *, char *, int)));
	proc.start(KProcess::Block, KProcess::All);

	buf = (char*)myStdStream.latin1();
	// Does not work (return string is empty): buf = QCString(myStdStream->local8Bit());
      }
    else
      {
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

    QIODevice *fd= KFilterDev::deviceForFile(filename);

    if (!fd->open(IO_ReadOnly))
    {
       delete fd;
       return 0;
    }
    char buffer[1025];
    int n;
    QCString text;
    while ( ( n = fd->readBlock(buffer, 1024) ) )
    {
        buffer[n] = 0;
        text += buffer;
    }
    kdDebug(7107) << "read " << text.length() << endl;
    fd->close();

    delete fd;

    int l = text.length();
	buf = new char[l + 4];
    memcpy(buf + 1, text.data(), l);
    buf[0]=buf[l]='\n';
    buf[l+1]=buf[l+2]='\0';
      }

    return buf;
}


void MANProtocol::outputError(const QString& errmsg)
{
    QString output;

    QTextStream os(&output, IO_WriteOnly);

    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("Man output") << "</title></head>" << endl;
    os << i18n("<body bgcolor=#ffffff><h1>KDE Man Viewer Error</h1>") << errmsg << "</body>" << endl;
    os << "</html>" << endl;

    data(output.utf8());
}

void MANProtocol::outputMatchingPages(const QStringList &matchingPages)
{
    QString output;

    QTextStream os(&output, IO_WriteOnly);

    os << "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";
    os << "<title>" << i18n("Man output");
    os <<"</title></head>\n<body bgcolor=#ffffff><h1>";
    os << i18n("There is more than one matching man page.");
    os << "</h1>\n<ul>";
    for (QStringList::ConstIterator it = matchingPages.begin(); it != matchingPages.end(); ++it)
       os<<"<li><a href=man:"<<QFile::encodeName(*it)<<">"<< *it <<"</a><br>\n<br>\n";
    os<< "</ul>\n</body>\n</html>"<<endl;

    data(output.utf8());
    finished();
}

void MANProtocol::stat( const KURL& url)
{
    kdDebug(7107) << "ENTERING STAT " << url.url() << endl;

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
    else if (section == "3p")
    	return i18n("Perl Modules");
    else if (section == "3n")
    	return i18n("Network Functions");
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
    else if (section == "l")
    	return i18n("Local Documentation");
    else if (section == "n")
        return i18n("New");

    return QString::null;
}

QStringList MANProtocol::buildSectionList(const QStringList& dirs) const
{
    QStringList l;

    for (QStringList::ConstIterator it = section_names.begin();
	    it != section_names.end(); ++it)
    {
	    for (QStringList::ConstIterator dir = dirs.begin();
		    dir != dirs.end(); ++dir)
	    {
		QDir d((*dir)+"/man"+(*it));
		if (d.exists())
		{
		    l << *it;
		    break;
		}
	    }
    }
    return l;
}

void MANProtocol::showMainIndex()
{
    QString output;

    QTextStream os(&output, IO_WriteOnly);

    // print header
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<head><title>" << i18n("UNIX Manual Index") << "</title></head>" << endl;
    os << i18n("<body bgcolor=#ffffff><h1>UNIX Manual Index</h1>") << endl;

    QString sectList = getenv("MANSECT");
    QStringList sections;
    if (sectList.isEmpty())
    	sections = buildSectionList(manDirectories());
    else
	sections = QStringList::split(':', sectList);

    os << "<table>" << endl;

    QStringList::ConstIterator it;
    for (it = sections.begin(); it != sections.end(); ++it)
        os << "<tr><td><a href=\"man:(" << *it << ")\">" << i18n("Section ") << *it << "</a></td><td>&nbsp;</td><td> " << sectionName(*it) << "</td></tr>" << endl;

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    data(output.utf8());
    finished();
}


void MANProtocol::checkManPaths()
{
    static bool inited = false;

    if (inited)
        return;

    inited = true;

    QString manpath_env = QString::fromLocal8Bit( ::getenv("MANPATH") );
    //QString mansect_env = QString::fromLocal8Bit( ::getenv("MANSECT") );

    // Decide if $MANPATH is enough on its own or if it should be merged
    // with the constructed path.
    // A $MANPATH starting or ending with ":", or containing "::",
    // should be merged with the constructed path.

    bool construct_path = false;

    if ( manpath_env.isEmpty()
        || manpath_env[0] == ':'
        || manpath_env[manpath_env.length()-1] == ':'
        || manpath_env.contains( QString("::") ) )
    {
        construct_path = true; // need to read config file
    }

    // Constucted man path -- consists of paths from
    //   /etc/man.conf
    //   default dirs
    //   $PATH
    QStringList constr_path;

    QString conf_section;

    if ( construct_path ) {
        QMap<QString, QString> manpath_map;

        // Add paths from /etc/man.conf
        //
        // Explicit manpaths may be given by lines starting with "MANPATH" or
        // "MANDATORY_MANPATH" (depending on system ?).
        // Mappings from $PATH to manpath are given by lines starting with
        // "MANPATH_MAP"

        QRegExp manpath_regex( "^MANPATH\\s" );
        QRegExp mandatory_regex( "^MANDATORY_MANPATH\\s" );
        QRegExp manpath_map_regex( "^MANPATH_MAP\\s" );
        //QRegExp section_regex( "^SECTION\\s" );
        QRegExp space_regex( "\\s+" ); // for parsing manpath map

        QFile mc("/etc/man.conf");             // Caldera
        if (!mc.exists())
            mc.setName("/etc/manpath.config"); // SuSE, Debian
        if (!mc.exists())
            mc.setName("/etc/man.config");  // Mandrake

        if (mc.open(IO_ReadOnly))
        {
            QTextStream is(&mc);
	    is.setEncoding(QTextStream::Locale);

            while (!is.eof())
            {
                QString line = is.readLine();
                if ( manpath_regex.search(line, 0) == 0 )
                {
                    QString path = line.mid(8).stripWhiteSpace();
                    constr_path += path;
                }
                else if ( mandatory_regex.search(line, 0) == 0 )
                {
                    QString path = line.mid(18).stripWhiteSpace();
                    constr_path += path;
                }
                else if ( manpath_map_regex.search(line, 0) == 0 )
                {
                    // The entry is "MANPATH_MAP  <path>  <manpath>"
                    QStringList mapping =
                        QStringList::split(space_regex, line);

                    if ( mapping.count() == 3 ) {
                        QString dir = QDir::cleanDirPath( mapping[1] );
                        QString mandir = QDir::cleanDirPath( mapping[2] );

                        manpath_map[ dir ] = mandir;
                    }
                }
/* sections are not used
                else if ( section_regex.find(line, 0) == 0 )
                {
                    if ( !conf_section.isEmpty() )
                        conf_section += ':';
                    conf_section += line.mid(8).stripWhiteSpace();
                }
*/
            }
            mc.close();
        }

        // Default paths
        static const char *manpaths[] = {
                        "/usr/X11/man",
                        "/usr/X11R6/man",
                        "/usr/man",
                        "/usr/local/man",
                        "/usr/exp/man",
                        "/usr/openwin/man",
                        "/usr/dt/man",
                        "/opt/freetool/man",
                        "/opt/local/man",
                        "/usr/tex/man",
                        "/usr/www/man",
                        "/usr/lang/man",
                        "/usr/gnu/man",
                        "/usr/share/man",
                        "/usr/motif/man",
                        "/usr/titools/man",
                        "/usr/sunpc/man",
                        "/usr/ncd/man",
                        "/usr/newsprint/man",
                        NULL };


        int i = 0;
        while (manpaths[i]) {
            if ( constr_path.findIndex( QString( manpaths[i] ) ) == -1 )
                 constr_path += QString( manpaths[i] );
            i++;
        }

        // Directories in $PATH
        // - if a manpath mapping exists, use that mapping
        // - if a directory "<path>/man" or "<path>/../man" exists, add it
        //   to the man path (the actual existence check is done further down)

        if ( ::getenv("PATH") ) {
            QStringList path =
                QStringList::split( ":",
                    QString::fromLocal8Bit( ::getenv("PATH") ) );

            for ( QStringList::Iterator it = path.begin();
                  it != path.end();
                  it++ )
            {
                QString dir = QDir::cleanDirPath( *it );
                QString mandir = manpath_map[ dir ];

                if ( !mandir.isEmpty() ) {
					// a path mapping exists
                    if ( constr_path.findIndex( mandir ) == -1 )
                        constr_path += mandir;
                }
                else {
					// no manpath mapping, use "<path>/man" and "<path>/../man"

                    mandir = dir + QString( "/man" );
                    if ( constr_path.findIndex( mandir ) == -1 )
                        constr_path += mandir;

                    int pos = dir.findRev( '/' );
                    if ( pos > 0 ) {
                        mandir = dir.left( pos ) + QString("/man");
                        if ( constr_path.findIndex( mandir ) == -1 )
                            constr_path += mandir;
                    }
                }
            }
        }
    } // construct_path

    // Merge $MANPATH with the constructed path to form the
    // actual manpath.
    //
    // The merging syntax with ":" and "::" in $MANPATH will be
    // satisfied if any empty string in path_list_env (there
    // should be 1 or 0) is replaced by the constructed path.

    QStringList path_list_env = QStringList::split( ':', manpath_env , true );

    for ( QStringList::Iterator it = path_list_env.begin();
          it != path_list_env.end();
          it++ )
    {
        struct stat sbuf;

        QString dir = (*it);

        if ( !dir.isEmpty() ) {
            // Add dir to the man path if it exists
            if ( m_manpath.findIndex( dir ) == -1 ) {
                if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    m_manpath += dir;
                }
            }
        }
        else {
            // Insert constructed path ($MANPATH was empty, or
            // there was a ":" at an end or "::")

            for ( QStringList::Iterator it2 = constr_path.begin();
                  it2 != constr_path.end();
                  it2++ )
            {
                dir = (*it2);

                if ( !dir.isEmpty() ) {
                    if ( m_manpath.findIndex( dir ) == -1 ) {
                        if ( ::stat( QFile::encodeName( dir ), &sbuf ) == 0
                            && S_ISDIR( sbuf.st_mode ) )
                        {
                            m_manpath += dir;
                        }
                    }
                }
            }
        }
    }

/* sections are not used
    // Sections
    QStringList m_mansect = QStringList::split( ':', mansect_env, true );

    const char* default_sect[] =
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "n", 0L };

    for ( int i = 0; default_sect[i] != 0L; i++ )
        if ( m_mansect.findIndex( QString( default_sect[i] ) ) == -1 )
            m_mansect += QString( default_sect[i] );
*/

}


//#define _USE_OLD_CODE

#ifdef _USE_OLD_CODE
#warning "using old code"
#else

// Define this, if you want to compile with qsort from stdlib.h
// else the Qt Heapsort will be used.
// Note, qsort seems to be a bit faster (~10%) on a large man section
// eg. man section 3
#define _USE_QSORT

// Setup my own structure, with char pointers.
// from now on only pointers are copied, no strings
//
// containing the whole path string,
// the beginning of the man page name
// and the length of the name
struct man_index_t {
    char *manpath;  // the full path including man file
    const char *manpage_begin;  // pointer to the begin of the man file name in the path
    int manpage_len; // len of the man file name
};
typedef man_index_t *man_index_ptr;

#ifdef _USE_QSORT
int compare_man_index(const void *s1, const void *s2)
{
    struct man_index_t *m1 = *(struct man_index_t **)s1;
    struct man_index_t *m2 = *(struct man_index_t **)s2;
    int i;
    // Compare the names of the pages
    // with the shorter length.
    // Man page names are not '\0' terminated, so
    // this is a bit tricky
    if ( m1->manpage_len > m2->manpage_len)
    {
	i = qstrncmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m2->manpage_len);
	if (!i)
	    return 1;
	return i;
    }

    if ( m1->manpage_len < m2->manpage_len)
    {
	i = qstrncmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m1->manpage_len);
	if (!i)
	    return -1;
	return i;
    }

    return qstrncmp( m1->manpage_begin,
		     m2->manpage_begin,
		     m1->manpage_len);
}

#else /* !_USE_QSORT */
#warning using heapsort
// Set up my own man page list,
// with a special compare function to sort itself
typedef QPtrList<struct man_index_t> QManIndexListBase;
typedef QPtrListIterator<struct man_index_t> QManIndexListIterator;

class QManIndexList : public QManIndexListBase
{
public:
private:
    int compareItems( QPtrCollection::Item s1, QPtrCollection::Item s2 )
	{
	    struct man_index_t *m1 = (struct man_index_t *)s1;
	    struct man_index_t *m2 = (struct man_index_t *)s2;
	    int i;
	    // compare the names of the pages
	    // with the shorter length
	    if (m1->manpage_len > m2->manpage_len)
	    {
		i = qstrncmp(m1->manpage_begin,
			     m2->manpage_begin,
			     m2->manpage_len);
		if (!i)
		    return 1;
		return i;
	    }

	    if (m1->manpage_len > m2->manpage_len)
	    {

		i = qstrncmp(m1->manpage_begin,
			     m2->manpage_begin,
			     m1->manpage_len);
		if (!i)
		    return -1;
		return i;
	    }

	    return qstrncmp(m1->manpage_begin,
			    m2->manpage_begin,
			    m1->manpage_len);
	}
};

#endif /* !_USE_QSORT */
#endif /* !_USE_OLD_CODE */




void MANProtocol::showIndex(const QString& section)
{
    QString output;

    QTextStream os(&output, IO_WriteOnly);

    // print header
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<head><title>" << i18n("UNIX Manual Index") << "</title></head>" << endl;
    os << i18n("<body bgcolor=#ffffff><h1>Index for Section %1: %2</h1>").arg(section).arg(sectionName(section)) << endl;

    // compose list of search paths -------------------------------------------------------------

    checkManPaths();
    infoMessage(i18n("Generating Index"));

    // search for the man pages
	QStringList pages = findPages( section, QString::null );

	QMap<QString, QString> indexmap = buildIndexMap(section);

    // print out the list
    os << "<table>" << endl;

#ifdef _USE_OLD_CODE
    pages.sort();

    QMap<QString, QString> pagemap;

    QStringList::ConstIterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
        QString fileName = *page;

        stripExtension( &fileName );

        pos = fileName.findRev('/');
        if (pos > 0)
            fileName = fileName.mid(pos+1);

        if (!fileName.isEmpty())
            pagemap[fileName] = *page;

    }

    for (QMap<QString,QString>::ConstIterator it = pagemap.begin();
	 it != pagemap.end(); ++it)
    {
	os << "<tr><td><a href=\"man:" << it.data() << "\">\n"
	   << it.key() << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(it.key()) ? indexmap[it.key()] : "" )
	   << "</td></tr>"  << endl;
    }

#else /* ! _USE_OLD_CODE */

#ifdef _USE_QSORT

    int listlen = pages.count();
    man_index_ptr *indexlist = new man_index_ptr[listlen];
    listlen = 0;

#else /* !_USE_QSORT */

    QManIndexList manpages;
    manpages.setAutoDelete(TRUE);

#endif /* _USE_QSORT */

    QStringList::ConstIterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
	// I look for the beginning of the man page name
	// i.e. "bla/pagename.3.gz" by looking for the last "/"
	// Then look for the end of the name by searching backwards
	// for the last ".", not counting zip extensions.
	// If the len of the name is >0,
	// store it in the list structure, to be sorted later

        char *manpage_end;
        struct man_index_t *manindex = new man_index_t;
	manindex->manpath = strdup((*page).utf8());

	manindex->manpage_begin = strrchr(manindex->manpath, '/');
	if (manindex->manpage_begin)
	{
	    manindex->manpage_begin++;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}
	else
	{
	    manindex->manpage_begin = manindex->manpath;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}

	// Skip extension ".section[.gz]"

	char *begin = (char*)(manindex->manpage_begin);
	int len = strlen( begin );
	char *end = begin+(len-1);

	if ( len >= 3 && strcmp( end-2, ".gz" ) == 0 )
	    end -= 3;
	else if ( len >= 2 && strcmp( end-1, ".Z" ) == 0 )
	    end -= 2;
	else if ( len >= 2 && strcmp( end-1, ".z" ) == 0 )
	    end -= 2;
	else if ( len >= 4 && strcmp( end-3, ".bz2" ) == 0 )
	    end -= 4;

	while ( end >= begin && *end != '.' )
	    end--;

	if ( end < begin )
	    manpage_end = 0;
	else
	    manpage_end = end;

	if (NULL == manpage_end)
	{
	    // no '.' ending ???
	    // set the pointer past the end of the filename
	    manindex->manpage_len = (*page).length();
	    manindex->manpage_len -= (manindex->manpage_begin - manindex->manpath);
	    assert(manindex->manpage_len >= 0);
	}
	else
	{
	    manindex->manpage_len = (manpage_end - manindex->manpage_begin);
	    assert(manindex->manpage_len >= 0);
	}

	if (0 < manindex->manpage_len)
	{

#ifdef _USE_QSORT

	    indexlist[listlen] = manindex;
	    listlen++;

#else /* !_USE_QSORT */

	    manpages.append(manindex);

#endif /* _USE_QSORT */

	}
    }

    //
    // Now do the sorting on the page names
    // and the printout afterwards
    // While printing avoid duplicate man page names
    //

    struct man_index_t dummy_index = {0l,0l,0};
    struct man_index_t *last_index = &dummy_index;

#ifdef _USE_QSORT

    // sort and print
    qsort(indexlist, listlen, sizeof(struct man_index_t *), compare_man_index);

    for (int i=0; i<listlen; i++)
    {
	struct man_index_t *manindex = indexlist[i];

	// qstrncmp():
	// "last_man" has already a \0 string ending, but
	// "manindex->manpage_begin" has not,
	// so do compare at most "manindex->manpage_len" of the strings.
	if (last_index->manpage_len == manindex->manpage_len &&
	    !qstrncmp(last_index->manpage_begin,
		      manindex->manpage_begin,
		      manindex->manpage_len)
	    )
	{
	    continue;
	}
	os << "<tr><td><a href=\"man:"
	   << manindex->manpath << "\">\n";

	((char *)manindex->manpage_begin)[manindex->manpage_len] = '\0';
	os << manindex->manpage_begin
	   << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
	   << "</td></tr>"  << endl;
	last_index = manindex;
    }

    for (int i=0; i<listlen; i++) {
	::free(indexlist[i]->manpath);   // allocated by strdup
	delete indexlist[i];
    }

    delete [] indexlist;

#else /* !_USE_QSORT */

    manpages.sort(); // using

    for (QManIndexListIterator mit(manpages);
	 mit.current();
	 ++mit )
    {
	struct man_index_t *manindex = mit.current();

	// qstrncmp():
	// "last_man" has already a \0 string ending, but
	// "manindex->manpage_begin" has not,
	// so do compare at most "manindex->manpage_len" of the strings.
	if (last_index->manpage_len == manindex->manpage_len &&
	    !qstrncmp(last_index->manpage_begin,
		      manindex->manpage_begin,
		      manindex->manpage_len)
	    )
	{
	    continue;
	}

	os << "<tr><td><a href=\"man:"
	   << manindex->manpath << "\">\n";

	manindex->manpage_begin[manindex->manpage_len] = '\0';
	os << manindex->manpage_begin
	   << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
	   << "</td></tr>"  << endl;
	last_index = manindex;
    }
#endif /* _USE_QSORT */
#endif /* _USE_OLD_CODE */

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    infoMessage(QString::null);
    mimeType("text/html");
    data(output.utf8());
    finished();
}

void MANProtocol::listDir(const KURL &url)
{
    kdDebug( 7107 ) << "ENTER listDir: " << url.prettyURL() << endl;

    QString title;
    QString section;

    if ( !parseUrl(url.path(), title, section) ) {
        error( KIO::ERR_MALFORMED_URL, url.url() );
        return;
    }

    QStringList list = findPages( section, QString::null, false );

    UDSEntryList uds_entry_list;
    UDSEntry     uds_entry;
    UDSAtom      uds_atom;

    uds_atom.m_uds = KIO::UDS_NAME; // we only do names...
    uds_entry.append( uds_atom );

    QStringList::Iterator it = list.begin();
    QStringList::Iterator end = list.end();

    for ( ; it != end; it++ ) {
        stripExtension( &(*it) );

        uds_entry[0].m_str = *it;
        uds_entry_list.append( uds_entry );
    }

    listEntries( uds_entry_list );
    finished();
}

void MANProtocol::getProgramPath()
{
  if (!mySgml2RoffPath.isEmpty())
    return;

  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff");
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* sgml2roff isn't found in PATH. Check some possible locations where it may be found. */
  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff", QString(SGML2ROFF_DIRS));
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* Cannot find sgml2roff programm: */
  outputError(i18n("Could not find the sgml2roff program on your system. Please install it, if necessary, and extend the search path by adjusting the environment variable PATH before starting KDE."));
  finished();
  exit();
}
