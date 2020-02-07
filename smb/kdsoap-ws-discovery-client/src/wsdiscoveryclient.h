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
#ifndef WSDISCOVERYCLIENT_H
#define WSDISCOVERYCLIENT_H

#include "wsdiscoveryclient_export.h"
#include <QHash>
#include <QObject>

class KDSoapHeaders;
class KDSoapMessage;
class KDQName;
class KDSoapUdpClient;
class QHostAddress;
class WSDiscoveryTargetService;

class WSDISCOVERYCLIENT_EXPORT WSDiscoveryClient : public QObject
{
    Q_OBJECT
public:
    explicit WSDiscoveryClient(QObject *parent = nullptr);
    ~WSDiscoveryClient();

signals:
    void probeMatchReceived(const QSharedPointer<WSDiscoveryTargetService>& probeMatchService);
    void resolveMatchReceived(const QSharedPointer<WSDiscoveryTargetService>& probeMatchService);

public slots:
    void start();
    void sendProbe(const QList<KDQName>& typeList, const QList<QUrl> &scopeList);
    void sendResolve(const QString& endpointReference);

private slots:
    void receivedMessage(const KDSoapMessage& replyMessage, const KDSoapHeaders& replyHeaders, const QHostAddress& senderAddress, quint16 senderPort);

private:
    KDSoapUdpClient * m_soapUdpClient;
    QHash<QString, QSharedPointer<WSDiscoveryTargetService>> m_targetServiceMap;
};

#endif // WSDISCOVERYCLIENT_H
