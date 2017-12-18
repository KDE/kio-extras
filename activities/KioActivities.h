/*
 *   Copyright (C) 2012 - 2016 by Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
