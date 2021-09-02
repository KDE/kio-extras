/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kio_man.h"
#include "kio_man_debug.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextCodec>
#include <QProcess>

#include <KLocalizedString>

#include "man2html.h"
#include <karchive_version.h>
#if KARCHIVE_VERSION >= QT_VERSION_CHECK(5, 85, 0)
#include <KCompressionDevice>
#else
#include <KFilterDev>
#endif


using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.man" FILE "man.json")
};

MANProtocol *MANProtocol::s_self = nullptr;

static const char *SGML2ROFF_DIRS = "/usr/lib/sgml";
static const char *SGML2ROFF_EXECUTABLE = "sgml2roff";


/*
 * Drop trailing compression suffix from name
 */
static QString stripCompression(const QString &name)
{
    int pos = name.length();

    if (name.endsWith(".gz")) pos -= 3;
    else if (name.endsWith(".z", Qt::CaseInsensitive)) pos -= 2;
    else if (name.endsWith(".bz2")) pos -= 4;
    else if (name.endsWith(".bz")) pos -= 3;
    else if (name.endsWith(".lzma")) pos -= 5;
    else if (name.endsWith(".xz")) pos -= 3;

    return (pos>0 ? name.left(pos) : name);
}

/*
 * Drop trailing compression suffix and section from name
 */
static QString stripExtension(const QString &name)
{
    QString wc = stripCompression(name);
    const int pos = wc.lastIndexOf('.');
    return (pos>0 ? wc.left(pos) : wc);
}

static bool parseUrl(const QString &_url, QString &title, QString &section)
{
    section.clear();

    QString url = _url.trimmed();
    if (url.isEmpty() || url.startsWith('/')) {
        if (url.isEmpty() || QFile::exists(url)) {
            // man:/usr/share/man/man1/ls.1.gz is a valid file
            title = url;
            return true;
        } else {
            // If a full path is specified but does not exist,
            // then it is perhaps a normal man page.
            qCDebug(KIO_MAN_LOG) << url << " does not exist";
        }
    }

    while (url.startsWith('/')) url.remove(0, 1);
    title = url;

    int pos = url.indexOf('(');
    if (pos < 0)
        return true; // man:ls -> title=ls

    title = title.left(pos);
    section = url.mid(pos+1);

    pos = section.indexOf(')');
    if (pos >= 0) {
        if (pos < section.length() - 2 && title.isEmpty()) {
            title = section.mid(pos + 2);
        }
        section = section.left(pos);
    }

    // man:ls(2) -> title="ls", section="2"

    return true;
}


MANProtocol::MANProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : QObject(), SlaveBase("man", pool_socket, app_socket)
{
    Q_ASSERT(s_self==nullptr);
    s_self = this;

    m_sectionNames << "0" << "0p" << "1" << "1p" << "2" << "3" << "3n" << "3p" << "4" << "5" << "6" << "7"
                  << "8" << "9" << "l" << "n";

    const QString cssPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_docfilter/kio_docfilter.css"));
    m_manCSSFile = QFile::encodeName(QUrl::fromLocalFile(cssPath).url());
}

MANProtocol *MANProtocol::self()
{
    return s_self;
}

MANProtocol::~MANProtocol()
{
    s_self = nullptr;
}


