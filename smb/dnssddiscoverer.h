/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
*/

#ifndef DNSSDDISCOVERER_H
#define DNSSDDISCOVERER_H

#include <QObject>

#include <kdnssd_version.h>
#if KDNSSD_VERSION >= QT_VERSION_CHECK(5, 84, 0)
#include <KDNSSD/RemoteService>
#include <KDNSSD/ServiceBrowser>
#else
#include <DNSSD/RemoteService>
#include <DNSSD/ServiceBrowser>
#endif

#include "discovery.h"

class DNSSDDiscovery : public Discovery
{
public:
    DNSSDDiscovery(KDNSSD::RemoteService::Ptr service);
    QString udsName() const override;
    KIO::UDSEntry toEntry() const override;

private:
    KDNSSD::RemoteService::Ptr m_service;
};

class DNSSDDiscoverer : public QObject, public Discoverer
{
    Q_OBJECT
public:
    DNSSDDiscoverer();

    void start() override;
    bool isFinished() const override;

Q_SIGNALS:
    void newDiscovery(Discovery::Ptr discovery) override;
    void finished() override;

private:
    void stop() override;
    void maybeFinish();

    KDNSSD::ServiceBrowser m_browser {QStringLiteral("_smb._tcp")};
    QList<KDNSSD::RemoteService::Ptr> m_services;
    int m_resolvedCount = 0;
    bool m_disconnected = false;
};

#endif // DNSSDDISCOVERER_H
