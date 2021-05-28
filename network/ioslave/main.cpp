/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "networkslave.h"

// Qt
#include <QCoreApplication>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.network" FILE "network.json")
};

extern "C"
{

    int Q_DECL_EXPORT kdemain( int argc, char** argv )
    {
        QCoreApplication app( argc, argv );
        app.setApplicationName(QLatin1String("kio_network"));

        NetworkSlave slave( argv[1], argv[2], argv[3] );
        slave.dispatchLoop();

        return 0;
    }

}

#include "main.moc"