void MANProtocol::parseWhatIs( QMap<QString, QString> &i, QTextStream &t, const QString &mark )
{
    const QRegularExpression re(mark);
    QString l;
    while ( !t.atEnd() )
    {
        l = t.readLine();
        QRegularExpressionMatch match = re.match(l);
        int pos = match.capturedStart(0);
        if (pos != -1)
        {
            QString names = l.left(pos);
            QString descr = l.mid(match.capturedEnd(0));
            while ((pos = names.indexOf(",")) != -1)
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
    if (!f.open(QIODevice::ReadOnly)) return false;

    QTextStream t(&f);
    parseWhatIs( i, t, mark );
    return true;
}

QMap<QString, QString> MANProtocol::buildIndexMap(const QString &section)
{
    qCDebug(KIO_MAN_LOG) << "for section" << section;

    QMap<QString, QString> i;
    QStringList man_dirs = manDirectories();
    // Supplementary places for whatis databases
    man_dirs += m_mandbpath;
    if (!man_dirs.contains("/var/cache/man"))
        man_dirs << "/var/cache/man";
    if (!man_dirs.contains("/var/catman"))
        man_dirs << "/var/catman";

    QStringList names;
    names << "whatis.db" << "whatis";
    const QString mark = "\\s+\\(" + section + "[a-z]*\\)\\s+-\\s+";

    int count0;
    for (const QString &it_dir : qAsConst(man_dirs))
    {
        if (!QFile::exists(it_dir)) continue;

        bool added = false;
        for (const QString &it_name : qAsConst(names))
        {
            count0 = i.count();
            if (addWhatIs(i, (it_dir+'/'+it_name), mark))
            {
                qCDebug(KIO_MAN_LOG) << "added" << (i.count()-count0) << "from" << it_name << "in" << it_dir;
                added = true;
                break;
            }
        }

        if (!added)
        {
            // Nothing was able to be added by scanning the directory,
            // so try parsing the output of the whatis(1) command.
            QProcess proc;
            proc.setProgram("whatis");
            proc.setArguments(QStringList() << "-M" << it_dir << "-w" << "*");
            proc.setProcessChannelMode( QProcess::ForwardedErrorChannel );
            proc.start();
            proc.waitForFinished();
            QTextStream t( proc.readAllStandardOutput(), QIODevice::ReadOnly );

            count0 = i.count();
            parseWhatIs( i, t, mark );
            qCDebug(KIO_MAN_LOG) << "added" << (i.count()-count0) << "from whatis in" << it_dir;
        }
    }

    qCDebug(KIO_MAN_LOG) << "returning" << i.count() << "index entries";
    return i;
}

//---------------------------------------------------------------------

QStringList MANProtocol::manDirectories()
{
    checkManPaths();
    //
    // Build a list of man directories including translations
    //
    QStringList man_dirs;
    const QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);

    for (const QString &it_dir : qAsConst(m_manpath))
    {
        // Translated pages in "<mandir>/<lang>" if the directory
        // exists
        for (const QLocale &it_loc : locales)
        {
            // TODO: languageToString() is wrong, that returns the readable name
            // of the language.  We want the country code returned by name().
            QString lang = QLocale::languageToString(it_loc.language());
            if ( !lang.isEmpty() && lang!=QString("C") )
            {
                QString dir = it_dir+'/'+lang;
                QDir d(dir);
                if (d.exists())
                {
                    const QString p = d.canonicalPath();
                    if (!man_dirs.contains(p)) man_dirs += p;
                }
            }
        }

        // Untranslated pages in "<mandir>"
        const QString p = QDir(it_dir).canonicalPath();
        if (!man_dirs.contains(p)) man_dirs += p;
    }

    qCDebug(KIO_MAN_LOG) << "returning" << man_dirs.count() << "man directories";
    return man_dirs;
}

QStringList MANProtocol::findPages(const QString &_section,
                                   const QString &title,
                                   bool full_path)
{
    QStringList list;
    // qCDebug(KIO_MAN_LOG) << "findPages '" << section << "' '" << title << "'\n";
    if (title.startsWith('/'))				// absolute man page path
    {
        list.append(title);				// just that and nothing else
        return list;
    }

    const QString star("*");				// flag for finding sections
    const QString man("man");				// prefix for ROFF subdirectories
    const QString sman("sman");				// prefix for SGML subdirectories

    // Generate a list of applicable sections
    QStringList sect_list;
    if (!_section.isEmpty())				// section requested as parameter
    {
        sect_list += _section;

        // It's not clear what this code does.  If a section with a letter
        // suffix is specified, for example "3p", both "3p" and "3" are
        // added to the section list.  The result is that "man:(3p)" returns
        // the same entries as "man:(3)"
        QString section = _section;
        while ( (!section.isEmpty()) && (section.at(section.length() - 1).isLetter()) ) {
            section.truncate(section.length() - 1);
            sect_list += section;
        }
    }
    else
    {
        sect_list += star;
    }

    const QStringList man_dirs = manDirectories();
    QStringList nameFilters;
    nameFilters += man+'*';
    nameFilters += sman+'*';

    // Find man pages in the sections specified above,
    // or all sections.
    //
    // Do not convert this loop to an iterator, or 'it_s' below
    // to a reference.  The 'sect_list' list can be modified
    // within the loop.
    for (int i = 0; i<sect_list.count(); ++i)
    {
        const QString it_s = sect_list.at(i);
        QString it_real = it_s.toLower();

        // Find applicable pages within all man directories.
        for (const QString &man_dir : man_dirs)
        {
            // Find all subdirectories named "man*" and "sman*"
            // to extend the list of sections and find the correct
            // case for the section suffix.
            QDir dp(man_dir);
            dp.setFilter(QDir::Dirs|QDir::NoDotAndDotDot);
            dp.setNameFilters(nameFilters);
            const QStringList entries = dp.entryList();
            for (const QString &file : entries)
            {
                QString sect;
                if (file.startsWith(man)) sect = file.mid(man.length());
                else if (file.startsWith(sman)) sect = file.mid(sman.length());
                // Should never happen, because of the name filter above.
                if (sect.isEmpty()) continue;

                if (sect.toLower()==it_real) it_real = sect;

                // Only add sect if not already contained, avoid duplicates
                if (!sect_list.contains(sect) && _section.isEmpty())
                {
                    //qCDebug(KIO_MAN_LOG) << "another section " << sect;
                    sect_list += sect;
                }
            }

            if (it_s!=star)				// finding pages, not just sections
            {
                const QString dir = man_dir + '/' + man + it_real + '/';
                list.append(findManPagesInSection(dir, title, full_path));

                const QString sdir = man_dir + '/' + sman + it_real + '/';
                list.append(findManPagesInSection(sdir, title, full_path));
            }
        }
    }

    //qCDebug(KIO_MAN_LOG) << "finished " << list << " " << sect_list;
    return list;
}

QStringList MANProtocol::findManPagesInSection(const QString &dir, const QString &title, bool full_path)
{
    QStringList list;

    qCDebug(KIO_MAN_LOG) << "in" << dir << "title" << title;
    const bool title_given = !title.isEmpty();

    QDir dp(dir);
    dp.setFilter(QDir::Files);
    const QStringList names = dp.entryList();
    for (const QString &name : names)
    {
        if (title_given)
        {
            // check title if we're looking for a specific page
            if (!name.startsWith(title)) continue;
            // beginning matches, do a more thorough check...
            const QString tmp_name = stripExtension(name);
            if (tmp_name!=title) continue;
        }

        list.append(full_path ? dir+name : name);
    }

    qCDebug(KIO_MAN_LOG) << "returning" << list.count() << "pages";
    return (list);
}

//---------------------------------------------------------------------

void MANProtocol::output(const char *insert)
{
    if (insert)
    {
        m_outputBuffer.write(insert,strlen(insert));
    }
    if (!insert || m_outputBuffer.pos() >= 2048)
    {
        m_outputBuffer.close();
        data(m_outputBuffer.buffer());
        m_outputBuffer.setData(QByteArray());
        m_outputBuffer.open(QIODevice::WriteOnly);
    }
}

#ifndef SIMPLE_MAN2HTML
// called by man2html
extern char *read_man_page(const char *filename)
{
    return MANProtocol::self()->readManPage(filename);
}

// called by man2html
extern void output_real(const char *insert)
{
    MANProtocol::self()->output(insert);
}
#endif

//---------------------------------------------------------------------

void MANProtocol::get(const QUrl &url)
{
    qCDebug(KIO_MAN_LOG) << "GET " << url.url();

    // Indicate the mimetype - this will apply to both
    // formatted man pages and error pages.
    mimeType("text/html");

    QString title, section;
    if (!parseUrl(url.path(), title, section))
    {
        showMainIndex();
        finished();
        return;
    }

    // see if an index was requested
    if (url.query().isEmpty() && (title.isEmpty() || title == "/" || title == "."))
    {
        if (section == "index" || section.isEmpty())
            showMainIndex();
        else
            showIndex(section);
        finished();
        return;
    }

    QStringList foundPages = findPages(section, title);
    if (foundPages.isEmpty())
    {
        outputError(xi18nc("@info", "No man page matching <resource>%1</resource> could be found."
                           "<nl/><nl/>"
                           "Check that you have not mistyped the name of the page, "
                           "and note that man page names are case sensitive."
                           "<nl/><nl/>"
                           "If the name is correct, then you may need to extend the search path "
                           "for man pages, either using the <envar>MANPATH</envar> environment "
                           "variable or a configuration file in the <filename>/etc</filename> "
                           "directory.", title.toHtmlEscaped()));
        return;
    }

    // Sort the list of pages now, for display if required and for
    // testing for equivalents below.
    std::sort(foundPages.begin(), foundPages.end());
    const QString pageFound = foundPages.first();

    if (foundPages.count()>1)
    {
        // See if the multiple pages found refer to the same man page, for example
        // if 'foo.1' and 'foo.1.gz' were both found.  To make this generic with
        // regard to compression suffixes, assume that the first page name (after
        // the list has been sorted above) is the shortest.  Then check that all of
        // the others are the same with a possible compression suffix added.
        for (int i = 1; i<foundPages.count(); ++i)
        {
            if (!foundPages[i].startsWith(pageFound+'.'))
            {
                // There is a page which is not the same as the reference, even
                // allowing for a compression suffix.  Output the list of multiple
                // pages only.
                outputMatchingPages(foundPages);
                finished();
                return;
            }
        }
    }

    setCssFile(m_manCSSFile);
    m_outputBuffer.open(QIODevice::WriteOnly);
    const QByteArray filename = QFile::encodeName(pageFound);
    const char *buf = readManPage(filename);
    if (buf==nullptr) return;

    // will call output_real
    scan_man_page(buf);
    delete [] buf;

    output(nullptr); // flush

    m_outputBuffer.close();
    data(m_outputBuffer.buffer());
    m_outputBuffer.setData(QByteArray());

    // tell we are done
    data(QByteArray());
    finished();
}

//---------------------------------------------------------------------

// If this function returns nullptr to indicate a problem,
// then it must call outputError() first.
char *MANProtocol::readManPage(const char *_filename)
{
    QByteArray filename = _filename;
    QByteArray array, dirName;

    // Determine the type of man page file by checking its path
    // Determination by MIME type with KMimeType doesn't work reliably.
    // E.g., Solaris 7: /usr/man/sman7fs/pcfs.7fs -> text/x-csrc - WRONG
    //
    // If the path name contains a component "sman", assume that it's SGML
    // and convert it to roff format (used on Solaris).  Check for a pathname
    // component of "sman" only - we don't want a man page called "gasman.1"
    // to match.
    //QString file_mimetype = KMimeType::findByPath(QString(filename), 0, false)->name();

    if (QString(filename).contains("/sman/", Qt::CaseInsensitive))
    {
        QProcess proc;
        // Determine path to sgml2roff, if not already done.
        if (!getProgramPath()) return nullptr;
        proc.setProgram(mySgml2RoffPath);
        proc.setArguments(QStringList() << filename);
        proc.setProcessChannelMode( QProcess::ForwardedErrorChannel );
        proc.start();
        proc.waitForFinished();
        array = proc.readAllStandardOutput();
    }
    else
    {
        if (QDir::isRelativePath(filename))
        {
            qCDebug(KIO_MAN_LOG) << "relative" << filename;
            filename = QDir::cleanPath(lastdir + '/' + filename).toUtf8();
            qCDebug(KIO_MAN_LOG) << "resolved to" << filename;
        }

        lastdir = filename.left(filename.lastIndexOf('/'));

        // get the last directory name (which might be a language name, to be able to guess the encoding)
        QDir dir(lastdir);
        dir.cdUp();
        dirName = QFile::encodeName(dir.dirName());

        if ( !QFile::exists(QFile::decodeName(filename)) )  // if given file does not exist, find with suffix
        {
            qCDebug(KIO_MAN_LOG) << "not existing " << filename;
            QDir mandir(lastdir);
            const QString nameFilter = filename.mid(filename.lastIndexOf('/') + 1) + ".*";
            mandir.setNameFilters(QStringList(nameFilter));

            const QStringList entries = mandir.entryList();
            if (entries.isEmpty())
            {
                outputError(xi18nc("@info", "The specified man page references "
                                   "another page <filename>%1</filename>,"
                                   "<nl/>"
                                   "but the referenced page <filename>%2</filename> "
                                   "could not be found.",
                                   QFile::decodeName(filename),
                                   QDir::cleanPath(lastdir + '/' + nameFilter)));
                return nullptr;
            }

            filename = lastdir + '/' + QFile::encodeName(entries.first());
            qCDebug(KIO_MAN_LOG) << "resolved to " << filename;
        }

#if KARCHIVE_VERSION >= QT_VERSION_CHECK(5, 85, 0)
        KCompressionDevice fd(QFile::encodeName(filename));
#else
        KFilterDev fd(QFile::encodeName(filename));
#endif

        if (!fd.open(QIODevice::ReadOnly)) {
            outputError(xi18nc("@info", "The man page <filename>%1</filename> could not be read.", QFile::decodeName(filename)));
            return nullptr;
        }

        array = fd.readAll();
        qCDebug(KIO_MAN_LOG) << "read " << array.size();
    }

    if (array.isEmpty()) {
        outputError(xi18nc("@info", "The man page <filename>%1</filename> could not be converted.", QFile::decodeName(filename)));
        return nullptr;
    }

    return manPageToUtf8(array, dirName);		// never returns nullptr
}

//---------------------------------------------------------------------

void MANProtocol::outputHeader(QTextStream &os, const QString &header, const QString &title)
{
    os.setCodec("UTF-8");

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">\n";
    os << "<html><head>\n";
    os << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n";
    os << "<title>" << (!title.isEmpty() ? title : header) << "</title>\n";
    if (!m_manCSSFile.isEmpty()) {
        os << "<link href=\"" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">\n";
    }
    os << "</head>\n\n";
    os << "<body>\n";
    os << "<h1>" << header << "</h1>\n";

    os.flush();
}


// This calls SlaveBase::finished(), so do not call any other
// SlaveBase functions afterwards.  It is assumed that mimeType()
// has already been called at the start of get().
void MANProtocol::outputError(const QString& errmsg)
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    outputHeader(os, i18n("Manual Page Viewer Error"));
    os << errmsg << "\n";
    os << "</body>\n";
    os << "</html>\n";

    os.flush();
    data(array);
    data(QByteArray());
    finished();
}


