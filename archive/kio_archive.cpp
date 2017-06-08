/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kio_archive.h"

#include <QCoreApplication>
#include <QUrl>

#include <kar.h>
#include <ktar.h>
#include <kzip.h>

#include "kio_archive_debug.h"

using namespace KIO;

extern "C" { int Q_DECL_EXPORT kdemain(int argc, char **argv); }

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
    if ( proto == "ar" ) {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening KAr on " << archiveFile;
        return new KAr( archiveFile );
    }
    else if ( proto == "tar" ) {
      qCDebug(KIO_ARCHIVE_LOG) << "Opening KTar on " << archiveFile;
      return new KTar( archiveFile );
    }
    else if ( proto == "zip" ) {
      qCDebug(KIO_ARCHIVE_LOG) << "Opening KZip on " << archiveFile;
      return new KZip( archiveFile );
    } else {
      qCWarning(KIO_ARCHIVE_LOG) << "Protocol" << proto << "not supported by this IOSlave" ;
      return 0L;
    }
}

// kate: space-indent on; indent-width 4; replace-tabs on;
