/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KIO_ACTIVITIES_H
#define KIO_ACTIVITIES_H

#include <KIO/ForwardingSlaveBase>

#include <utils/d_ptr.h>

class ActivitiesProtocol : public KIO::ForwardingSlaveBase {
    Q_OBJECT

public:
    ActivitiesProtocol(const QByteArray &poolSocket, const QByteArray &appSocket);
    ~ActivitiesProtocol() override;

protected:
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
    void listDir(const QUrl &url) override;
    void prepareUDSEntry(KIO::UDSEntry &entry, bool listing = false) const override;
    void stat(const QUrl& url) override;
    void mimetype(const QUrl& url) override;
    void del(const QUrl& url, bool isfile) override;

private:
    D_PTR;
};

#endif // KIO_ACTIVITIES_H
