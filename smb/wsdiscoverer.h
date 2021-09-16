/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019-2021 Harald Sitter <sitter@kde.org>
*/

#ifndef WSDISCOVERER_H
#define WSDISCOVERER_H

#include "discovery.h"
#include <QHash>
#include <QObject>
#include <QTimer>

class WSDiscoveryClient;
class WSDiscoveryTargetService;
class PBSDResolver;
class WSDResolver;

namespace KIO
{
class UDSEntry;
}

class WSDiscovery : public Discovery
{
    const QString m_computer;
    const QString m_remote;

public:
    WSDiscovery(const QString &computer, const QString &remote);
    QString udsName() const override;
    KIO::UDSEntry toEntry() const override;
};

class WSDiscoverer : public QObject, public Discoverer
{
    Q_OBJECT
public:
    WSDiscoverer();
    ~WSDiscoverer() override;

    void start() override;
    bool isFinished() const override;

Q_SIGNALS:
    void newDiscovery(Discovery::Ptr discovery) override;
    void finished() override;

private Q_SLOTS:
    void matchReceived(const WSDiscoveryTargetService &matchedService);
    void resolveReceived(const WSDiscoveryTargetService &matchedService);

private:
    void stop() override;
    void maybeFinish();

    WSDiscoveryClient *m_client = nullptr;
    bool m_startedTimer = false;
    QTimer m_probeMatchTimer;
    QStringList m_seenEndpoints;
    QList<PBSDResolver *> m_resolvers;
    int m_resolvedCount = 0;
    QHash<QString, WSDResolver *> m_endpointResolvers;

    Q_DISABLE_COPY_MOVE(WSDiscoverer)
};

#endif // WSDISCOVERER_H
