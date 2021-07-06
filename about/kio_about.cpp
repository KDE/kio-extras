/*  This file is part of the KDE libraries

    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kio_about.h"

#include <QCoreApplication>
#include <QTextStream>

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.about" FILE "about.json")
};

AboutProtocol::AboutProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("about", pool_socket, app_socket)
{
}

AboutProtocol::~AboutProtocol()
{
}

void AboutProtocol::get( const QUrl& )
{
    QByteArray output;

    QTextStream os( &output, QIODevice::WriteOnly );
    os.setCodec( "ISO-8859-1" ); // In fact ASCII

    os << "<html><head><title>about:blank</title></head><body></body></html>";
    os.flush();

    mimeType(QStringLiteral("text/html"));
    data( output );
    finished();
}

void AboutProtocol::mimetype( const QUrl& )
{
    mimeType(QStringLiteral("text/html"));
    finished();
}

extern "C" Q_DECL_EXPORT int kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("kio_about"));

    if (argc != 4)
    {
        fprintf(stderr, "Usage: kio_about protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    AboutProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

#include "kio_about.moc"
