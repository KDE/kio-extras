/*
 *   SPDX-FileCopyrightText: 2022 Méven Car <meven.car@kdemail.net>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "forgetfileitemaction.h"

#include <QAction>

#include <KDirNotify>
#include <KFileItem>
#include <KIO/SimpleJob>
#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ForgetFileItemAction, "forgetfileitemaction.json")

ForgetFileItemAction::ForgetFileItemAction(QObject *parent, const QVariantList &args)
    : KAbstractFileItemActionPlugin{parent}
{
    Q_UNUSED(args)
}

QList<QAction *> ForgetFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    Q_UNUSED(parentWidget)

    const KFileItemList &fileItems = fileItemInfos.items();
    const auto url = fileItems.first().url();

    if (url.scheme() != QLatin1String("recentlyused")) {
        return {};
    }

    if (fileItems.count() == 1) {
        const auto path = url.path();
        // exclude "root" folders of recentlyused:/
        if (path.endsWith(QStringLiteral("/")) || path == QStringLiteral("/") || path == QStringLiteral("/files") || path == QStringLiteral("/locations")) {
            return {};
        }
    }

    QString text;
    if (fileItemInfos.isFile()) {
        text = i18ncp("@action:inmenu", "Forget File", "Forget Files", fileItems.size());
    } else {
        text = i18ncp("@action:inmenu", "Forget Location", "Forget Locations", fileItems.size());
    }
    QAction *forgetFileAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-clear-history")), text, this);
    forgetFileAction->setWhatsThis(i18nc("@info:whatsthis",
                                         "Remove the selected file(s) or location(s) from the recently used "
                                         "list in Dolphin and in Plasma's menus. This does not remove or move the resource(s)."));

    connect(forgetFileAction, &QAction::triggered, this, [this, fileItems, fileItemInfos]() {
        QList<QUrl> urls;
        for (const auto &item : fileItems) {
            urls << item.targetUrl();
        }

        QByteArray packedArgs;
        QDataStream stream(&packedArgs, QIODevice::WriteOnly);
        stream << int(1); // Forget, see kio-extras/recentlyused/recentlyused.cpp special
        stream << urls;

        auto job = KIO::special(QUrl(QStringLiteral("recentlyused:/")), packedArgs);

        connect(job, &KJob::finished, this, [fileItems](const KJob *job) {
            if (!job->error()) {
                org::kde::KDirNotify::emitFilesRemoved({fileItems.urlList()});
            }
        });
    });

    return {forgetFileAction};
}

#include "forgetfileitemaction.moc"
#include "moc_forgetfileitemaction.cpp"