void MANProtocol::outputMatchingPages(const QStringList &matchingPages)
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    outputHeader(os, i18n("There is more than one matching man page:"), i18n("Multiple Manual Pages"));
    os << "<ul>\n";

    int acckey = 1;
    for (const QString &page : matchingPages)
    {
        os << "<li><a href='man:" << page << "' accesskey='" << acckey << "'>" << page << "</a><br>\n<br>\n";
        ++acckey;
    }

    os << "</ul>\n";
    os << "<hr>\n";
    os << "<p>" << i18n("Note: if you read a man page in your language,"
                        " be aware it can contain some mistakes or be obsolete."
                        " In case of doubt, you should have a look at the English version.") << "</p>";

    os << "</body>\n";
    os << "</html>\n";
    os.flush();
    data(array);
    // Do not call finished(), the caller will do that
}


void MANProtocol::stat( const QUrl& url)
{
    qCDebug(KIO_MAN_LOG) << "STAT " << url.url();

    QString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        error(KIO::ERR_MALFORMED_URL, url.url());
        return;
    }

    qCDebug(KIO_MAN_LOG) << "URL" << url.url() << "parsed to title" << title << "section" << section;

    UDSEntry entry;
    entry.reserve(3);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, title);
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("text/html"));

    statEntry(entry);
    finished();
}


