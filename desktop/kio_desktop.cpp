/* This file is part of the KDE project
   Copyright (C) 2008 Fredrik Höglund <fredrik@kde.org>

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

#include "kio_desktop.h"

#include <KApplication>
#include <KCmdLineArgs>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KGlobalSettings>
#include <KStandardDirs>

#include <kio/udsentry.h>

#include <QFile>
#include <QDBusInterface>

extern "C"
{
    int KDE_EXPORT kdemain(int argc, char **argv)
    {
        // KApplication is necessary to use kio_file
        putenv(strdup("SESSION_MANAGER="));
        KCmdLineArgs::init(argc, argv, "kio_desktop", 0, KLocalizedString(), 0);

        KCmdLineOptions options;
        options.add("+protocol", ki18n("Protocol name"));
        options.add("+pool", ki18n("Socket name"));
        options.add("+app", ki18n("Socket name"));
        KCmdLineArgs::addCmdLineOptions(options);
        KApplication app(false);

        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        DesktopProtocol slave(QFile::encodeName(args->arg(0)),
                              QFile::encodeName(args->arg(1)),
                              QFile::encodeName(args->arg(2)));
        slave.dispatchLoop();
        return 0;
    }
}

DesktopProtocol::DesktopProtocol(const QByteArray& protocol, const QByteArray &pool, const QByteArray &app)
    : KIO::ForwardingSlaveBase(protocol, pool, app)
{
    QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
    kded.call("loadModule", "desktopnotifier");
}

DesktopProtocol::~DesktopProtocol()
{
}

bool DesktopProtocol::rewriteUrl(const KUrl &url, KUrl &newUrl)
{
    newUrl.setProtocol("file");
    newUrl.setPath(KGlobalSettings::desktopPath());
    newUrl.addPath(url.path());

    return true;
}

QString DesktopProtocol::desktopFile(KIO::UDSEntry &entry) const
{
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
    if (name == "." || name == "..")
        return QString();

    KUrl url = processedUrl();
    url.addPath(name);

    if (entry.isDir()) {
        url.addPath(".directory");
        if (!KStandardDirs::exists(url.path()))
            return QString();

        return url.path();
    }

    if (KDesktopFile::isDesktopFile(url.path()))
        return url.path();

    return QString();
}

void DesktopProtocol::prepareUDSEntry(KIO::UDSEntry &entry, bool listing) const
{
    ForwardingSlaveBase::prepareUDSEntry(entry, listing);
    const QString path = desktopFile(entry);

    if (!path.isEmpty()) {
        KDesktopFile file(path);

        const QString name = file.readName();
        if (!name.isEmpty())
            entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, name);

        if (file.noDisplay())
            entry.insert(KIO::UDSEntry::UDS_HIDDEN, 1);
    }
}

void DesktopProtocol::rename(const KUrl &src, const KUrl &dest, KIO::JobFlags flags)
{
    KUrl url;
    rewriteUrl(src, url);

    if (src.protocol() != "desktop" || dest.protocol() != "desktop" ||
        !KDesktopFile::isDesktopFile(url.path()))
    {
        ForwardingSlaveBase::rename(src, dest, flags);
        return;
    }

    // Instead of renaming the .desktop file, we'll change the value of the
    // Name key in the file.
    KDesktopFile file(url.path());
    file.desktopGroup().writeEntry("Name", dest.fileName());
    file.sync();
    finished();
}

