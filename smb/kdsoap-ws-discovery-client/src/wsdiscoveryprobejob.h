/* Copyright (C) 2019 Casper Meijn <casper@meijn.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef WSDISCOVERYPROBEJOB_H
#define WSDISCOVERYPROBEJOB_H

#include "wsdiscoveryclient_export.h"
#include <KDSoapClient/KDQName>
#include <QTimer>
#include <QUrl>
#include <QObject>

class WSDiscoveryClient;
class WSDiscoveryTargetService;

class WSDISCOVERYCLIENT_EXPORT WSDiscoveryProbeJob : public QObject
{
    Q_OBJECT
public:
    explicit WSDiscoveryProbeJob(WSDiscoveryClient *parent = nullptr);

    QList<KDQName> typeList() const;
    void setTypeList(const QList<KDQName> &typeList);
    void addType(const KDQName& type);

    QList<QUrl> scopeList() const;
    void setScopeList(const QList<QUrl> &scopeList);
    void addScope(const QUrl& scope);

    int interval() const;
    void setInterval(int interval);

signals:
    void matchReceived(const QSharedPointer<WSDiscoveryTargetService>& matchedService);

public slots:
    void start();
    void stop();

private slots:
    void timeout();
    void probeMatchReceived(const QSharedPointer<WSDiscoveryTargetService>& probeMatchService);

private:
    WSDiscoveryClient * m_client;
    QList<KDQName> m_typeList;
    QList<QUrl> m_scopeList;
    QTimer m_timer;
};

#endif // WSDISCOVERYPROBEJOB_H
