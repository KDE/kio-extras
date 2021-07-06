// SPDX-License-Identifier: MIT

#include "info.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef Q_OS_WIN
#include <process.h>
#define getpid _getpid
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h> // getpid()
#endif

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>

#include <KLocalizedString>
#include <kshell.h>
#include <QUrl>

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.info" FILE "info.json")
};

InfoProtocol::InfoProtocol( const QByteArray &pool, const QByteArray &app )
    : SlaveBase( "info", pool, app )
{
    qCDebug(LOG_KIO_INFO);

    QStringList missingFiles;
    m_cssLocation = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_docfilter/kio_docfilter.css");
    if (m_cssLocation.isEmpty()) missingFiles.append("kio_docfilter/kio_docfilter.css");
    m_perl = QStandardPaths::findExecutable( "perl" );
    if (m_perl.isEmpty()) missingFiles.append("perl");
    m_infoScript = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_info/kde-info2html" );
    if (m_infoScript.isEmpty()) missingFiles.append("kio_info/kde-info2html");
    m_infoConf = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_info/kde-info2html.conf");
    if (m_infoConf.isEmpty()) missingFiles.append("kio_info/kde-info2html.conf");

    if (!missingFiles.isEmpty())
    {
        qCCritical(LOG_KIO_INFO) << "Cannot locate files for HTML conversion," << qPrintable(missingFiles.join(' '));
        QString errorStr = i18n("Unable to locate files which are necessary to run this service:<br>%1<br>"
                                "Please check your software installation.", missingFiles.join(' '));
        error(KIO::ERR_SLAVE_DEFINED, errorStr);
        exit();
    }

    qCDebug(LOG_KIO_INFO) << "done";
}


void InfoProtocol::get( const QUrl& url )
{
    qCDebug(LOG_KIO_INFO) << "URL" << url.toDisplayString() << "path" << url.path();

    if (url.path()=="/")
    {
        QUrl newUrl("info:/dir");
        redirection(newUrl);
        finished();
        return;
    };

    // some people write info://autoconf instead of info:/autoconf
    if (!url.host().isEmpty()) {
        QUrl newURl(url);
        newURl.setPath(url.host()+url.path());
        newURl.setHost(QString());
        redirection(newURl);
        finished();
        return;
    }

    if ( url.path().right(1) == "/" )
    {
        // Trailing / are not supported, so we need to remove them.
        QUrl newUrl( url );
        QString newPath( url.path() );
        newPath.chop( 1 );
        newUrl.setPath( newPath );
        redirection( newUrl );
        finished();
        return;
    }

    // '<' in the path looks suspicious, someone is trying info:/dir/<script>alert('xss')</script>
    if (url.path().contains('<'))
    {
        error(KIO::ERR_MALFORMED_URL, url.url());
        return;
    }

    mimeType("text/html");
    // extract the path and node from url
    decodeURL( url );

    QString cmd = KShell::quoteArg(m_perl);
    cmd += ' ';
    cmd += KShell::quoteArg(m_infoScript);
    cmd += ' ';
    cmd += KShell::quoteArg(m_infoConf);
    cmd += ' ';
    cmd += KShell::quoteArg(m_cssLocation);
    cmd += ' ';
    cmd += KShell::quoteArg(m_page);
    cmd += ' ';
    cmd += KShell::quoteArg(m_node);

    qCDebug(LOG_KIO_INFO) << "cmd" << cmd;

    FILE *file = popen( QFile::encodeName(cmd).constData(), "r" );
    if ( !file ) {
        qCDebug(LOG_KIO_INFO) << "popen failed";
        error( ERR_CANNOT_LAUNCH_PROCESS, cmd );
        return;
    }

    char buffer[ 4096 ];

    bool empty = true;
    while ( !feof( file ) )
    {
        int n = fread( buffer, 1, sizeof( buffer ), file );
        if ( !n && feof( file ) && empty ) {
            error( ERR_CANNOT_LAUNCH_PROCESS, cmd );
            return;
        }
        if ( n < 0 )
        {
            // ERROR
            qCWarning(LOG_KIO_INFO) << "read error!";
            pclose( file );
            return;
        }

        empty = false;
        data( QByteArray::fromRawData( buffer, n ) );
    }

    pclose( file );

    finished();

    qCDebug(LOG_KIO_INFO) << "done";
}

void InfoProtocol::mimetype( const QUrl& /* url */ )
{
    qCDebug(LOG_KIO_INFO);

    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();
}

void InfoProtocol::decodeURL( const QUrl &url )
{
    qCDebug(LOG_KIO_INFO) << url;

    /* Notes:
     *
     * I cleaned up the URL decoding and chose not to support URLs in the
     * form "info:/usr/local/share/info/libc.info.gz" or similar which the
     * older code attempted (and failed, maybe it had worked once) to do.
     *
     * The reason is that an obvious use such as viewing a info file off your
     * infopath would work for the first page, but then all the links would be
     * wrong. Of course, one could change kde-info2html to make it work, but I don't
     * think it worthy, others are free to disagree and write the necessary code ;)
     *
     * luis pedro
     */

    if ( url == QUrl("info:/browse_by_file?special=yes") ) {
        m_page = "#special#";
        m_node = "browse_by_file";
        qCDebug(LOG_KIO_INFO) << "InfoProtocol::decodeURL - special - browse by file";
        return;
    }

    decodePath( url.path() );
}

void InfoProtocol::decodePath( QString path )
{
    qCDebug(LOG_KIO_INFO) << path;

    m_page = "dir";  //default
    m_node = "";

    // remove leading slash
    if ('/' == path[0]) {
        path = path.mid( 1 );
    }
    //qCDebug(LOG_KIO_INFO) << "Path: " << path;

    int slashPos = path.indexOf( "/" );

    if( slashPos < 0 )
    {
        m_page = path;
        m_node = "Top";
        return;
    }

    m_page = path.left( slashPos );

    // remove leading+trailing whitespace
    m_node = path.right( path.length() - slashPos - 1).trimmed ();

    qCDebug(LOG_KIO_INFO) << "-> page" << m_page << "node" << m_node;
}

// A minimalistic stat with only the file type
// This seems to be enough for konqueror
void InfoProtocol::stat( const QUrl & )
{
    UDSEntry uds_entry;

#ifdef Q_OS_WIN
    // Regular file with rwx permission for all
    uds_entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG );
#else
    // Regular file with rwx permission for all
    uds_entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO );
#endif

    statEntry( uds_entry );
    finished();
}


extern "C" {
    int Q_DECL_EXPORT kdemain( int argc, char **argv );
}

int kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);         // needed for QSocketNotifier
    app.setApplicationName(QLatin1String("kio_info"));

    qCDebug(LOG_KIO_INFO) << "kio_info starting" << getpid();

    if (argc != 4)
    {
        fprintf(stderr, "Usage: kio_info protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    InfoProtocol slave( argv[2], argv[3] );
    slave.dispatchLoop();

    return 0;
}

#include "info.moc"
