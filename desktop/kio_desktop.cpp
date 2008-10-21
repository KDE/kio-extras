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
#include <QDesktopServices>
#include <QDir>

extern "C"
{
    int KDE_EXPORT kdemain(int argc, char **argv)
    {
        // necessary to use other kio slaves
        KComponentData( "kio_desktop" );
        QCoreApplication app( argc, argv );

        // start the slave
        DesktopProtocol slave(argv[1], argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

DesktopProtocol::DesktopProtocol(const QByteArray& protocol, const QByteArray &pool, const QByteArray &app)
    : KIO::ForwardingSlaveBase(protocol, pool, app)
{
    checkLocalInstall();

    QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
    kded.call("loadModule", "desktopnotifier");
}

DesktopProtocol::~DesktopProtocol()
{
}

void DesktopProtocol::checkLocalInstall()
{
#ifndef Q_WS_WIN
    // We can't use KGlobalSettings::desktopPath() here, since it returns the home dir
    // if the desktop folder doesn't exist.
    QString desktopPath = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    if (desktopPath.isEmpty())
        desktopPath = QDir::homePath() + "/Desktop";

    const QDir desktopDir(desktopPath);
    bool desktopIsEmpty;
    bool newRelease;

    // Check if we have a new KDE release
    KConfig config("kio_desktoprc");
    KConfigGroup cg(&config, "General");
    QString version = cg.readEntry("Version", "0.0.0");
    int major = version.section('.', 0, 0).toInt();
    int minor = version.section('.', 1, 1).toInt();
    int release = version.section('.', 2, 2).toInt();

    if (KDE_MAKE_VERSION(major, minor, release) < KDE::version()) {
        const QString version = QString::number(KDE::versionMajor()) + '.' +
                                QString::number(KDE::versionMinor()) + '.' +
                                QString::number(KDE::versionRelease());
        cg.writeEntry("Version", version);
        newRelease = true;
    } else 
        newRelease = false;

    // Create the desktop folder if it doesn't exist
    if (!desktopDir.exists()) {
        ::mkdir(QFile::encodeName(desktopPath), S_IRWXU);
        desktopIsEmpty = true;
    } else
        desktopIsEmpty = desktopDir.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty();

    if (desktopIsEmpty) {
        // Copy the .directory file
        QFile::copy(KStandardDirs::locate("data", "kio_desktop/directory.desktop"),
                    desktopPath + "/.directory");

        // Copy the trash link
        QFile::copy(KStandardDirs::locate("data", "kio_desktop/directory.trash"),
                    desktopPath + "/trash.desktop");
 
        // Copy the desktop links
        const QStringList links = KGlobal::dirs()->findAllResources("data", "kio_desktop/DesktopLinks/*",
                                                                    KStandardDirs::NoDuplicates);
        foreach (const QString &link, links) {
            KDesktopFile file(link);
            if (!file.desktopGroup().readEntry("Hidden", false))
                QFile::copy(link, desktopPath + link.mid(link.lastIndexOf('/')));
        }
    } else if (newRelease) {
        // Update the icon name in the .directory file to the FDO naming spec
        const QString directoryFile = desktopPath + "/.directory";
        if (QFile::exists(directoryFile)) {
             KDesktopFile file(directoryFile);
             if (file.readIcon() == "desktop")
                 file.desktopGroup().writeEntry("Icon", "user-desktop");
        } else
             QFile::copy(KStandardDirs::locate("data", "kio_desktop/directory.desktop"), directoryFile);
  
        // Update the home icon to the FDO naming spec
        const QString homeLink = desktopPath + "/Home.desktop";
        if (QFile::exists(homeLink)) {
            KDesktopFile home(homeLink);
            const QString icon = home.readIcon();
            if (icon == "kfm_home" || icon == "folder_home")
                home.desktopGroup().writeEntry("Icon", "user-home");
        }

        // Update the trash icon to the FDO naming spec  
        const QString trashLink = desktopPath + "/trash.desktop";
        if (QFile::exists(trashLink)) {
            KDesktopFile trash(trashLink);
            if (trash.readIcon() == "trashcan_full")
                trash.desktopGroup().writeEntry("Icon", "user-trash-full");
            if (trash.desktopGroup().readEntry("EmptyIcon") == "trashcan_empty")
                trash.desktopGroup().writeEntry("EmptyIcon", "user-trash");
        }
    }
#endif
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