extern "C"
{

    int Q_DECL_EXPORT kdemain( int argc, char **argv ) {

        QCoreApplication app(argc, argv);
        app.setApplicationName(QLatin1String("kio_man"));

        qCDebug(KIO_MAN_LOG) <<  "STARTING";

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_man protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        MANProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        qCDebug(KIO_MAN_LOG) << "Done";

        return 0;
    }

}

void MANProtocol::mimetype(const QUrl & /*url*/)
{
    mimeType("text/html");
    finished();
}

//---------------------------------------------------------------------

static QString sectionName(const QString& section)
{
    if      (section ==  "0") return i18n("Header Files");
    else if (section == "0p") return i18n("Header Files (POSIX)");
    else if (section ==  "1") return i18n("User Commands");
    else if (section == "1p") return i18n("User Commands (POSIX)");
    else if (section ==  "2") return i18n("System Calls");
    else if (section ==  "3") return i18n("Subroutines");
    // TODO: current usage of '3p' seems to be "Subroutines (POSIX)"
    else if (section == "3p") return i18n("Perl Modules");
    else if (section == "3n") return i18n("Network Functions");
    else if (section ==  "4") return i18n("Devices");
    else if (section ==  "5") return i18n("File Formats");
    else if (section ==  "6") return i18n("Games");
    else if (section ==  "7") return i18n("Miscellaneous");
    else if (section ==  "8") return i18n("System Administration");
    else if (section ==  "9") return i18n("Kernel");
    else if (section ==  "l") return i18n("Local Documentation");
    // TODO: current usage of 'n' seems to be TCL commands
    else if (section ==  "n") return i18n("New");

    return QString();
}

