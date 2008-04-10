/* This file is part of the KDE Project
   Copyright (C) 2008 Fredrik Höglund <fredrik@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "desktopnotifier.h"

#include <KDirWatch>
#include <KGlobal>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KPluginLoader>

#include <kdirnotify.h>


K_PLUGIN_FACTORY(DesktopNotifierFactory, registerPlugin<DesktopNotifier>();)
K_EXPORT_PLUGIN(DesktopNotifierFactory("kio_desktop"))


DesktopNotifier::DesktopNotifier(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    KDirWatch *dirWatch = new KDirWatch(this);
    dirWatch->addDir(KGlobalSettings::desktopPath());

    connect(dirWatch, SIGNAL(dirty(QString)), SLOT(dirty(QString)));
}

void DesktopNotifier::dirty(const QString &path)
{
    Q_UNUSED(path)

    // Emitting FilesAdded forces a re-read of the dir
    org::kde::KDirNotify::emitFilesAdded("desktop:/");
}

