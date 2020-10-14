/* Copyright (C) 2019 Casper Meijn <casper@meijn.net>
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/*!
 * \brief WSDiscoveryClient is a low-level helper for sending and receiving WS-Discovery messages. 
 * 
 * start() will bind to the network and start receveing messages. 
 * When a message is received, it will trigger the signals probeMatchReceived() and
 * resolveMatchReceived(). You can send messages using sendProbe() and sendResolve().
 * 
 * \see WSDiscoveryProbeJob for a more high level interface
 */
class WSDISCOVERYCLIENT_EXPORT WSDiscoveryClient : public QObject
{
    Q_OBJECT
public:
    /*!
     * Create a WSDiscoveryClient
     * \param parent The QObject parent
     */
    explicit WSDiscoveryClient(QObject *parent = nullptr);
    ~WSDiscoveryClient();

signals:
    /*!
     * Emitted when a WS-Discovery probe match message is received. When a single message is reveived 
     * with multiple matches, then multiple signals are emitted. 
     * \param probeMatchService The service as described in the match
     */
    void probeMatchReceived(const WSDiscoveryTargetService& probeMatchService);
    
    /*!
     * Emitted when a WS-Discovery resolve match message is received. 
     * \param probeMatchService The service as described in the match
     */
    //TODO: Rename parameter
    void resolveMatchReceived(const WSDiscoveryTargetService& probeMatchService);

public slots:
    /*!
    * Bind to the WS-Discovery network ports. After binding messages will be
    * received. It binds on both IPv4 and IPv6.
    */
    //TODO: Rename to bind()
    void start();
    
    /*!
     * Send a WS-Discovery probe message. This will cause all devices in the network
     * that match to \p typeList and \p scopeList to send a probeMatch back.
     * \param  typeList List of types that a device should match
     * \param  scopeList List of scoped that a device should match
     */
    void sendProbe(const QList<KDQName>& typeList, const QList<QUrl> &scopeList);
    
    /*!
     * Send a WS-Discovery resolve message. This will cause a specific device 
     * in the network to send a resolveMatch back.
     * \param endpointReference The identifier of the device of interest
     */
    void sendResolve(const QString& endpointReference);

private slots:
    //TODO: Make implementation private
    void receivedMessage(const KDSoapMessage& replyMessage, const KDSoapHeaders& replyHeaders, const QHostAddress& senderAddress, quint16 senderPort);

private:
    KDSoapUdpClient * m_soapUdpClient;
};

#endif // WSDISCOVERYCLIENT_H