QStringList MANProtocol::buildSectionList(const QStringList& dirs) const
{
    QStringList l;
    for (const QString &it_sect : qAsConst(m_sectionNames))
    {
        for (const QString &it_dir : dirs)
        {
            QDir d(it_dir+"/man"+it_sect);
            if (d.exists())
            {
                l << it_sect;
                break;
            }
        }
    }
    return l;
}

//---------------------------------------------------------------------

void MANProtocol::showMainIndex()
{
    QByteArray array;
    QTextStream os(&array, QIODevice::WriteOnly);

    outputHeader(os, i18n("Main Manual Page Index"));

    // ### TODO: why still the environment variable
    // if keeping it, also use it in listDir()
    const QString sectList = qgetenv("MANSECT");
    QStringList sections;
    if (sectList.isEmpty())
        sections = buildSectionList(manDirectories());
    else
        sections = sectList.split(':');

    os << "<table>\n";

    QSet<QChar> accessKeys;
    char alternateAccessKey = 'a';
    for (const QString &it_sect : qAsConst(sections))
    {
        if (it_sect.isEmpty()) continue;		// guard back() below

        // create a unique access key
        QChar accessKey = it_sect.back();		// rightmost char
        while (accessKeys.contains(accessKey)) accessKey = alternateAccessKey++;
        accessKeys.insert(accessKey);

        os << "<tr><td><a href=\"man:(" << it_sect << ")\" accesskey=\"" << accessKey
           << "\">" << i18n("Section %1", it_sect)
           << "</a></td><td>&nbsp;</td><td> " << sectionName(it_sect) << "</td></tr>\n";
    }

    os << "</table>\n";
    os << "</body>\n";
    os << "</html>\n";
    os.flush();
    data(array);
    data(QByteArray());
    // Do not call finished(), the caller will do that
}

//---------------------------------------------------------------------

