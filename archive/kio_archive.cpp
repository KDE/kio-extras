/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kio_archive.h"

#include <QCoreApplication>
#include <QUrl>

#include <kar.h>
#include <ktar.h>
#include <kzip.h>
#include <k7zip.h>

#include "kio_archive_debug.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.archive" FILE "archive.json")
};

using namespace KIO;

extern "C" {
    int Q_DECL_EXPORT kdemain(int argc, char **argv);
}

int kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_archive"));

    qCDebug(KIO_ARCHIVE_LOG) << "Starting" << QCoreApplication::applicationPid();

    if (argc != 4)
    {
        fprintf(stderr, "Usage: kio_archive protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    ArchiveProtocol slave( argv[1], argv[2], argv[3]);
    slave.dispatchLoop();

    qCDebug(KIO_ARCHIVE_LOG) << "Done";
    return 0;
}


ArchiveProtocol::ArchiveProtocol( const QByteArray &proto, const QByteArray &pool, const QByteArray &app )
    : ArchiveProtocolBase( proto, pool, app )
{
    qCDebug(KIO_ARCHIVE_LOG);
}


KArchive *ArchiveProtocol::createArchive( const QString & proto, const QString & archiveFile )
{
    if ( proto == QLatin1String("ar") ) {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening KAr on " << archiveFile;
        return new KAr( archiveFile );
    }
    else if ( proto == QLatin1String("tar") ) {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening KTar on " << archiveFile;
        return new KTar( archiveFile );
    }
    else if ( proto == QLatin1String("zip") ) {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening KZip on " << archiveFile;
        return new KZip( archiveFile );
    }
    else if ( proto == QLatin1String("sevenz") ) {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening K7Zip on " << archiveFile;
        return new K7Zip( archiveFile );
    } else {
        qCWarning(KIO_ARCHIVE_LOG) << "Protocol" << proto << "not supported by this IOSlave" ;
        return nullptr;
    }
}

// needed for JSON file embedding
#include "kio_archive.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
