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
#ifndef WSDISCOVERYPROBEJOB_H
#define WSDISCOVERYPROBEJOB_H

#include "wsdiscoveryclient_export.h"
#include <KDSoapClient/KDQName>
#include <QTimer>
#include <QUrl>
#include <QObject>

class WSDiscoveryClient;
class WSDiscoveryTargetService;

/*!
 * \brief Periodically probe the network for WS-Discovery devices.
 * 
 * You can set a filter for interested types and scopes, only devices that match 
 * the filter will be reported. After starting it will probe the network and 
 * report any matches.
 */
class WSDISCOVERYCLIENT_EXPORT WSDiscoveryProbeJob : public QObject
{
    Q_OBJECT
public:
    /*!
     * Creates a WSDiscoveryProbeJob
     * \param parent is both the QObject parent as the WSDiscoveryClient 
     *   used for sending and receiving messages
     */
    explicit WSDiscoveryProbeJob(WSDiscoveryClient *parent);

    /*!
     * \return List of types to filter devices with
     */
    QList<KDQName> typeList() const;
    /*!
     * \param typeList List of types to filter devices with
     */
    void setTypeList(const QList<KDQName> &typeList);
    /*!
     * \param type Adds a type to the list to filter devices with
     */
    void addType(const KDQName& type);

    /*!
     * \return List of scopes to filter devices with
     */
    QList<QUrl> scopeList() const;
    /*!
     * \param scopeList List of scopes to filter devices with
     */
    void setScopeList(const QList<QUrl> &scopeList);
    /*!
     * \param scope Adds a scopes to the list to filter devices with
     */
    void addScope(const QUrl& scope);

    /*!
     * \return The interval between probes
     */
    int interval() const;
    /*!
     * \param interval Sets the interval between probes
     */
    void setInterval(int interval);

signals:
    /*!
     * Emitted when a match is received
     * \param matchedService The service as described in the match
     */
    void matchReceived(const WSDiscoveryTargetService& matchedService);

public slots:
    /*!
     * Start sending periodic probes 
     */
    void start();
    /*!
     * Stop sending periodic probes 
     */
    void stop();

//TODO: Hide private interface
private slots:
    void timeout();
    void probeMatchReceived(const WSDiscoveryTargetService& probeMatchService);

private:
    WSDiscoveryClient * m_client;
    QList<KDQName> m_typeList;
    QList<QUrl> m_scopeList;
    QTimer m_timer;
};

#endif // WSDISCOVERYPROBEJOB_H