// TODO: constr_catmanpath modified here, should parameter be passed by reference?
void MANProtocol::constructPath(QStringList& constr_path, QStringList constr_catmanpath)
{
    QMap<QString, QString> manpath_map;
    QMap<QString, QString> mandb_map;

    // Add paths from /etc/man.conf
    //
    // Explicit manpaths may be given by lines starting with "MANPATH" or
    // "MANDATORY_MANPATH" (depending on system ?).
    // Mappings from $PATH to manpath are given by lines starting with
    // "MANPATH_MAP"

    // The entry is e.g. "MANDATORY_MANPATH    <manpath>"
    const QRegularExpression manpath_regex("^(?:MANPATH|MANDATORY_MANPATH)\\s+(\\S+)");

    // The entry is "MANPATH_MAP  <path>  <manpath>"
    const QRegularExpression manpath_map_regex("^MANPATH_MAP\\s+(\\S+)\\s+(\\S+)");

    // The entry is "MANDB_MAP  <manpath>  <catmanpath>"
    const QRegularExpression mandb_map_regex("^MANDB_MAP\\s+(\\S+)\\s+(\\S+)");

    QFile mc("/etc/man.conf");             // Caldera
    if (!mc.exists())
        mc.setFileName("/etc/manpath.config"); // SuSE, Debian
    if (!mc.exists())
        mc.setFileName("/etc/man.config");  // Mandrake

    if (mc.open(QIODevice::ReadOnly))
    {
        QTextStream is(&mc);
        is.setCodec( QTextCodec::codecForLocale () );

        while (!is.atEnd())
        {
            const QString line = is.readLine();

            QRegularExpressionMatch rmatch;
            if (line.contains(manpath_regex, &rmatch)) {
                constr_path += rmatch.captured(1);
            } else if (line.contains(manpath_map_regex, &rmatch)) {
                const QString dir = QDir::cleanPath(rmatch.captured(1));
                const QString mandir = QDir::cleanPath(rmatch.captured(2));
                manpath_map[dir] = mandir;
            } else if (line.contains(mandb_map_regex, &rmatch)) {
                const QString mandir = QDir::cleanPath(rmatch.captured(1));
                const QString catmandir = QDir::cleanPath(rmatch.captured(2));
                mandb_map[mandir] = catmandir;
            }
            /* sections are not used
                    else if ( section_regex.find(line, 0) == 0 )
                    {
                    if ( !conf_section.isEmpty() )
                    conf_section += ':';
                    conf_section += line.mid(8).trimmed();
                }
            */
        }
        mc.close();
    }

    // Default paths
    static const char * const manpaths[] = {
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
        nullptr
    };


    int i = 0;
    while (manpaths[i]) {
        if ( constr_path.indexOf( QString( manpaths[i] ) ) == -1 )
            constr_path += QString( manpaths[i] );
        i++;
    }

    // Directories in $PATH
    // - if a manpath mapping exists, use that mapping
    // - if a directory "<path>/man" or "<path>/../man" exists, add it
    //   to the man path (the actual existence check is done further down)

    if ( ::getenv("PATH") ) {
        const QStringList path =
            QString::fromLocal8Bit( ::getenv("PATH") ).split( ':', Qt::SkipEmptyParts );

        for (const QString &it : qAsConst(path))
        {
            const QString dir = QDir::cleanPath(it);
            QString mandir = manpath_map[ dir ];

            if ( !mandir.isEmpty() ) {
                // a path mapping exists
                if ( constr_path.indexOf( mandir ) == -1 )
                    constr_path += mandir;
            }
            else {
                // no manpath mapping, use "<path>/man" and "<path>/../man"

                mandir = dir + QString( "/man" );
                if ( constr_path.indexOf( mandir ) == -1 )
                    constr_path += mandir;

                int pos = dir.lastIndexOf( '/' );
                if ( pos > 0 ) {
                    mandir = dir.left( pos ) + QString("/man");
                    if ( constr_path.indexOf( mandir ) == -1 )
                        constr_path += mandir;
                }
            }
            QString catmandir = mandb_map[ mandir ];
            if ( !mandir.isEmpty() )
            {
                if ( constr_catmanpath.indexOf( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
            else
            {
                // What is the default mapping?
                catmandir = mandir;
                catmandir.replace("/usr/share/","/var/cache/");
                if ( constr_catmanpath.indexOf( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
        }
    }
}

void MANProtocol::checkManPaths()
{
    static bool inited = false;
    if (inited) return;
    inited = true;

    const QString manpath_env = qgetenv("MANPATH");

    // Decide if $MANPATH is enough on its own or if it should be merged
    // with the constructed path.
    // A $MANPATH starting or ending with ":", or containing "::",
    // should be merged with the constructed path.

    const bool construct_path = (manpath_env.isEmpty() ||
                                 manpath_env.startsWith(':') ||
                                 manpath_env.endsWith(':') ||
                                 manpath_env.contains("::"));

    // Constructed man path -- consists of paths from
    //   /etc/man.conf
    //   default dirs
    //   $PATH
    QStringList constr_path;
    QStringList constr_catmanpath; // catmanpath

    if (construct_path)					// need to read config file
    {
        constructPath(constr_path, constr_catmanpath);
    }

    m_mandbpath = constr_catmanpath;

    // Merge $MANPATH with the constructed path to form the
    // actual manpath.
    //
    // The merging syntax with ":" and "::" in $MANPATH will be
    // satisfied if any empty string in path_list_env (there
    // should be 1 or 0) is replaced by the constructed path.

    const QStringList path_list_env = manpath_env.split(':', Qt::KeepEmptyParts);
    for (const QString &dir : path_list_env)
    {
        if (!dir.isEmpty())				// non empty part - add if exists
        {
            if (m_manpath.contains(dir)) continue;	// ignore if already present
            if (QDir(dir).exists()) m_manpath += dir;	// add to list if exists
        }
        else						// empty part - merge constructed path
        {
            // Insert constructed path ($MANPATH was empty, or
            // there was a ":" at either end or "::" within)
            for (const QString &dir2 : qAsConst(constr_path))
            {
                if (dir2.isEmpty()) continue;		// don't add a null entry
                if (m_manpath.contains(dir2)) continue;	// ignore if already present
							// add to list if exists
                if (QDir(dir2).exists()) m_manpath += dir2;
            }
        }
    }

    /* sections are not used
        // Sections
        QStringList m_mansect = mansect_env.split( ':', Qt::KeepEmptyParts);

        const char* const default_sect[] =
            { "1", "2", "3", "4", "5", "6", "7", "8", "9", "n", 0L };

        for ( int i = 0; default_sect[i] != 0L; i++ )
            if ( m_mansect.indexOf( QString( default_sect[i] ) ) == -1 )
                m_mansect += QString( default_sect[i] );
    */

    qCDebug(KIO_MAN_LOG) << "manpath" << m_manpath;
}


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
        i = qstrnicmp(m1->manpage_begin,
                      m2->manpage_begin,
                      m2->manpage_len);
        if (!i)
            return 1;
        return i;
    }

    if ( m1->manpage_len < m2->manpage_len)
    {
        i = qstrnicmp(m1->manpage_begin,
                      m2->manpage_begin,
                      m1->manpage_len);
        if (!i)
            return -1;
        return i;
    }

    return qstrnicmp(m1->manpage_begin,
                     m2->manpage_begin,
                     m1->manpage_len);
}

void MANProtocol::showIndex(const QString& section)
{
    QByteArray array_h;
    QTextStream os_h(&array_h, QIODevice::WriteOnly);

    // print header
    outputHeader(os_h, i18n( "Index for section %1: %2", section, sectionName(section)), i18n("Manual Page Index"));

    QByteArray array_d;
    QTextStream os(&array_d, QIODevice::WriteOnly);
    os.setCodec( "UTF-8" );
    os << "<div class=\"secidxmain\">\n";

    // compose list of search paths -------------------------------------------------------------

    checkManPaths();
    infoMessage(i18n("Generating Index"));

    // search for the man pages
    QStringList pages = findPages( section, QString() );

    if (pages.isEmpty())                // not a single page found
    {
        // print footer
        os << "</div></body></html>\n";
        os.flush();
        os_h.flush();

        infoMessage(QString());
        data(array_h + array_d);
        return;
    }

    QMap<QString, QString> indexmap = buildIndexMap(section);

    // print out the list
    os << "<br/><br/>\n";
    os << "<table>\n";

    int listlen = pages.count();
    man_index_ptr *indexlist = new man_index_ptr[listlen];
    listlen = 0;

    QStringList::const_iterator page;
    for (const QString &page : qAsConst(pages))
    {
        // I look for the beginning of the man page name
        // i.e. "bla/pagename.3.gz" by looking for the last "/"
        // Then look for the end of the name by searching backwards
        // for the last ".", not counting zip extensions.
        // If the len of the name is >0,
        // store it in the list structure, to be sorted later

        struct man_index_t *manindex = new man_index_t;
        manindex->manpath = strdup(page.toUtf8());

        manindex->manpage_begin = strrchr(manindex->manpath, '/');
        if (manindex->manpage_begin)
        {
            manindex->manpage_begin++;
            Q_ASSERT(manindex->manpage_begin >= manindex->manpath);
        }
        else
        {
            manindex->manpage_begin = manindex->manpath;
            Q_ASSERT(manindex->manpage_begin >= manindex->manpath);
        }

        // Skip extension ".section[.gz]"

        const char *begin = manindex->manpage_begin;
        const int len = strlen( begin );
        const char *end = begin+(len-1);

        if ( len >= 3 && strcmp( end-2, ".gz" ) == 0 )
            end -= 3;
        else if ( len >= 2 && strcmp( end-1, ".Z" ) == 0 )
            end -= 2;
        else if ( len >= 2 && strcmp( end-1, ".z" ) == 0 )
            end -= 2;
        else if ( len >= 4 && strcmp( end-3, ".bz2" ) == 0 )
            end -= 4;
        else if ( len >= 5 && strcmp( end-4, ".lzma" ) == 0 )
            end -= 5;
        else if ( len >= 3 && strcmp( end-2, ".xz" ) == 0 )
            end -= 3;

        while ( end >= begin && *end != '.' )
            end--;

        if (end <begin)
        {
            // no '.' ending ???
            // set the pointer past the end of the filename
            manindex->manpage_len = page.length();
            manindex->manpage_len -= (manindex->manpage_begin - manindex->manpath);
            Q_ASSERT(manindex->manpage_len >= 0);
        }
        else
        {
            const char *manpage_end = end;
            manindex->manpage_len = (manpage_end - manindex->manpage_begin);
            Q_ASSERT(manindex->manpage_len >= 0);
        }

        if (manindex->manpage_len>0)
        {
            indexlist[listlen] = manindex;
            listlen++;
        }
        else delete manindex;
    }

    //
    // Now do the sorting on the page names
    // and the printout afterwards
    // While printing avoid duplicate man page names
    //

    struct man_index_t dummy_index = {nullptr,nullptr,0};
    struct man_index_t *last_index = &dummy_index;

    // sort and print
    qsort(indexlist, listlen, sizeof(struct man_index_t *), compare_man_index);

    QChar firstchar, tmp;
    QString indexLine="<div class=\"secidxshort\">\n";
    if (indexlist[0]->manpage_len>0)
    {
        firstchar=QChar((indexlist[0]->manpage_begin)[0]).toLower();

        const QString appendixstr = QString(
                                        " [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
                                    ).arg(firstchar).arg(firstchar).arg(firstchar);
        indexLine.append(appendixstr);
    }
    os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n";
    os << "<a name=\"" << firstchar << "\">" << firstchar <<"</a>\n";
    os << "</td></tr>\n";

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

        tmp=QChar((manindex->manpage_begin)[0]).toLower();
        if (firstchar != tmp)
        {
            firstchar = tmp;
            os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n  <a name=\""
               << firstchar << "\">" << firstchar << "</a>\n</td></tr>\n";

            const QString appendixstr = QString(
                                            " [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
                                        ).arg(firstchar).arg(firstchar).arg(firstchar);
            indexLine.append(appendixstr);
        }
        os << "<tr><td><a href=\"man:"
           << manindex->manpath << "\">\n";

        ((char *)manindex->manpage_begin)[manindex->manpage_len] = '\0';
        os << manindex->manpage_begin
           << "</a></td><td>&nbsp;</td><td> "
           << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
           << "</td></tr>\n";
        last_index = manindex;
    }
    indexLine.append("</div>");

    for (int i=0; i<listlen; i++) {
        ::free(indexlist[i]->manpath);   // allocated by strdup
        delete indexlist[i];
    }

    delete [] indexlist;

    os << "</table></div>\n";
    os << "<br/><br/>\n";

    os << indexLine << '\n';

    // print footer
    os << "</body></html>\n";

    // set the links "toolbar" also at the top
    os_h << indexLine << '\n';
    os.flush();
    os_h.flush();

    infoMessage(QString());
    data(array_h + array_d);
    // Do not call finished(), the caller will do that
}

void MANProtocol::listDir(const QUrl &url)
{
    qCDebug(KIO_MAN_LOG) << url;

    QString title;
    QString section;

    if ( !parseUrl(url.path(), title, section) ) {
        error( KIO::ERR_MALFORMED_URL, url.url() );
        return;
    }

    // stat() and listDir() declared that everything is a HTML file.
    // However we can list man: and man:(1) as a directory (e.g. in dolphin).
    // But we cannot list man:ls as a directory, this makes no sense (#154173).

    if (!title.isEmpty() && title != "/") {
        error(KIO::ERR_IS_FILE, url.url());
        return;
    }

    // There is no need to accumulate a list of UDSEntry's and deliver
    // them all in one block with SlaveBase::listEntries(), because
    // SlaveBase::listEntry() batches the entries and delivers them
    // timed to maximise application performance.

    UDSEntry uds_entry;
    uds_entry.reserve(4);

    uds_entry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
    uds_entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    listEntry(uds_entry);

    if (section.isEmpty())				// list the top level directory
    {
        for (const QString &sect : m_sectionNames)
        {
            uds_entry.clear();				// sectionName() is already I18N'ed
            uds_entry.fastInsert( KIO::UDSEntry::UDS_NAME, (sect + " - " + sectionName(sect)) );
            uds_entry.fastInsert( KIO::UDSEntry::UDS_URL, ("man:/(" + sect + ')') );
            uds_entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
            listEntry(uds_entry);
        }
    }
    else						// list all pages in a section
    {
        const QStringList list = findPages(section, QString());
        for (const QString &page : list)
        {
            // Remove any compression suffix present
            QString name = stripCompression(page);
            // Remove any preceding pathname components, just leave the base name
            int pos = name.lastIndexOf('/');
            if (pos>0) name = name.mid(pos+1);
            // Reformat the section suffix into the standard form
            pos = name.lastIndexOf('.');
            if (pos>0) name = name.left(pos)+" ("+name.mid(pos+1)+')';

            uds_entry.clear();
            uds_entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
            uds_entry.fastInsert(KIO::UDSEntry::UDS_URL, ("man:" + page));
            uds_entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
            uds_entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("text/html"));
            listEntry(uds_entry);
        }
    }

    finished();
}


bool MANProtocol::getProgramPath()
{
    if (!mySgml2RoffPath.isEmpty())
        return true;

    mySgml2RoffPath = QStandardPaths::findExecutable(SGML2ROFF_EXECUTABLE);
    if (!mySgml2RoffPath.isEmpty())
        return true;

    /* sgml2roff isn't found in PATH. Check some possible locations where it may be found. */
    mySgml2RoffPath = QStandardPaths::findExecutable(SGML2ROFF_EXECUTABLE, QStringList(QLatin1String(SGML2ROFF_DIRS)));
    if (!mySgml2RoffPath.isEmpty())
        return true;

    /* Cannot find sgml2roff program: */
    outputError(xi18nc("@info", "Could not find the <command>%1</command> program on your system. "
                       "Please install it if necessary, and ensure that it can be found using "
                       "the environment variable <envar>PATH</envar>.", SGML2ROFF_EXECUTABLE));
    return false;
}

#include "kio_man.moc"
