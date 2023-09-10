/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *   SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KIO_ACTIVITIES_H
#define KIO_ACTIVITIES_H

#include <KActivities/Consumer>
#include <KIO/ForwardingWorkerBase>
#include <utils/d_ptr.h>

class ActivitiesProtocolApi;

class ActivitiesProtocol : public KIO::ForwardingWorkerBase
{
    Q_OBJECT

public:
    ActivitiesProtocol(const QByteArray &poolSocket, const QByteArray &appSocket);
    ~ActivitiesProtocol() override;

protected:
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
    KIO::WorkerResult listDir(const QUrl &url) override;
    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult mimetype(const QUrl &url) override;

private:
    D_PTRC(ActivitiesProtocolApi);
};

#endif // KIO_ACTIVITIES_H
