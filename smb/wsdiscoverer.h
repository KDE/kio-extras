/*
    Copyright 2019-2020 Harald Sitter <sitter@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License or any later version accepted by the membership of
    KDE e.V. (or its successor approved by the membership of KDE
    e.V.), which shall act as a proxy defined in Section 14 of
    version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef WSDISCOVERER_H
#define WSDISCOVERER_H

#include <QObject>
#include <QTimer>
#include "discovery.h"

class WSDiscoveryClient;
class WSDiscoveryTargetService;
class PBSDResolver;

namespace KIO {
class UDSEntry;
}

class WSDiscovery : public Discovery
{
    const QString m_computer;
    const QString m_remote;

public:
    WSDiscovery(const QString &computer, const QString &remote);

    KIO::UDSEntry toEntry() const override;
};

class WSDiscoverer : public QObject, public Discoverer
{
    Q_OBJECT
public:
    WSDiscoverer();

    void start() override;
    bool isFinished() const override;

signals:
    void newDiscovery(Discovery::Ptr discovery) override;
    void finished() override;

private slots:
    void matchReceived(const QSharedPointer<WSDiscoveryTargetService> &matchedService);
    void resolveReceived(const QSharedPointer<WSDiscoveryTargetService> &matchedService);

private:
    void stop() override;
    void maybeFinish();

    WSDiscoveryClient *m_client = nullptr;
    bool m_startedTimer = false;
    QTimer m_probeMatchTimer;
    QStringList m_seenEndpoints;
    QList<PBSDResolver *> m_resolvers;
    int m_resolvedCount = 0;
};

#endif // WSDISCOVERER_H
